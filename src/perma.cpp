#include <spdlog/spdlog.h>

#include <CLI11.hpp>

#include "benchmark_suite.hpp"
#include "utils.hpp"

using namespace perma;

constexpr auto DEFAULT_CONFIG_PATH = "configs/workloads";
constexpr auto DEFAULT_RESULT_PATH = "results";

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
      ->check(CLI::ExistingPath);

  // Define result directory
  std::filesystem::path result_path = std::filesystem::current_path() / DEFAULT_RESULT_PATH;
  app.add_option("-r,--results", result_path, "Path to the result directory (default: " + result_path.string() + ")");

  // Define NUMA nodes to pin to.
  // This takes a list of nodes, e.g., --numa=0,1
  std::vector<uint64_t> numa_nodes;
  app.add_option<std::vector<uint64_t>>(
         "--numa", numa_nodes,
         "Comma separated list of NUMA nodes to pin to, e.g., --numa=0,1 (default: determined from PMem directory)")
      ->delimiter(',')
      ->expected(1, 10);

  // Path to PMem directory
  std::filesystem::path pmem_directory;
  auto path_opt =
      app.add_option("-p,--path", pmem_directory,
                     "Path to persistent memory directory in which to perform the benchmarks, e.g., /mnt/pmem1/")
          ->default_str("")
          ->check(CLI::ExistingDirectory);

  // Flag if DRAM should be used
  bool use_dram;
  auto dram_flg = app.add_flag("--dram", use_dram, "Set this flag to run benchmarks in DRAM")->default_val(false);

  // Do not allow path to be set if dram is set
  dram_flg->excludes(path_opt);
  // Do not allow dram flag to be set if path is set
  path_opt->excludes(dram_flg);

  try {
    app.parse(argc, argv);
    if (path_opt->empty() && dram_flg->empty()) {
      throw CLI::RequiredError("Either --path or --dram");
    }
  } catch (const CLI::ParseError& e) {
    app.failure_message(CLI::FailureMessage::help);
    return app.exit(e);
  }

  // Make sure that the benchmarks are NUMA-aware. Setting this in the main thread will inherit to all child threads.
  init_numa(pmem_directory, numa_nodes, use_dram);

  // Run the actual benchmarks after parsing and validating them.
  spdlog::info("Running benchmarks on '{}' with config(s) from '{}'.", pmem_directory.string(), config_file.string());
  spdlog::info("Writing results to '{}'.", result_path.string());
  BenchmarkSuite::run_benchmarks(pmem_directory, config_file, result_path);

  return 0;
}
