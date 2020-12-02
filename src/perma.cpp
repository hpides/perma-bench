#include <spdlog/spdlog.h>

#include <CLI11.hpp>

#include "benchmark_suite.hpp"
#include "utils.hpp"

using namespace perma;

namespace {
auto check_path_exists = [](const std::string& path) {
  return (!std::filesystem::exists(path)) ? "No such file or directory: " + path : "";
};

auto check_path_exists_or_create = [](const std::string& path) {
  bool exists = std::filesystem::exists(path);
  bool is_created = false;
  if (!exists) {
    is_created = std::filesystem::create_directories(path);
  }
  return (!is_created && !exists) ? "Neither the path exists nor can it be created: " + path : "";
};

auto check_is_dir = [](const std::string& pmem_dir) {
  return (!std::filesystem::is_directory(pmem_dir)) ? "Path is not a directory: " + pmem_dir : "";
};
}  // namespace

constexpr auto DEFAULT_CONFIG_PATH = "configs/bm-suite.yaml";

int main(int argc, char** argv) {
#ifdef NDEBUG
  spdlog::set_level(spdlog::level::info);
#else
  spdlog::set_level(spdlog::level::debug);
#endif

  CLI::App app{"PerMA-Bench: Benchmark your Persistent Memory"};

  // Define command line args
  std::filesystem::path config_file = std::filesystem::current_path() / DEFAULT_CONFIG_PATH;
  app.add_option("-c,--config", config_file,
                 "Path to the benchmark config YAML file (default: " + std::string{DEFAULT_CONFIG_PATH} + ")")
      ->check(check_path_exists_or_create);

  // Define result directory
  std::filesystem::path result_path = std::filesystem::current_path();
  app.add_option("-r,--result-dir", result_path,
                 "Path to the result directory (default: " + std::string{result_path} + ")");

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

  // Make sure that the benchmarks are NUMA-aware. Setting this in the main thread will inherit to all child threads.
  init_numa(pmem_directory);

  // Run the actual benchmarks after parsing and validating them.
  BenchmarkSuite::run_benchmarks(pmem_directory, config_file, result_path);

  return 0;
}
