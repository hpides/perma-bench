#include "benchmark_suite.hpp"

#include <spdlog/spdlog.h>

#include <json.hpp>

#include "benchmark_factory.hpp"
#include "utils.hpp"

namespace {

nlohmann::json single_results_to_json(const perma::SingleBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"bm_type", bm.benchmark_type_as_str()},
          {"matrix_args", bm.get_benchmark_configs()[0].matrix_args},
          {"benchmarks", bm_results}};
}

nlohmann::json parallel_results_to_json(const perma::ParallelBenchmark& bm, const nlohmann::json& bm_results) {
  return {{"bm_name", bm.benchmark_name()},
          {"sub_bm_names", {bm.get_benchmark_name_one(), bm.get_benchmark_name_two()}},
          {"bm_type", bm.benchmark_type_as_str()},
          {"matrix_args",
           {{bm.get_benchmark_name_one(), bm.get_benchmark_configs()[0].matrix_args},
            {bm.get_benchmark_name_two(), bm.get_benchmark_configs()[1].matrix_args}}},
          {"benchmarks", bm_results}};
}

nlohmann::json benchmark_results_to_json(const perma::Benchmark& bm, const nlohmann::json& bm_results) {
  if (bm.get_benchmark_type() == perma::BenchmarkType::Single) {
    return single_results_to_json(dynamic_cast<const perma::SingleBenchmark&>(bm), bm_results);
  } else if (bm.get_benchmark_type() == perma::BenchmarkType::Parallel) {
    return parallel_results_to_json(dynamic_cast<const perma::ParallelBenchmark&>(bm), bm_results);
  } else {
    return {{"bm_name", bm.benchmark_name()}, {"bm_type", bm.benchmark_type_as_str()}, {"benchmarks", bm_results}};
  }
}

void print_bm_information(const perma::Benchmark& bm) {
  if (bm.get_benchmark_type() == perma::BenchmarkType::Single) {
    spdlog::info("Running single benchmark {} with matrix args {}", bm.benchmark_name(),
                 nlohmann::json(bm.get_benchmark_configs()[0].matrix_args).dump());
  } else if (bm.get_benchmark_type() == perma::BenchmarkType::Parallel) {
    const auto& benchmark = dynamic_cast<const perma::ParallelBenchmark&>(bm);
    spdlog::info("Running parallel benchmark {} with sub benchmarks {} and {}.", benchmark.benchmark_name(),
                 benchmark.get_benchmark_name_one(), benchmark.get_benchmark_name_two());
  } else {
    // This should never happen
    spdlog::critical("Unknown benchmark type: {}", bm.get_benchmark_type());
    perma::utils::crash_exit();
  }
}

}  // namespace

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file,
                                    const std::filesystem::path& result_directory) {
  std::vector<YAML::Node> configs = BenchmarkFactory::get_config_files(config_file);
  nlohmann::json results = nlohmann::json::array();

  // Start single benchmarks
  std::vector<SingleBenchmark> single_benchmarks = BenchmarkFactory::create_single_benchmarks(pmem_directory, configs);
  spdlog::info("Found {} single benchmark{}.", single_benchmarks.size(), single_benchmarks.size() != 1 ? "s" : "");
  std::vector<ParallelBenchmark> parallel_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks(pmem_directory, configs);
  spdlog::info("Found {} parallel benchmark{}.", parallel_benchmarks.size(),
               parallel_benchmarks.size() != 1 ? "s" : "");

  Benchmark* previous_bm = nullptr;
  std::vector<Benchmark*> benchmarks{};
  benchmarks.reserve(single_benchmarks.size() + parallel_benchmarks.size());
  for (Benchmark& benchmark : single_benchmarks) {
    benchmarks.push_back(&benchmark);
  }
  for (Benchmark& benchmark : parallel_benchmarks) {
    benchmarks.push_back(&benchmark);
  }

  const std::filesystem::path result_file = utils::create_result_file(result_directory, config_file);

  if (benchmarks.empty()) {
    spdlog::warn("No benchmarks found. Nothing to do.");
    return;
  }

  bool had_error = false;
  nlohmann::json matrix_bm_results = nlohmann::json::array();
  bool printed_info = false;
  for (size_t i = 0; i < benchmarks.size(); ++i) {
    Benchmark& benchmark = *benchmarks[i];
    if (previous_bm && previous_bm->benchmark_name() != benchmark.benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      nlohmann::json bm_results = benchmark_results_to_json(*previous_bm, matrix_bm_results);
      utils::write_benchmark_results(result_file, bm_results);
      matrix_bm_results = nlohmann::json::array();
      previous_bm->tear_down(/*force=*/true);
      printed_info = false;
    }

    if (!printed_info) {
      print_bm_information(benchmark);
      printed_info = true;
    }

    benchmark.create_data_files();
    benchmark.set_up();
    const bool success = benchmark.run();
    previous_bm = &benchmark;

    if (!success) {
      // Encountered an error. End suite gracefully.
      had_error = true;
      break;
    }

    matrix_bm_results += benchmark.get_result_as_json();
    benchmark.tear_down(false);
    spdlog::info("Completed {0}/{1} benchmark{2}.", i + 1, benchmarks.size(), benchmarks.size() > 1 ? "s" : "");
  }

  if (!benchmarks.empty()) {
    nlohmann::json bm_results = benchmark_results_to_json(*previous_bm, matrix_bm_results);
    utils::write_benchmark_results(result_file, bm_results);
    previous_bm->tear_down(/*force=*/true);
  }

  if (had_error) {
    utils::crash_exit();
  }

  spdlog::info("Finished all benchmarks successfully.");
}

}  // namespace perma
