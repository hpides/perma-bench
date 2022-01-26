#include <spdlog/spdlog.h>

#include <CLI11.hpp>

#include "benchmark_suite.hpp"
#include "numa.hpp"

namespace {

std::string empty_directory(const std::string& path) {
  if (!std::filesystem::is_empty(path)) {
    return "PMem benchmark directory '" + path + "' must be empty.";
  }
  return "";
}

}  // namespace

using namespace perma;

constexpr auto DEFAULT_WORKLOAD_PATH = "workloads";
constexpr auto DEFAULT_RESULT_PATH = "results";

int main(int argc, char** argv) {
#ifdef NDEBUG
  spdlog::set_level(spdlog::level::info);
#else
  spdlog::set_level(spdlog::level::debug);
#endif

  CLI::App app{"PerMA-Bench: Benchmark your Persistent Memory"};

  // Define command line args
  std::filesystem::path config_file = std::filesystem::current_path() / DEFAULT_WORKLOAD_PATH;
  app.add_option("-c,--config", config_file,
                 "Path to the benchmark config YAML file(s) (default: " + std::string{DEFAULT_WORKLOAD_PATH} + ")")
      ->check(CLI::ExistingPath);

  // Define result directory
  std::filesystem::path result_path = std::filesystem::current_path() / DEFAULT_RESULT_PATH;
  app.add_option("-r,--results", result_path, "Path to the result directory (default: " + result_path.string() + ")");

  // Define NUMA nodes to pin to.
  // This takes a list of nodes, e.g., --numa=0,1
  std::vector<uint64_t> numa_nodes;
  auto numa_opt =
      app.add_option<std::vector<uint64_t>>(
             "--numa", numa_nodes,
             "Comma separated list of NUMA nodes to pin to, e.g., --numa=0,1 (default: determined from PMem directory)")
          ->delimiter(',')
          ->expected(1, 10);

  // Flag if numa should be initialized.
  bool ignore_numa;
  auto ignore_numa_opt = app.add_flag("--no-numa", ignore_numa,
                                      "Set this flag to ignore all NUMA related behavior for using, e.g., numactl")
                             ->default_val(false);

  // Path to PMem directory
  std::filesystem::path pmem_directory;
  auto path_opt =
      app.add_option(
             "-p,--path", pmem_directory,
             "Path to empty persistent memory directory in which to perform the benchmarks, e.g., /mnt/pmem1/perma")
          ->default_str("")
          ->check(CLI::ExistingDirectory)
          ->check(empty_directory);

  // Flag if DRAM should be used
  bool use_dram;
  auto dram_flg = app.add_flag("--dram", use_dram, "Set this flag to run benchmarks in DRAM")->default_val(false);

  // Do not allow path to be set if dram is set
  dram_flg->excludes(path_opt);
  // Do not allow dram flag to be set if path is set
  path_opt->excludes(dram_flg);
  // Do not allow skipping initialization of numa and setting numa nodes
  ignore_numa_opt->excludes(numa_opt);
  numa_opt->excludes(ignore_numa_opt);

  try {
    app.parse(argc, argv);
    if (path_opt->empty() && dram_flg->empty()) {
      throw CLI::RequiredError("Must specify either --path or --dram");
    }
  } catch (const CLI::ParseError& e) {
    app.failure_message(CLI::FailureMessage::help);
    return app.exit(e);
  }

  // Make sure that the benchmarks are NUMA-aware. Setting this in the main thread will inherit to all child threads.
  init_numa(pmem_directory, numa_nodes, use_dram, ignore_numa);

  // Run the actual benchmarks after parsing and validating them.
  const std::string run_location = use_dram ? "DRAM" : pmem_directory.string();
  spdlog::info("Running benchmarks on '{}' with config(s) from '{}'.", run_location, config_file.string());
  spdlog::info("Writing results to '{}'.", result_path.string());

  try {
    BenchmarkSuite::run_benchmarks(pmem_directory, config_file, result_path);
  } catch (const PermaException& e) {
    // Clean up files before exiting
    if (!use_dram) {
      std::filesystem::remove_all(pmem_directory / "*");
    }
    throw e;
  }

  return 0;
}
