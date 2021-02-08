#include "benchmark_suite.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <json.hpp>

#include "benchmark_factory.hpp"

namespace {

nlohmann::json benchmark_results_to_json(const perma::UnaryBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"matrix_args", bm.get_benchmark_config().matrix_args},
          {"benchmarks", bm_results}};
}

nlohmann::json benchmark_results_to_json(const perma::BinaryBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"matrix_args_one", bm.get_benchmark_config_one().matrix_args},
          {"matrix_args_two", bm.get_benchmark_config_two().matrix_args},
          {"benchmarks", bm_results}};
}

}  // namespace

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file,
                                    const std::filesystem::path& result_directory) {
  UnaryBenchmark* previous_u_bm = nullptr;
  BinaryBenchmark* previous_b_bm = nullptr;
  std::vector<YAML::Node> configs = BenchmarkFactory::get_config_files(config_file);
  std::vector<UnaryBenchmark> single_benchmarks = BenchmarkFactory::create_single_benchmarks(pmem_directory, configs);

  spdlog::info("Running {0} single benchmark{1}...", single_benchmarks.size(), single_benchmarks.size() > 1 ? "s" : "");
  nlohmann::json results = nlohmann::json::array();
  nlohmann::json matrix_bm_results = nlohmann::json::array();
  bool printed_info = false;
  for (size_t i = 0; i < single_benchmarks.size(); ++i) {
    UnaryBenchmark& benchmark = single_benchmarks[i];

    if (previous_u_bm && previous_u_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      results += benchmark_results_to_json(*previous_u_bm, matrix_bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_u_bm->tear_down(/*force=*/true);
      printed_info = false;
    }
    if (!printed_info) {
      spdlog::info("Running single benchmark {} with matrix args {}", benchmark.benchmark_name(),
                   nlohmann::json(benchmark.get_benchmark_config().matrix_args).dump());
      printed_info = true;
    }

    benchmark.create_data_file();
    benchmark.set_up();
    benchmark.run();

    matrix_bm_results += benchmark.get_result_as_json();

    benchmark.tear_down(false);
    previous_u_bm = &benchmark;
    spdlog::info("Completed {0}/{1} single benchmark{2}.", i + 1, single_benchmarks.size(),
                 single_benchmarks.size() > 1 ? "s" : "");
  }

  // Add last matrix benchmark to final results
  if (!single_benchmarks.empty()) {
    results += benchmark_results_to_json(single_benchmarks.back(), matrix_bm_results);
    previous_u_bm->tear_down(/*force=*/true);
  }

  std::vector<BinaryBenchmark> parallel_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks(pmem_directory, configs);
  spdlog::info("Running {0} parallel benchmark{1}...", parallel_benchmarks.size(),
               parallel_benchmarks.size() > 1 ? "s" : "");
  printed_info = false;
  matrix_bm_results = nlohmann::json::array();
  for (size_t i = 0; i < parallel_benchmarks.size(); ++i) {
    BinaryBenchmark& benchmark = parallel_benchmarks[i];

    if (previous_b_bm && previous_b_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      results += benchmark_results_to_json(*previous_b_bm, matrix_bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_b_bm->tear_down(/*force=*/true);
      printed_info = false;
    }
    if (!printed_info) {
      spdlog::info("Running parallel benchmark {} with sub benchmarks {} and {}.", benchmark.benchmark_name(),
                   benchmark.get_benchmark_name_one(), benchmark.get_benchmark_name_two());
      printed_info = true;
    }

    benchmark.create_data_file();
    benchmark.set_up();
    benchmark.run();

    matrix_bm_results += benchmark.get_result_as_json();

    benchmark.tear_down(false);
    previous_b_bm = &benchmark;
    spdlog::info("Completed {0}/{1} parallel benchmark{2}.", i + 1, parallel_benchmarks.size(),
                 parallel_benchmarks.size() > 1 ? "s" : "");
  }
  results += benchmark_results_to_json(parallel_benchmarks.back(), matrix_bm_results);

  const std::filesystem::path result_file = result_directory / config_file.stem().concat("-results.json");
  std::ofstream output(result_file);
  output << results.dump() << std::endl;
  output.close();

  // Final clean up
  if (previous_b_bm != nullptr) {
    previous_b_bm->tear_down(/*force=*/true);
  }
}

}  // namespace perma
