#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkSuite {
 public:
  void run_benchmarks(const std::filesystem::path& pmem_directory, const std::filesystem::path& config_file);

 private:
  std::vector<std::unique_ptr<Benchmark>> benchmarks_;
};

}  // namespace perma
