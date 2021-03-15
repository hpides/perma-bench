#include "utils.hpp"

#include <libpmem.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <random>
#include <thread>

#include "read_write_ops.hpp"

#ifdef HAS_NUMA
#include <numa.h>
#include <numaif.h>
#endif

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, const size_t expected_length) {
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), 0, 0, 0, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not map file: " + file.string()};
  }

  if (mapped_length != expected_length) {
    throw std::runtime_error("Existing pmem data file has wrong size.");
  }

  if (!is_pmem) {
    spdlog::warn("File {} is not in persistent memory!", file.string());
  }

  return static_cast<char*>(pmem_addr);
}

char* create_pmem_file(const std::filesystem::path& file, const size_t length) {
  const std::filesystem::path base_dir = file.parent_path();
  if (!std::filesystem::exists(base_dir)) {
    if (!std::filesystem::create_directories(base_dir)) {
      throw std::runtime_error{"Could not create dir: " + base_dir.string()};
    }
  }

  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), length, PMEM_FILE_CREATE, 0644, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not create file: " + file.string() + " (error: " + std::strerror(errno) + ")"};
  }

  if (!is_pmem) {
    spdlog::warn("File {} is not in persistent memory!", file.string());
  }

  if (length != mapped_length) {
    throw std::runtime_error{"Mapped size different than specified size!"};
  }

  return static_cast<char*>(pmem_addr);
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
}

void prefault_file(char* addr, const uint64_t total_memory_range) {
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

void init_numa(const std::filesystem::path& pmem_dir) {
#ifndef HAS_NUMA
  // Don't do anything, as we don't have NUMA support.
  spdlog::warn("Running without NUMA-awareness.");
  return;
#else
  if (numa_available() < 0) {
    throw std::runtime_error("NUMA supported but could not be found!");
  }

  const size_t num_numa_nodes = numa_num_configured_nodes();
  spdlog::debug("Number of NUMA nodes in system: {}", num_numa_nodes);
  if (num_numa_nodes < 2) {
    // Do nothing, as there isn't any affinity to be set.
    spdlog::info("Not setting NUMA-awareness with {} nodes", num_numa_nodes);
    return;
  }

  const std::filesystem::path temp_file = generate_random_file_name(pmem_dir);
  // Create random 2 MiB file
  const size_t temp_size = 2u * (1024u * 1024u);
  char* pmem_data = create_pmem_file(temp_file, temp_size);
  rw_ops::write_data(pmem_data, pmem_data + temp_size);

  int numa_node = -1;
  get_mempolicy(&numa_node, NULL, 0, (void*)pmem_data, MPOL_F_NODE | MPOL_F_ADDR);
  pmem_unmap(pmem_data, temp_size);
  std::filesystem::remove(temp_file);

  if (numa_node < 0 || numa_node > num_numa_nodes) {
    spdlog::warn("Could not determine NUMA node. Running without NUMA-awareness.");
    return;
  }
  spdlog::info("Detected memory on NUMA node: {}", numa_node);

  bitmask* numa_nodes = numa_bitmask_alloc(num_numa_nodes);
  std::vector<uint16_t> allowed_numa_nodes{};
  for (int other_node = 0; other_node < num_numa_nodes; ++other_node) {
    const size_t numa_dist = numa_distance(numa_node, other_node);
    spdlog::trace("NUMA-Distance: {} -> {} = {}", numa_node, other_node, numa_dist);
    if (numa_dist < 20) {
      // This should cover all NUMA nodes that are close, i.e. self = 10, close = 11.
      numa_bitmask_setbit(numa_nodes, other_node);
      allowed_numa_nodes.push_back(other_node);
    }
  }

  if (allowed_numa_nodes.empty()) {
    // Distance info did not work
    numa_bitmask_setbit(numa_nodes, numa_node);
    allowed_numa_nodes.push_back(numa_node);
  }

  const std::string used_noes_str = std::accumulate(
      allowed_numa_nodes.begin(), allowed_numa_nodes.end(), std::string(),
      [](const auto& a, const auto b) -> std::string { return a + (a.length() > 0 ? ", " : "") + std::to_string(b); });
  spdlog::info("Setting NUMA-affinity to node{}: {}", allowed_numa_nodes.size() > 1 ? "s" : "", used_noes_str);
  numa_bind(numa_nodes);
  numa_free_nodemask(numa_nodes);
#endif
}

#ifdef HAS_NUMA
void set_to_far_cpus() {
  const size_t num_numa_nodes = numa_num_configured_nodes();
  if (num_numa_nodes < 2) {
    // Do nothing, as there isn't any affinity to be set.
    // Infos printed during init numa
    return;
  }

  bitmask* init_node_mask = numa_get_run_node_mask();
  bitmask* thread_node_mask = numa_allocate_nodemask();

  for (uint64_t numa_node = 0; numa_node < num_numa_nodes; numa_node++) {
    // Set numa node if not set in initial near node mask
    if (!numa_bitmask_isbitset(init_node_mask, numa_node)) {
      spdlog::trace("Far NUMA node {} set", numa_node);
      numa_bitmask_setbit(thread_node_mask, numa_node);
    }
  }

  if (*thread_node_mask->maskp == 0) {
    spdlog::warn("Benchmark is set to the NUMA far pattern but no far NUMA nodes are available.");
    spdlog::warn("Running benchmark on near NUMA nodes.");
    return;
  }
  numa_run_on_node_mask(thread_node_mask);
}
#endif

}  // namespace perma
