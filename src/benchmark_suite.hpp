#pragma once

#include "benchmark.hpp"

namespace nvmbm {

class BenchmarkSuite {
 public:
  void start_benchmarks(const std::string& file_name);

 private:
  void prepare_benchmarks(const std::string& file_name);
  void run_benchmarks();
  std::vector<std::unique_ptr<Benchmark>> benchmarks_;
};

}  // namespace nvmbm
