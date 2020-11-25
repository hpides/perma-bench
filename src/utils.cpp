#include "utils.hpp"

#include <libpmem.h>
#include <ndctl/libndctl.h>
#include <spdlog/spdlog.h>
#include <mntent.h>

#include <algorithm>
#include <random>

#include "read_write_ops.hpp"

#ifdef HAS_NUMA
#include <numa.h>
#include <numaif.h>
#include <sched.h>
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
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), length, PMEM_FILE_CREATE, 0644, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not create file: " + file.string()};
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
  if (!std::filesystem::exists(base_dir)) {
    if (!std::filesystem::create_directories(base_dir)) {
      throw std::runtime_error{"Could not create dir: " + base_dir.string()};
    }
  }
  std::string str("abcdefghijklmnopqrstuvwxyz");
  std::random_device rd;
  std::mt19937 generator(rd());
  std::shuffle(str.begin(), str.end(), generator);
  const std::string file_name = str + ".file";
  const std::filesystem::path file{file_name};
  return base_dir / file;
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

std::optional<std::string> get_device_of_mount_point(const std::filesystem::path& pmem_dir)
{
  const std::string pmem_dir_str = pmem_dir.string();
  FILE* mount_file = fopen("/proc/mounts", "r");

  size_t max_mount_match_size = 0;
  std::string best_device;
  mntent* mount_entry = getmntent(mount_file);

  while (mount_entry != nullptr) {
    const std::string_view mount_dir = mount_entry->mnt_dir;
    const std::string_view mount_fsname = mount_entry->mnt_fsname;
    const std::string_view mount_type = mount_entry->mnt_type;
    spdlog::info("Checking mount entry: {} -> {} (type: {})", mount_fsname, mount_dir, mount_type);

    if (pmem_dir_str.find(mount_dir) != std::string::npos) {
      // Current entry matches the user's PMem directory
      spdlog::info("Found matching mount entry: {} with device: {}", mount_dir, mount_fsname);
      if (mount_dir.size() > max_mount_match_size) {
        best_device = mount_entry->mnt_fsname;
        max_mount_match_size = mount_dir.size();
      }
    }

    mount_entry = getmntent(mount_file);
  }

  fclose(mount_file);
  if (best_device.empty()) {
    return {};
  }

  spdlog::info("Running on device {}", best_device);
  return best_device;
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
    return;
  }

  const std::filesystem::path temp_file = generate_random_file_name(pmem_dir);
  // Create random 16 MiB file
//  const size_t temp_size = 16u * (1024u * 1024u);
//  char* pmem_data = create_pmem_file(temp_file, temp_size);
//  rw_ops::write_data(pmem_data, pmem_data + temp_size);
//  pmem_unmap(pmem_data, temp_size);
//  pmem_data = map_pmem_file(temp_file, temp_size);


//  const size_t num_pages = 10;
//  std::vector<int> nodes(num_pages);
//  std::vector<char*> pages(num_pages);
//  std::vector<int> statuses(num_pages);
//  std::vector<char> dummy(num_pages);
//  for (int i = 0; i < num_pages; ++i) {
//    char* raw_data = pmem_data + (4096 * i);
//    dummy[i] = *raw_data;
//    pages[i] = raw_data;
//  }
//
//  spdlog::info("max nodes: {}", numa_max_node());
//  auto x = move_pages(0, num_pages, (void**) pages.data(), nullptr, statuses.data(), MPOL_MF_MOVE);
//  if (x != 0) {
//    spdlog::info("ret: {}", std::strerror(errno));
//  }

  std::optional<std::string> opt_mount_device = get_device_of_mount_point(pmem_dir);
  if (!opt_mount_device) {
    spdlog::info("Did not find the mounted device!");
  }

  const std::string& mount_device = opt_mount_device.value();

  auto get_numa_info_from_dir = [](const std::string& mount_dev, ndctl_bus** dir_bus, ndctl_region** dir_region, ndctl_namespace** dir_namespace) {
    struct ndctl_ctx* ndctx;
    if (ndctl_new(&ndctx) != 0) {
      throw std::runtime_error("Could not create ndctl context.");
    }

    *dir_bus = nullptr;
    *dir_region = nullptr;
    *dir_namespace = nullptr;

    struct ndctl_bus* bus;
    struct ndctl_region* region;
    struct ndctl_namespace* ndns;

    ndctl_bus_foreach(ndctx, bus) {
      ndctl_region_foreach(bus, region) {
        ndctl_namespace_foreach(region, ndns) {
          struct ndctl_btt* btt;
          struct ndctl_pfn* pfn;
          std::string dev_name;

          if ((btt = ndctl_namespace_get_btt(ndns))) {
            dev_name = ndctl_btt_get_block_device(btt);
          } else if ((pfn = ndctl_namespace_get_pfn(ndns))) {
            dev_name = ndctl_pfn_get_block_device(pfn);
          } else {
            dev_name = ndctl_namespace_get_block_device(ndns);
          }

          if (dev_name.empty()) {
            continue;
          }

          spdlog::info("Checking region for device name: {}", dev_name);

          if (mount_dev.size() > dev_name.size() && std::equal(dev_name.rbegin(), dev_name.rend(), mount_dev.rbegin())) {
            // Device matches the mount device
            spdlog::info("Found matching region for: {}", dev_name);
            *dir_bus = bus;
            *dir_region = region;
            *dir_namespace = ndns;
            ndctl_unref(ndctx);
            return;
          }
        }
      }
    }

    ndctl_unref(ndctx);
  };

  ndctl_bus* dir_bus;
  ndctl_region* dir_region;
  ndctl_namespace* dir_namespace;
  get_numa_info_from_dir(mount_device, &dir_bus, &dir_region, &dir_namespace);
//  ndctl_namespace_get_region(dir_namespace);

  if (dir_bus == nullptr) {
    spdlog::info("no region found");
  }

  uint32_t region_id = ndctl_region_get_id(dir_region);
  uint32_t bus_id = ndctl_bus_get_id(dir_bus);
  spdlog::info("dev bus: {}, {}", ndctl_bus_get_devname(dir_bus), ndctl_bus_get_provider(dir_bus));
  spdlog::info("Region id: {}, bus id: {}", region_id, bus_id);
  int x = 10;

  ndctl_region* pRegion = ndctl_namespace_get_region(dir_namespace);
  spdlog::info("numa ns: {}, {}", ndctl_namespace_get_numa_node(dir_namespace), ndctl_region_get_numa_node(dir_region));


//  ndctl_btt* btt;
//  ndctl_namespace_get_btt(ndns);




//  for (int status : statuses) {
//    spdlog::info("{}", std::strerror(std::abs(status)));
//  }

//  int numa_node = -1;
//  get_mempolicy(&numa_node, NULL, 0, (void*) pmem_data, MPOL_F_NODE | MPOL_F_ADDR);

//  pmem_unmap(pmem_data, temp_size);
//  std::filesystem::remove(temp_file);


//  spdlog::info("Setting benchmark NUMA-affinity to node: {}", numa_node);
//  numa_run_on_node(numa_node);
#endif
}

}  // namespace perma
