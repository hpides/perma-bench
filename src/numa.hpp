#pragma once

#include <filesystem>
#include <vector>

namespace perma {

void init_numa(const std::filesystem::path& pmem_dir, const std::vector<uint64_t>& arg_nodes, bool is_dram,
               bool ignore_numa);

void set_to_far_cpus();

bool has_far_numa_nodes();

}  // namespace perma
