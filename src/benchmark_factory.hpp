#pragma once

#include "benchmark.hpp"

namespace perma {

namespace internal {
static const std::string CONFIG_FILE_EXTENSION = ".yaml";
}  // namespace internal

class BenchmarkFactory {
 public:
  static std::vector<YAML::Node> get_config_files(const std::filesystem::path& config_file_path);

  static std::vector<SingleBenchmark> create_single_benchmarks(const std::filesystem::path& pmem_directory,
                                                               std::vector<YAML::Node>& configs);

  static std::vector<ParallelBenchmark> create_parallel_benchmarks(const std::filesystem::path& pmem_directory,
                                                                   std::vector<YAML::Node>& configs);

 private:
  static std::vector<BenchmarkConfig> create_benchmark_matrix(const std::filesystem::path& pmem_directory,
                                                              YAML::Node& config_args, YAML::Node& matrix_args);
};

}  // namespace perma
