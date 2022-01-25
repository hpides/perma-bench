#pragma once

#include "benchmark.hpp"
#include "parallel_benchmark.hpp"
#include "single_benchmark.hpp"

namespace perma {

static constexpr auto CONFIG_FILE_EXTENSION = ".yaml";

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
  static void parse_yaml_node(const std::filesystem::path& pmem_directory, std::vector<BenchmarkConfig>& bm_configs,
                              YAML::iterator& par_it, std::string& unique_name);
};

}  // namespace perma
