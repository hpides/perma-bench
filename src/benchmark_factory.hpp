#pragma once

#include "benchmark.hpp"

namespace perma {

namespace internal {
static const std::string CONFIG_FILE_EXTENSION = ".yaml";
}  // namespace internal
class BenchmarkFactory {
 public:
  static std::vector<Benchmark> create_benchmarks(const std::filesystem::path& pmem_directory,
                                                  const std::filesystem::path& config_file_path);

 private:
  static std::vector<Benchmark> create_benchmark_matrix(const std::string& bm_name,
                                                        const std::filesystem::path& pmem_directory,
                                                        YAML::Node& config_args, YAML::Node& matrix_args);
};

}  // namespace perma
