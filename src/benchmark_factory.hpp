#pragma once

#include "benchmark.hpp"

namespace perma {

class BenchmarkFactory {
 public:
  static std::vector<Benchmark> create_benchmarks(const std::filesystem::path& pmem_directory,
                                                                   const std::filesystem::path& config_file);

 private:
  static std::vector<Benchmark> create_benchmark_matrix(const std::string& bm_name,
                                                                         const std::filesystem::path& pmem_directory,
                                                                         YAML::Node& config_args,
                                                                         YAML::Node& matrix_args);
};

}  // namespace perma
