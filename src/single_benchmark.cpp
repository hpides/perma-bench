#include "single_benchmark.hpp"

#include <csignal>

namespace {

volatile sig_atomic_t thread_error;
void thread_error_handler(int) { thread_error = 1; }

}  // namespace

namespace perma {

bool SingleBenchmark::run() {
  signal(SIGSEGV, thread_error_handler);

  const BenchmarkConfig& config = configs_[0];
  std::vector<std::thread>& pool = pools_[0];
  for (size_t thread_index = 0; thread_index < config.number_threads; thread_index++) {
    pool.emplace_back(run_in_thread, &thread_configs_[0][thread_index], std::ref(config));
  }

  // wait for all threads
  for (std::thread& thread : pool) {
    if (thread_error) {
      utils::print_segfault_error();
      return false;
    }
    thread.join();
  }

  return true;
}

void SingleBenchmark::create_data_files() {
  pmem_data_.push_back(create_pmem_data_file(configs_[0], memory_regions_[0]));
  dram_data_.push_back(create_dram_data(configs_[0]));
}

void SingleBenchmark::set_up() {
  pools_.resize(1);
  thread_configs_.resize(1);
  single_set_up(configs_[0], pmem_data_[0], dram_data_[0], executions_[0].get(), results_[0].get(), &pools_[0],
                &thread_configs_[0]);
}

nlohmann::json SingleBenchmark::get_result_as_json() {
  nlohmann::json result;
  result["config"] = get_json_config(0);
  result.update(results_[0]->get_result_as_json());
  return result;
}

SingleBenchmark::SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config,
                                 std::vector<std::unique_ptr<BenchmarkExecution>>&& executions,
                                 std::vector<std::unique_ptr<BenchmarkResult>>&& results)
    : Benchmark(
          benchmark_name, BenchmarkType::Single,
          std::vector<MemoryRegion>{{utils::generate_random_file_name(config.pmem_directory), true, config.is_hybrid}},
          std::vector<BenchmarkConfig>{config}, std::move(executions), std::move(results)) {}

SingleBenchmark::SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config,
                                 std::vector<std::unique_ptr<BenchmarkExecution>>&& executions,
                                 std::vector<std::unique_ptr<BenchmarkResult>>&& results,
                                 std::filesystem::path pmem_file)
    : Benchmark(benchmark_name, BenchmarkType::Single,
                std::vector<MemoryRegion>{{std::move(pmem_file), false, config.is_hybrid}},
                std::vector<BenchmarkConfig>{config}, std::move(executions), std::move(results)) {}

}  // namespace perma
