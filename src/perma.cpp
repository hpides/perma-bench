#include <iostream>

#include "benchmark_suite.hpp"

using namespace perma;

constexpr auto DEFAULT_CONFIG_PATH = "configs/bm-suite.yaml";

int main(int argc, char** argv) {
  std::filesystem::path config_file = std::filesystem::current_path() / ".." / DEFAULT_CONFIG_PATH;
  if (argc < 2) {
    std::cerr << "Usage: ./perma-bench /path/to/pmem/dir [/path/to/config]" << std::endl;
    throw std::invalid_argument{"Need to specify pmem directory."};
  }

  const std::filesystem::path pmem_directory = argv[1];
  const bool pmem_dir_exists = std::filesystem::exists(pmem_directory);
  if (pmem_dir_exists && !std::filesystem::is_directory(pmem_directory)) {
    throw std::invalid_argument{"Need to specify pmem directory but got: " + pmem_directory.string()};
  }

  if (argc > 2) {
    // Use config file specified by user.
    config_file = argv[2];
  }

  // Run the actual benchmarks after parsing and validating them.
  BenchmarkSuite::run_benchmarks(pmem_directory, config_file);

  if (!pmem_dir_exists) {
    // We created the directory, so we should remove it again.
    std::filesystem::remove_all(pmem_directory);
  }

  return 0;
}
