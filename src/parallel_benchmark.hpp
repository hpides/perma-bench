#pragma once

#include "benchmark.hpp"

namespace perma {

class ParallelBenchmark : public Benchmark {
 public:
  ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                    std::string second_benchmark_name, const BenchmarkConfig& first_config,
                    const BenchmarkConfig& second_config);
  ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                    std::string second_benchmark_name, const BenchmarkConfig& first_config,
                    const BenchmarkConfig& second_config, std::filesystem::path pmem_file_first);
  ParallelBenchmark(const std::string& benchmark_name, std::string first_benchmark_name,
                    std::string second_benchmark_name, const BenchmarkConfig& first_config,
                    const BenchmarkConfig& second_config, std::filesystem::path pmem_file_first,
                    std::filesystem::path pmem_file_second);

  ParallelBenchmark(ParallelBenchmark&& other) = default;
  ParallelBenchmark(const ParallelBenchmark& other) = delete;
  ParallelBenchmark& operator=(const ParallelBenchmark& other) = delete;
  ParallelBenchmark& operator=(ParallelBenchmark&& other) = delete;

  /** Main run method which executes the benchmark. `setup()` should be called before this. */
  void run() override;

  /**
   * Generates the data needed for the benchmark.
   * This is probably the first method to be called so that a virtual
   * address space is available to generate the IO addresses.
   */
  void create_data_file() override;

  /** Create all the IO addresses ahead of time to avoid unnecessary ops during the actual benchmark. */
  void set_up() override;

  /** Clean up after te benchmark */
  void tear_down(bool force) override;

  /** Return the results as a JSON to be exported to the user and visualization. */
  nlohmann::json get_result_as_json() override;

  const std::string& get_benchmark_name_one() const;
  const std::string& get_benchmark_name_two() const;

  ~ParallelBenchmark() { ParallelBenchmark::tear_down(false); }

 private:
  const std::string benchmark_name_one_;
  const std::string benchmark_name_two_;
};

}  // namespace perma
