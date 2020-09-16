#pragma once

#include <yaml-cpp/yaml.h>

#include "benchmark.hpp"

namespace perma {

class BenchmarkFactory {
 public:
  static std::vector<std::unique_ptr<Benchmark>> create_benchmarks(const std::string& file_name);
};

}  // namespace perma
