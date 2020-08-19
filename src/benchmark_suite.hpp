#pragma once

#include "benchmark.hpp"
#include "benchmark_factory.hpp"

namespace nvmbm {

class BenchmarkSuite {
 private:
  std::vector<Benchmark> benchmarks;
  BenchmarkFactory builder;
};

}  // namespace nvmbm
