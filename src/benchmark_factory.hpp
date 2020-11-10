#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkFactory {
 public:
  static std::vector<std::unique_ptr<Benchmark>> create_benchmarks(const std::filesystem::path& pmem_directory,
                                                                   const std::filesystem::path& config_file);
};

}  // namespace perma
