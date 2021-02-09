#include "single_benchmark.hpp"

#include <utility>

namespace perma {

void SingleBenchmark::run() {
  for (size_t thread_index = 0; thread_index < configs_[0].number_threads; thread_index++) {
    pools_[0].emplace_back(&run_in_thread, std::ref(thread_configs_[0][thread_index]), std::ref(configs_[0]));
  }

  // wait for all threads
  for (std::thread& thread : pools_[0]) {
    thread.join();
  }
}

void SingleBenchmark::create_data_file() { create_single_data_file(configs_[0], &pmem_data_[0], pmem_files_[0]); }

void SingleBenchmark::set_up() {
  single_set_up(configs_[0], pmem_data_[0], results_[0], pools_[0], thread_configs_[0]);
}

void SingleBenchmark::tear_down(const bool force) {
  if (pmem_data_[0] != nullptr) {
    pmem_unmap(pmem_data_[0], configs_[0].total_memory_range);
    pmem_data_[0] = nullptr;
  }
  if (owns_pmem_files_[0] || force) {
    std::filesystem::remove(pmem_files_[0]);
  }
}

nlohmann::json SingleBenchmark::get_result_as_json() {
  nlohmann::json result;
  result["config"] = get_json_config(0);
  result.update(results_[0]->get_result_as_json());
  return result;
}

SingleBenchmark::SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config)
    : Benchmark(benchmark_name, internal::BenchmarkType::Single,
                std::vector<std::filesystem::path>{generate_random_file_name(config.pmem_directory)},
                std::vector<bool>{true}, std::vector<BenchmarkConfig>{config},
                std::vector<std::unique_ptr<BenchmarkResult>>{std::move(std::make_unique<BenchmarkResult>(config))}) {}

SingleBenchmark::SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config,
                                 std::filesystem::path pmem_file)
    : Benchmark(benchmark_name, internal::BenchmarkType::Single,
                std::vector<std::filesystem::path>{std::move(pmem_file)}, std::vector<bool>{false},
                std::vector<BenchmarkConfig>{config},
                std::vector<std::unique_ptr<BenchmarkResult>>{std::move(std::make_unique<BenchmarkResult>(config))}) {}

}  // namespace perma
