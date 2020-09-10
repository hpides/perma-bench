#pragma once

#include "benchmark.hpp"
#include "benchmark_factory.hpp"

namespace perma {

class BenchmarkSuite {
 private:
  std::vector<Benchmark> benchmarks;
  BenchmarkFactory builder;
};

}  // namespace perma
