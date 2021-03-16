#include <spdlog/spdlog.h>

#include <CLI11.hpp>

#include "benchmark_suite.hpp"
#include "utils.hpp"

using namespace perma;

namespace {
auto check_path_exists = [](const std::string& path) {
  return (!std::filesystem::exists(path)) ? "No such file or directory: " + path : "";
};

auto check_is_dir = [](const std::string& pmem_dir) {
  return (!std::filesystem::is_directory(pmem_dir)) ? "Path is not a directory: " + pmem_dir : "";
};
}  // namespace

constexpr auto DEFAULT_CONFIG_PATH = "configs/workloads";

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
      ->check(check_path_exists);

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

  // Path to PMem directory
  // --path
  std::filesystem::path pmem_directory;
  auto path_opt =
      app.add_option("-p,--path", pmem_directory,
                     "Path to persistent memory directory in which to perform the benchmarks, e.g., /mnt/pmem1/")
          ->check(check_path_exists)
          ->check(check_is_dir);

  // Flag if DRAM should be used
  // --dram
  bool use_dram;
  auto dram_flg = app.add_flag("--dram", use_dram, "Set this flag to run benchmarks in DRAM");

  // Do not allow path to be set if dram is set
  dram_flg->excludes(path_opt);
  // Do not allow dram flag to be set if path is set
  path_opt->excludes(dram_flg);
  try {
    app.parse(argc, argv);
    if (path_opt->empty() && dram_flg->empty()) {
      throw CLI::RequiredError("Either --path or --dram must be set.");
    }
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
