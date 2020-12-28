#include "benchmark_suite.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <json.hpp>

#include "benchmark_factory.hpp"

namespace {

nlohmann::json benchmark_results_to_json(const perma::Benchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"matrix_args", bm.get_benchmark_config().matrix_args},
          {"benchmarks", bm_results}};
}

}  // namespace

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file,
                                    const std::filesystem::path& result_directory) {
  Benchmark* previous_bm = nullptr;
  std::vector<Benchmark> benchmarks = BenchmarkFactory::create_benchmarks(pmem_directory, config_file);

  spdlog::info("Running benchmarks...");
  nlohmann::json results = nlohmann::json::array();
  nlohmann::json matrix_bm_results = nlohmann::json::array();

  for (size_t i = 0; i < benchmarks.size(); ++i) {
    Benchmark& benchmark = benchmarks[i];
    if (previous_bm && previous_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      results += benchmark_results_to_json(*previous_bm, matrix_bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_bm->tear_down(/*force=*/true);
    }

    benchmark.create_data_file();
    benchmark.set_up();
    benchmark.run();

    matrix_bm_results += benchmark.get_result_as_json();

    benchmark.tear_down();
    previous_bm = &benchmark;
    spdlog::info("Completed {0}/{1} benchmark{2}.", i + 1, benchmarks.size(), benchmarks.size() > 1 ? "s" : "");
  }

  // Add last matrix benchmark to final results
  results += benchmark_results_to_json(benchmarks.back(), matrix_bm_results);

  const std::filesystem::path result_file = result_directory / config_file.stem().concat("-results.json");
  std::ofstream output(result_file);
  output << results.dump() << std::endl;
  output.close();

  // Final clean up
  previous_bm->tear_down(/*force=*/true);
}

}  // namespace perma
