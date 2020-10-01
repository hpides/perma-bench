#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkSuite {
 public:
  void run_benchmarks(const std::string& file_name);

 private:
  std::vector<std::unique_ptr<Benchmark>> benchmarks_;
};

}  // namespace perma
