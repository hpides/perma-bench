#include "numa.hpp"

#include <spdlog/spdlog.h>

#include "read_write_ops.hpp"
#include "utils.hpp"

#ifdef HAS_NUMA
#include <numa.h>
#include <numaif.h>
#endif

namespace perma {

namespace internal {

static const size_t NUMA_FAR_DISTANCE = 20;

static bool IGNORE_NUMA = false;

}  // namespace internal

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
    if (numa_dist < internal::NUMA_FAR_DISTANCE) {
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

void init_numa(const std::filesystem::path& pmem_dir, const std::vector<uint64_t>& arg_nodes, const bool is_dram,
               const bool ignore_numa) {  // TODO: handle numa for dram

  if (ignore_numa) {
    internal::IGNORE_NUMA = true;
    spdlog::info("NUMA is ignored in this run.");
    return;
  }

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

  std::vector<uint64_t> inv_numa_nodes{};
  std::vector<uint64_t> set_numa_nodes{};
  for (uint64_t numa_node = 0; numa_node < num_numa_nodes; numa_node++) {
    // Set numa node if not set in initial near node mask
    if (numa_bitmask_isbitset(init_node_mask, numa_node)) {
      set_numa_nodes.push_back(numa_node);
    } else {
      inv_numa_nodes.push_back(numa_node);
    }
  }

  std::vector<uint64_t> far_numa_nodes{};
  for (uint64_t possible_far_node : inv_numa_nodes) {
    bool found_far_node = true;
    // Check for each possible far NUMA node if it is far to every node that is set
    for (uint64_t set_node : set_numa_nodes) {
      const size_t numa_dist = numa_distance(possible_far_node, set_node);
      if (numa_dist < internal::NUMA_FAR_DISTANCE) {
        // This should cover all NUMA nodes that are close, i.e., bigger than self = 10 and close = 11.
        found_far_node = false;
        break;
      }
    }
    if (found_far_node) {
      // This covers one NUMA node that is far to each user-provided NUMA node.
      // If found move to next NUMA node in list
      spdlog::trace("Found far NUMA node {}.", possible_far_node);
      far_numa_nodes.push_back(possible_far_node);
    }
  }

  return far_numa_nodes;
#endif
}

void set_to_far_cpus() {
  if (internal::IGNORE_NUMA) {
    spdlog::critical("Setting far NUMA nodes when running in no-numa mode.");
    crash_exit();
  }
#ifndef HAS_NUMA
  throw std::runtime_error("Running far numa pattern benchmark without NUMA-awareness.");
#else
  return set_numa_nodes(get_far_nodes(), numa_num_configured_nodes());
#endif
}

bool has_far_numa_nodes() {
  if (internal::IGNORE_NUMA) {
    spdlog::critical("Checking far NUMA nodes when running in no-numa mode.");
    crash_exit();
  }
#ifndef HAS_NUMA
  return false;
#else
  return !get_far_nodes().empty();
#endif
}

}  // namespace perma
