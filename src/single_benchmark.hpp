#pragma once

#include <string>

#include "benchmark.hpp"

namespace perma {

class SingleBenchmark : public Benchmark {
 public:
  SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config,
                  std::vector<std::unique_ptr<BenchmarkExecution>>&& executions,
                  std::vector<std::unique_ptr<BenchmarkResult>>&& results);
  SingleBenchmark(const std::string& benchmark_name, const BenchmarkConfig& config,
                  std::vector<std::unique_ptr<BenchmarkExecution>>&& executions,
                  std::vector<std::unique_ptr<BenchmarkResult>>&& results, std::filesystem::path pmem_file);

  SingleBenchmark(SingleBenchmark&& other) = default;
  SingleBenchmark(const SingleBenchmark& other) = delete;
  SingleBenchmark& operator=(const SingleBenchmark& other) = delete;
  SingleBenchmark& operator=(SingleBenchmark&& other) = delete;

  /** Main run method which executes the benchmark. `setup()` should be called before this. */
  bool run() override;

  /**
   * Generates the data needed for the benchmark.
   * This is probably the first method to be called so that a virtual
   * address space is available to generate the IO addresses.
   */
  void create_data_files() override;

  /** Create all the IO addresses ahead of time to avoid unnecessary ops during the actual benchmark. */
  void set_up() override;

  /** Clean up after te benchmark */
  void tear_down(bool force) override;

  /** Return the results as a JSON to be exported to the user and visualization. */
  nlohmann::json get_result_as_json() override;

  ~SingleBenchmark() { SingleBenchmark::tear_down(false); }
};

}  // namespace perma
