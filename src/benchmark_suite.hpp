#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkSuite {
 public:
  static void run_benchmarks(const std::filesystem::path& pmem_directory, const std::filesystem::path& config_file);
};

}  // namespace perma
