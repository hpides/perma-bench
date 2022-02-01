#pragma once

#include <hdr_histogram.h>

#include <filesystem>
#include <json.hpp>
#include <map>
#include <random>
#include <utility>
#include <vector>

#include "benchmark_config.hpp"
#include "io_operation.hpp"
#include "utils.hpp"

namespace perma {

struct MemoryRegion {
  const std::filesystem::path pmem_file;
  const bool owns_pmem_file;
  const bool is_dram;

  MemoryRegion(std::filesystem::path pmem_file, const bool owns_pmem_file, const bool is_dram)
      : pmem_file{std::move(pmem_file)}, owns_pmem_file{owns_pmem_file}, is_dram{is_dram} {};
};

enum BenchmarkType { Single, Parallel };

struct BenchmarkEnums {
  static const std::unordered_map<std::string, BenchmarkType> str_to_benchmark_type;
};

struct ThreadRunConfig {
  char* partition_start_addr;
  char* dram_partition_start_addr;
  const size_t partition_size;
  const size_t dram_partition_size;
  const size_t num_threads_per_partition;
  const size_t thread_num;
  const size_t num_ops;
  const BenchmarkConfig& config;

  // Pointers to store performance data in.
  uint64_t* total_operation_size;
  uint64_t* total_operation_duration;
  uint64_t* custom_op_duration;
  std::vector<uint64_t>* custom_op_latencies;

  ThreadRunConfig(char* partition_start_addr, char* dram_partition_start_addr, const size_t partition_size,
                  const size_t dram_partition_size, const size_t num_threads_per_partition, const size_t thread_num,
                  const size_t num_ops, const BenchmarkConfig& config, uint64_t* total_operation_duration,
                  uint64_t* total_operation_size, uint64_t* custom_op_duration,
                  std::vector<uint64_t>* custom_op_latencies)
      : partition_start_addr{partition_start_addr},
        dram_partition_start_addr{dram_partition_start_addr},
        partition_size{partition_size},
        dram_partition_size{dram_partition_size},
        num_threads_per_partition{num_threads_per_partition},
        thread_num{thread_num},
        num_ops{num_ops},
        config{config},
        total_operation_duration{total_operation_duration},
        total_operation_size{total_operation_size},
        custom_op_duration{custom_op_duration},
        custom_op_latencies{custom_op_latencies} {}
};

struct BenchmarkResult {
  explicit BenchmarkResult(BenchmarkConfig config);
  ~BenchmarkResult();

  nlohmann::json get_result_as_json() const;
  nlohmann::json get_custom_results_as_json() const;

  // Result vectors for raw operation workloads
  std::vector<uint64_t> total_operation_sizes;
  std::vector<uint64_t> total_operation_durations;

  // Result vectors for custom operation workloads
  std::vector<uint64_t> custom_operation_durations;
  std::vector<std::vector<uint64_t>> custom_operation_latencies;

  hdr_histogram* latency_hdr = nullptr;
  const BenchmarkConfig config;
};

class Benchmark {
 public:
  Benchmark(std::string benchmark_name, BenchmarkType benchmark_type, std::vector<MemoryRegion> memory_regions,
            std::vector<BenchmarkConfig> configs, std::vector<std::unique_ptr<BenchmarkResult>> results)
      : benchmark_name_{std::move(benchmark_name)},
        benchmark_type_{benchmark_type},
        memory_regions_{std::move(memory_regions)},
        configs_{std::move(configs)},
        results_{std::move(results)} {}

  Benchmark(Benchmark&& other) = default;
  Benchmark(const Benchmark& other) = delete;
  Benchmark& operator=(const Benchmark& other) = delete;
  Benchmark& operator=(Benchmark&& other) = delete;

  /** Main run method which executes the benchmark. `setup()` should be called before this.
   *  Return true if benchmark ran successfully, false if an error was encountered.
   */
  virtual bool run() = 0;

  /**
   * Generates the data needed for the benchmark.
   * This is probably the first method to be called so that a virtual
   * address space is available to generate the IO addresses.
   */
  virtual void create_data_files() = 0;

  /** Create all the IO addresses ahead of time to avoid unnecessary ops during the actual benchmark. */
  virtual void set_up() = 0;

  /** Clean up after te benchmark */
  virtual void tear_down(bool force) = 0;

  /** Return the results as a JSON to be exported to the user and visualization. */
  virtual nlohmann::json get_result_as_json() = 0;

  /** Return the name of the benchmark. */
  const std::string& benchmark_name() const;

  /** Return the type of the benchmark. */
  std::string benchmark_type_as_str() const;
  BenchmarkType get_benchmark_type() const;

  const std::filesystem::path& get_pmem_file(uint8_t index) const;
  bool owns_pmem_file(uint8_t index) const;

  const std::vector<char*>& get_pmem_data() const;
  const std::vector<char*>& get_dram_data() const;

  const std::vector<BenchmarkConfig>& get_benchmark_configs() const;
  const std::vector<std::vector<ThreadRunConfig>>& get_thread_configs() const;
  const std::vector<std::unique_ptr<BenchmarkResult>>& get_benchmark_results() const;

 protected:
  nlohmann::json get_json_config(uint8_t config_index);
  static void single_set_up(const BenchmarkConfig& config, char* pmem_data, char* dram_data, BenchmarkResult* result,
                            std::vector<std::thread>* pool, std::vector<ThreadRunConfig>* thread_config);

  static char* create_pmem_data_file(const BenchmarkConfig& config, const MemoryRegion& memory_region);
  static char* create_dram_data(const BenchmarkConfig& config);
  static void prepare_data_file(char* file_data, const BenchmarkConfig& config, uint64_t memory_range,
                                uint64_t page_size);

  static void run_custom_ops_in_thread(ThreadRunConfig* thread_config, const BenchmarkConfig& config);
  static void run_in_thread(ThreadRunConfig* thread_config, const BenchmarkConfig& config);

  static nlohmann::json get_benchmark_config_as_json(const BenchmarkConfig& bm_config);

  const std::string benchmark_name_;
  const BenchmarkType benchmark_type_;

  std::vector<MemoryRegion> memory_regions_;
  std::vector<char*> pmem_data_;
  std::vector<char*> dram_data_;

  const std::vector<BenchmarkConfig> configs_;
  std::vector<std::unique_ptr<BenchmarkResult>> results_;
  std::vector<std::vector<ThreadRunConfig>> thread_configs_;
  std::vector<std::vector<std::thread>> pools_;
};

}  // namespace perma
