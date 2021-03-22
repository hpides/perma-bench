#include "utils.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <random>
#include <thread>
#include <vector>

#include "json.hpp"
#include "libpmem.h"
#include "read_write_ops.hpp"
#include "spdlog/spdlog.h"

#ifdef HAS_NUMA
#include <numa.h>
#include <numaif.h>
#endif

namespace perma {

void internal::setPMEM_MAP_FLAGS(const int flags) { internal::PMEM_MAP_FLAGS = flags; }

char* map_file(const std::filesystem::path& file, const bool is_dram, size_t expected_length) {
  uint64_t fd = -1;
  int flags;
  if (!is_dram) {
    const mode_t mode = 0644;
    fd = open(file.c_str(), O_RDWR | O_DIRECT, mode);
    if (fd == -1) {
      throw std::runtime_error{"Could not open file: " + file.string()};
    }
    flags = internal::PMEM_MAP_FLAGS;
  } else {
    flags = internal::DRAM_MAP_FLAGS;
  }

  void* addr = mmap(nullptr, expected_length, PROT_READ | PROT_WRITE, flags, fd, 0);
  close(fd);
  if (addr == MAP_FAILED) {
    throw std::runtime_error{"Could not map file: " + file.string()};
  }

  return static_cast<char*>(addr);
}

char* create_file(const std::filesystem::path& file, const bool is_dram, size_t length) {
  if (!is_dram) {
    const std::filesystem::path base_dir = file.parent_path();
    if (!std::filesystem::exists(base_dir)) {
      if (!std::filesystem::create_directories(base_dir)) {
        throw std::runtime_error{"Could not create dir: " + base_dir.string()};
      }
    }

    std::ofstream temp_stream{file};
    temp_stream.close();
    std::filesystem::resize_file(file, length);
  }
  return map_file(file, is_dram, length);
}

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir) {
  std::string str("abcdefghijklmnopqrstuvwxyz");
  std::random_device rd;
  std::mt19937 generator(rd());
  std::shuffle(str.begin(), str.end(), generator);
  const std::string file_name = str + ".file";
  const std::filesystem::path file{file_name};
  return base_dir / file;
}

void generate_read_data(char* addr, const uint64_t total_memory_range) {
  spdlog::info("Generating {} GB of random data to read.", total_memory_range / internal::ONE_GB);
  std::vector<std::thread> thread_pool;
  thread_pool.reserve(internal::NUM_UTIL_THREADS - 1);
  uint64_t thread_memory_range = total_memory_range / internal::NUM_UTIL_THREADS;
  for (uint8_t thread_count = 0; thread_count < internal::NUM_UTIL_THREADS - 1; thread_count++) {
    char* from = addr + thread_count * thread_memory_range;
    const char* to = addr + (thread_count + 1) * thread_memory_range;
    thread_pool.emplace_back(rw_ops::write_data, from, to);
  }

  rw_ops::write_data(addr + (internal::NUM_UTIL_THREADS - 1) * thread_memory_range, addr + total_memory_range);

  // wait for all threads
  for (std::thread& thread : thread_pool) {
    thread.join();
  }
  spdlog::info("Finished generating data.");
}

void prefault_file(char* addr, const uint64_t total_memory_range) {
  spdlog::debug("Prefaulting data.");
  const size_t page_size = internal::PMEM_PAGE_SIZE;
  const size_t num_prefault_pages = total_memory_range / page_size;
  for (size_t prefault_offset = 0; prefault_offset < num_prefault_pages; ++prefault_offset) {
    addr[prefault_offset * page_size] = '\0';
  }
}

uint64_t duration_to_nanoseconds(const std::chrono::high_resolution_clock::duration duration) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

// FROM https://www.csee.usf.edu/~kchriste/tools/genzipf.c and
// https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
//===========================================================================
//=  Function to generate Zipf (power law) distributed random variables     =
//=    - Input: alpha and N                                                 =
//=    - Output: Returns with Zipf distributed random variable              =
//===========================================================================
uint64_t zipf(const double alpha, const uint64_t n) {
  static bool first = true;  // Static first time flag
  static double c = 0;       // Normalization constant
  static double* sum_probs;  // Pre-calculated sum of probabilities
  double z;                  // Uniform random number (0 < z < 1)
  int zipf_value;            // Computed exponential value to be returned

  // Compute normalization constant on first call only
  if (first) {
    for (uint64_t i = 1; i <= n; i++) {
      c = c + (1.0 / pow((double)i, alpha));
    }
    c = 1.0 / c;

    sum_probs = static_cast<double*>(malloc((n + 1) * sizeof(*sum_probs)));
    sum_probs[0] = 0;
    for (uint64_t i = 1; i <= n; i++) {
      sum_probs[i] = sum_probs[i - 1] + c / pow((double)i, alpha);
    }
    first = false;
  }

  // Pull a uniform random number (0 < z < 1)
  do {
    z = rand_val();
  } while ((z == 0) || (z == 1));

  // Map z to the value
  int low = 1, high = n, mid;
  do {
    mid = floor((low + high) / 2);
    if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
      zipf_value = mid;
      break;
    } else if (sum_probs[mid] >= z) {
      high = mid - 1;
    } else {
      low = mid + 1;
    }
  } while (low <= high);

  return zipf_value - 1;  // Subtract one to map to a range from 0 - (n - 1)
}

//=========================================================================
//= Multiplicative LCG for generating uniform(0.0, 1.0) random numbers    =
//=   - x_n = 7^5*x_(n-1)mod(2^31 - 1)                                    =
//=   - With x seeded to 1 the 10000th x value should be 1043618065       =
//=   - From R. Jain, "The Art of Computer Systems Performance Analysis," =
//=     John Wiley & Sons, 1991. (Page 443, Figure 26.2)                  =
//=========================================================================
double rand_val() {
  const long a = 16807;       // Multiplier
  const long m = 2147483647;  // Modulus
  const long q = 127773;      // m div a
  const long r = 2836;        // m mod a
  static long x = 1687248;    // Random int value
  long x_div_q;               // x divided by q
  long x_mod_q;               // x modulo q
  long x_new;                 // New x value

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0) {
    x = x_new;
  } else {
    x = x_new + m;
  }

  // Return a random value between 0.0 and 1.0
  return ((double)x / m);
}

void crash_exit() {
  spdlog::error("Terminating due to a critical error. See logs above for more details.");
  exit(1);
}

void log_numa_nodes(const std::vector<uint64_t>& nodes) {
  const std::string used_nodes_str = std::accumulate(
      nodes.begin(), nodes.end(), std::string(),
      [](const auto& a, const auto b) -> std::string { return a + (a.length() > 0 ? ", " : "") + std::to_string(b); });
  spdlog::info("Setting NUMA-affinity to node{}: {}", nodes.size() > 1 ? "s" : "", used_nodes_str);
}

std::vector<uint64_t> auto_detect_numa(const std::filesystem::path& pmem_dir, const size_t num_numa_nodes,
                                       const bool is_dram) {
#ifndef HAS_NUMA
  spdlog::critical("Cannot detect numa nodes without NUMA support.");
  crash_exit();
#else
  const std::filesystem::path temp_file = generate_random_file_name(pmem_dir);
  // Create random 2 MiB file
  const size_t temp_size = 2u * (1024u * 1024u);
  char* pmem_data = create_file(temp_file, is_dram, temp_size);
  rw_ops::write_data(pmem_data, pmem_data + temp_size);

  int numa_node = -1;
  get_mempolicy(&numa_node, NULL, 0, (void*)pmem_data, MPOL_F_NODE | MPOL_F_ADDR);
  munmap(pmem_data, temp_size);
  if (!is_dram) {
    std::filesystem::remove(temp_file);
  }

  if (numa_node < 0 || numa_node > num_numa_nodes) {
    spdlog::warn("Could not determine NUMA node. Running without NUMA-awareness.");
    return {};
  }

  spdlog::info("Detected memory on NUMA node: {}", numa_node);

  std::vector<uint64_t> allowed_numa_nodes{};
  for (int other_node = 0; other_node < num_numa_nodes; ++other_node) {
    const size_t numa_dist = numa_distance(numa_node, other_node);
    spdlog::trace("NUMA-Distance: {} -> {} = {}", numa_node, other_node, numa_dist);
    if (numa_dist < 20) {
      // This should cover all NUMA nodes that are close, i.e. self = 10, close = 11.
      allowed_numa_nodes.push_back(other_node);
    }
  }

  if (allowed_numa_nodes.empty()) {
    // Distance info did not work
    allowed_numa_nodes.push_back(numa_node);
  }

  return allowed_numa_nodes;
#endif
}

void set_numa_nodes(const std::vector<uint64_t>& nodes, const size_t num_numa_nodes) {
#ifndef HAS_NUMA
  spdlog::critical("Cannot set numa nodes without NUMA support.");
  crash_exit();
#else
  bitmask* numa_nodes = numa_bitmask_alloc(num_numa_nodes);
  for (uint64_t node : nodes) {
    if (node >= num_numa_nodes) {
      spdlog::critical("Given numa node too large! (given: {}, max: {})", node, num_numa_nodes - 1);
      crash_exit();
    }
    numa_bitmask_setbit(numa_nodes, node);
  }

  numa_run_on_node_mask(numa_nodes);
  numa_free_nodemask(numa_nodes);
#endif
}

void init_numa(const std::filesystem::path& pmem_dir, const std::vector<uint64_t>& arg_nodes, const bool is_dram) {
#ifndef HAS_NUMA
  if (!arg_nodes.empty()) {
    spdlog::critical("Cannot explicitly set numa nodes without NUMA support.");
    crash_exit();
  }

  // Don't do anything, as we don't have NUMA support.
  spdlog::warn("Running without NUMA-awareness.");
  return;
#else
  if (numa_available() < 0) {
    throw std::runtime_error("NUMA supported but could not be found!");
  }

  const size_t num_numa_nodes = numa_num_configured_nodes();
  spdlog::info("Number of NUMA nodes in system: {}", num_numa_nodes);

  if (!arg_nodes.empty()) {
    // User specified numa nodes via the command line. No need to auto-detect.
    if (num_numa_nodes < arg_nodes.size()) {
      spdlog::critical("More numa nodes specified than detected on server.");
      crash_exit();
    }
    spdlog::info("Setting NUMA nodes according to command line arguments.");
    log_numa_nodes(arg_nodes);
    return set_numa_nodes(arg_nodes, num_numa_nodes);
  }

  if (num_numa_nodes < 2) {
    // Do nothing, as there isn't any affinity to be set.
    spdlog::info("Not setting NUMA-awareness with {} node(s).", num_numa_nodes);
    return;
  }

  const std::vector<uint64_t> detected_nodes = auto_detect_numa(pmem_dir, num_numa_nodes, is_dram);
  log_numa_nodes(detected_nodes);
  return set_numa_nodes(detected_nodes, num_numa_nodes);
#endif
}

std::vector<uint64_t> get_far_nodes() {
#ifndef HAS_NUMA
  throw std::runtime_error("Running far numa pattern benchmark without NUMA-awareness.");
#else
  const size_t num_numa_nodes = numa_num_configured_nodes();
  bitmask* init_node_mask = numa_get_run_node_mask();

  std::vector<uint64_t> far_numa_nodes{};
  for (uint64_t numa_node = 0; numa_node < num_numa_nodes; numa_node++) {
    // Set numa node if not set in initial near node mask
    if (!numa_bitmask_isbitset(init_node_mask, numa_node)) {
      spdlog::trace("Far NUMA node {} set", numa_node);
      far_numa_nodes.push_back(numa_node);
    }
  }
  return far_numa_nodes;
#endif
}

void set_to_far_cpus() {
#ifndef HAS_NUMA
  throw std::runtime_error("Running far numa pattern benchmark without NUMA-awareness.");
#else
  return set_numa_nodes(get_far_nodes(), numa_num_configured_nodes());
#endif
}

bool has_far_numa_nodes() {
#ifndef HAS_NUMA
  return false;
#else
  return !get_far_nodes().empty();
#endif
}

std::string get_time_string() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H-%M");
  return ss.str();
}

std::filesystem::path create_result_file(const std::filesystem::path& result_dir,
                                         const std::filesystem::path& config_path) {
  std::error_code ec;
  const bool created = std::filesystem::create_directories(result_dir, ec);
  if (!created && ec) {
    spdlog::critical("Could not create result directory! Error: {}", ec.message());
    perma::crash_exit();
  }

  std::string file_name;
  const std::string result_suffix = "-results-" + get_time_string() + ".json";
  if (std::filesystem::is_regular_file(config_path)) {
    file_name = config_path.stem().concat(result_suffix);
  } else if (std::filesystem::is_directory(config_path)) {
    std::filesystem::path config_dir_name = *(--config_path.end());
    file_name = config_dir_name.concat(result_suffix);
  } else {
    spdlog::critical("Unexpected config file type for '{}'.", config_path.string());
    perma::crash_exit();
  }

  std::filesystem::path result_path = result_dir / file_name;
  std::ofstream result_file(result_path);
  result_file << nlohmann::json::array() << std::endl;

  return result_path;
}

void write_benchmark_results(const std::filesystem::path& result_path, const nlohmann::json& results) {
  nlohmann::json all_results;
  std::ifstream previous_result_file(result_path);
  previous_result_file >> all_results;

  if (!all_results.is_array()) {
    previous_result_file.close();
    spdlog::critical("Result file '{}' is corrupted! Content must be a valid JSON array.", result_path.string());
    perma::crash_exit();
  }

  all_results.push_back(results);
  // Clear all existing data.
  std::ofstream new_result_file(result_path, std::ofstream::trunc);
  new_result_file << std::setw(2) << all_results << std::endl;
}

void print_segfault_error() {
  spdlog::critical("A thread encountered an unexpected SIGSEGV!");
  spdlog::critical(
      "Please create an issue on GitHub (https://github.com/hpides/perma-bench/issues/new) "
      "with your configuration and system information so that we can try to fix this.");
}

}  // namespace perma
