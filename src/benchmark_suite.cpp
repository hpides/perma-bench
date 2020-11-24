#include "benchmark_suite.hpp"

#include <iostream>
#include <json.hpp>

#include "benchmark_factory.hpp"
#include <spdlog/spdlog.h>

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file) {
  Benchmark* previous_bm = nullptr;
  std::vector<std::unique_ptr<Benchmark>> benchmarks = BenchmarkFactory::create_benchmarks(pmem_directory, config_file);

  spdlog::info("Running benchmarks...");
  for (size_t i = 0; i < benchmarks.size(); ++i) {
    Benchmark* benchmark = benchmarks[i].get();
    if (previous_bm && previous_bm->benchmark_name() != benchmark->benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      previous_bm->tear_down(/*force=*/true);
    }

    benchmark->create_data_file();
    benchmark->set_up();
    benchmark->run();

    // TODO: Handle get_result
    nlohmann::json result = benchmark->get_result();
//    spdlog::info("Number io results: {}", result.dump(2).size());
    spdlog::info("Number io results: {}", result["results"].size());
    spdlog::info("Bandwidth:\n{}", result["bandwidth"].dump(2));
    spdlog::info("Latency:\n{}", result["latency"].dump(2));

    benchmark->tear_down();
    previous_bm = benchmark;

    spdlog::debug("Completed {0}/{1} benchmark(s).",  i + 1, benchmarks.size());
  }

  // Final clean up
  previous_bm->tear_down(/*force=*/true);
}

}  // namespace perma
