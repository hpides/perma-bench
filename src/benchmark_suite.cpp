#include "benchmark_suite.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <json.hpp>

#include "benchmark_factory.hpp"

namespace {

nlohmann::json benchmark_results_to_json(const perma::SingleBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"bm_type", bm.benchmark_type_as_str()},
          {"matrix_args", bm.get_benchmark_config()[0].matrix_args},
          {"benchmarks", bm_results}};
}

nlohmann::json benchmark_results_to_json(const perma::ParallelBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"sub_bm_names", {bm.get_benchmark_name_one(), bm.get_benchmark_name_two()}},
          {"bm_type", bm.benchmark_type_as_str()},
          {"matrix_args",
           {{bm.get_benchmark_name_one(), bm.get_benchmark_config()[0].matrix_args},
            {bm.get_benchmark_name_two(), bm.get_benchmark_config()[1].matrix_args}}},
          {"benchmarks", bm_results}};
}

}  // namespace

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file,
                                    const std::filesystem::path& result_directory) {
  std::vector<YAML::Node> configs = BenchmarkFactory::get_config_files(config_file);
  nlohmann::json results = nlohmann::json::array();
  // Start single benchmarks
  SingleBenchmark* previous_single_bm = nullptr;
  std::vector<SingleBenchmark> single_benchmarks = BenchmarkFactory::create_single_benchmarks(pmem_directory, configs);

  spdlog::info("Running {0} single benchmark{1}...", single_benchmarks.size(), single_benchmarks.size() > 1 ? "s" : "");
  nlohmann::json matrix_bm_results = nlohmann::json::array();
  bool printed_info = false;
  for (size_t i = 0; i < single_benchmarks.size(); ++i) {
    SingleBenchmark& benchmark = single_benchmarks[i];

    if (previous_single_bm && previous_single_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      results += benchmark_results_to_json(*previous_single_bm, matrix_bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_single_bm->tear_down(/*force=*/true);
      printed_info = false;
    }
    if (!printed_info) {
      spdlog::info("Running single benchmark {} with matrix args {}", benchmark.benchmark_name(),
                   nlohmann::json(benchmark.get_benchmark_config()[0].matrix_args).dump());
      printed_info = true;
    }

    benchmark.create_data_file();
    benchmark.set_up();
    benchmark.run();

    matrix_bm_results += benchmark.get_result_as_json();

    benchmark.tear_down(false);
    previous_single_bm = &benchmark;
    spdlog::info("Completed {0}/{1} single benchmark{2}.", i + 1, single_benchmarks.size(),
                 single_benchmarks.size() > 1 ? "s" : "");
  }

  // Add last matrix benchmark to final results and final clean up single benchmarks
  if (!single_benchmarks.empty()) {
    results += benchmark_results_to_json(single_benchmarks.back(), matrix_bm_results);
    previous_single_bm->tear_down(/*force=*/true);
  }

  // Start parallel benchmarks
  ParallelBenchmark* previous_par_bm = nullptr;
  std::vector<ParallelBenchmark> parallel_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks(pmem_directory, configs);
  spdlog::info("Running {0} parallel benchmark{1}...", parallel_benchmarks.size(),
               parallel_benchmarks.size() > 1 ? "s" : "");
  matrix_bm_results = nlohmann::json::array();
  printed_info = false;
  for (size_t i = 0; i < parallel_benchmarks.size(); ++i) {
    ParallelBenchmark& benchmark = parallel_benchmarks[i];

    if (previous_par_bm && previous_par_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      results += benchmark_results_to_json(*previous_par_bm, matrix_bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_par_bm->tear_down(/*force=*/true);
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
    previous_par_bm = &benchmark;
    spdlog::info("Completed {0}/{1} parallel benchmark{2}.", i + 1, parallel_benchmarks.size(),
                 parallel_benchmarks.size() > 1 ? "s" : "");
  }
  // Add last matrix benchmark to final results and final clean up parallel benchmarks
  if (!parallel_benchmarks.empty()) {
    results += benchmark_results_to_json(parallel_benchmarks.back(), matrix_bm_results);
    previous_par_bm->tear_down(true);
  }

  const std::filesystem::path result_file = result_directory / config_file.stem().concat("-results.json");
  std::ofstream output(result_file);
  output << results.dump() << std::endl;
  output.close();
}

}  // namespace perma
