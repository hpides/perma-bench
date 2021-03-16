#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkSuite {
 public:
  static void run_benchmarks(const std::filesystem::path& pmem_directory, const bool is_dram,
                             const std::filesystem::path& config_file, const std::filesystem::path& result_directory);
};

}  // namespace perma
