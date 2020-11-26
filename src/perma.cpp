#include <spdlog/spdlog.h>

#include <CLI11.hpp>

#include "benchmark_suite.hpp"

using namespace perma;

namespace {
auto check_path_exists = [](const std::string& path) {
  return (!std::filesystem::exists(path)) ? "No such file or directory: " + path : "";
};

auto check_is_dir = [](const std::string& pmem_dir) {
  return (!std::filesystem::is_directory(pmem_dir)) ? "Path is not a directory: " + pmem_dir : "";
};
}  // namespace

constexpr auto DEFAULT_CONFIG_PATH = "configs/bm-suite.yaml";

int main(int argc, char** argv) {
  CLI::App app{"PerMA-Bench: Benchmark your Persistent Memory"};

  // Define command line args
  std::filesystem::path config_file = std::filesystem::current_path() / ".." / DEFAULT_CONFIG_PATH;
  app.add_option("-c,--config", config_file,
                 "Path to the benchmark config YAML file (default: " + std::string{DEFAULT_CONFIG_PATH} + ")")
      ->check(check_path_exists);

  // Define NUMA nodes to pin to.
  // This takes a list of nodes, e.g., --numa=0,1
  std::vector<uint16_t> numa_nodes;
  app.add_option<std::vector<uint16_t>>(
         "--numa", numa_nodes,
         "Comma separated list of NUMA nodes to pin to, e.g., --numa=0,1 (default: determined from PMem directory)")
      ->delimiter(',')
      ->expected(1, 10);

  // Require path to PMem directory
  std::filesystem::path pmem_directory;
  app.add_option("/path/to/pmem", pmem_directory,
                 "Path to persistent memory directory in which to perform the benchmarks, e.g., /mnt/pmem1/")
      ->required()
      ->check(check_path_exists)
      ->check(check_is_dir);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    app.failure_message(CLI::FailureMessage::help);
    return app.exit(e);
  }

  // Run the actual benchmarks after parsing and validating them.
  BenchmarkSuite::run_benchmarks(pmem_directory, config_file);

  return 0;
}
