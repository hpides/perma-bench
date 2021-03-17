#include "parallel_benchmark.hpp"

#include <csignal>

namespace {

volatile sig_atomic_t thread_error;
void thread_error_handler(int) { thread_error = 1; }

}  // namespace

namespace perma {

bool ParallelBenchmark::run() {
  signal(SIGSEGV, thread_error_handler);

  for (size_t bm_num = 0; bm_num < configs_.size(); ++bm_num) {
    for (size_t thread_index = 0; thread_index < configs_[bm_num].number_threads; thread_index++) {
      pools_[bm_num].emplace_back(&run_in_thread, std::ref(thread_configs_[bm_num][thread_index]),
                                  std::ref(configs_[bm_num]));
    }
  }

  // wait for all threads
  for (std::vector<std::thread>& pool : pools_) {
    for (std::thread& thread : pool) {
      if (thread_error) {
        print_segfault_error();
        return false;
      }
      thread.join();
    }
  }

  return true;
}

void ParallelBenchmark::create_data_file() {
  pmem_data_.push_back(create_single_data_file(configs_[0], pmem_files_[0]));
  pmem_data_.push_back(create_single_data_file(configs_[1], pmem_files_[1]));
}

void ParallelBenchmark::set_up() {
  pools_.resize(2);
  thread_configs_.resize(2);
  single_set_up(configs_[0], pmem_data_[0], results_[0], pools_[0], thread_configs_[0]);
  single_set_up(configs_[1], pmem_data_[1], results_[1], pools_[1], thread_configs_[1]);
}

void ParallelBenchmark::tear_down(bool force) {
  for (size_t index = 0; index < pmem_data_.size(); index++) {
    if (pmem_data_[index] != nullptr) {
      pmem_unmap(pmem_data_[index], configs_[index].total_memory_range);
      pmem_data_[index] = nullptr;
    }
    if (owns_pmem_files_[index] || force) {
      std::filesystem::remove(pmem_files_[index]);
    }
  }
}

nlohmann::json ParallelBenchmark::get_result_as_json() {
  nlohmann::json result;
  result["configs"][benchmark_name_one_] = get_json_config(0);
  result["configs"][benchmark_name_two_] = get_json_config(1);
  result["results"][benchmark_name_one_].update(results_[0]->get_result_as_json());
  result["results"][benchmark_name_two_].update(results_[1]->get_result_as_json());
  return result;
}

ParallelBenchmark::ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                                     std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                     const BenchmarkConfig& second_config,
                                     std::vector<std::unique_ptr<BenchmarkResult>>& results)
    : Benchmark(benchmark_name, internal::BenchmarkType::Parallel,
                std::vector<std::filesystem::path>{generate_random_file_name(first_config.pmem_directory),
                                                   generate_random_file_name(second_config.pmem_directory)},
                std::vector<bool>{true, true}, std::vector<BenchmarkConfig>{first_config, second_config},
                std::move(results)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)} {}

ParallelBenchmark::ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                                     std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                     const BenchmarkConfig& second_config,
                                     std::vector<std::unique_ptr<BenchmarkResult>>& results,
                                     std::filesystem::path pmem_file_first)
    : Benchmark(benchmark_name, internal::BenchmarkType::Parallel,
                std::vector<std::filesystem::path>{std::move(pmem_file_first),
                                                   generate_random_file_name(second_config.pmem_directory)},
                std::vector<bool>{false, true}, std::vector<BenchmarkConfig>{first_config, second_config},
                std::move(results)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)} {}

ParallelBenchmark::ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                                     std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                     const BenchmarkConfig& second_config,
                                     std::vector<std::unique_ptr<BenchmarkResult>>& results,
                                     std::filesystem::path pmem_file_first, std::filesystem::path pmem_file_second)
    : Benchmark(benchmark_name, internal::BenchmarkType::Parallel,
                std::vector<std::filesystem::path>{std::move(pmem_file_first), std::move(pmem_file_second)},
                std::vector<bool>{false, false}, std::vector<BenchmarkConfig>{first_config, second_config},
                std::move(results)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)} {}

const std::string& ParallelBenchmark::get_benchmark_name_one() const { return benchmark_name_one_; }

const std::string& ParallelBenchmark::get_benchmark_name_two() const { return benchmark_name_two_; }

}  // namespace perma
