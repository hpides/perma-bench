#pragma once

#include <hdr_histogram.h>
#include <libpmem.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <json.hpp>
#include <map>
#include <random>
#include <utility>
#include <vector>

#include "io_operation.hpp"
#include "utils.hpp"

namespace perma {

namespace internal {

static const size_t BYTE_IN_GIGABYTE = 1e9;
static const size_t NANOSECONDS_IN_SECONDS = 1e9;

struct Latency {
  Latency() : data{-1u} {}
  Latency(const uint64_t latency, const internal::OpType op_type) : duration{latency}, op_type{op_type} {};

  union {
    const uint64_t data;
    struct {
      const uint64_t duration : 62;
      const internal::OpType op_type : 2;
    };
  };
};

struct alignas(16) Measurement {
  Measurement(const std::chrono::high_resolution_clock::time_point start_ts, const Latency latency)
      : start_ts(start_ts), latency(latency){};
  const std::chrono::high_resolution_clock::time_point start_ts;
  const Latency latency;
};

}  // namespace internal

struct BenchmarkConfig {
  // This field is required and has no default value,
  // i.e., it must be set as a command line argument.
  std::string pmem_directory{};
  std::vector<std::string> matrix_args{};

  // The values below here actually define the benchmark and are not just used for meta-information.
  uint64_t total_memory_range = 10'737'418'240;  // 10 GiB
  uint32_t access_size = 256;
  uint64_t number_operations = 10'000'000;
  internal::Mode exec_mode{internal::Mode::Sequential};

  internal::RandomDistribution random_distribution{internal::RandomDistribution::Uniform};
  // TODO: re-evaluate this value for real world access patterns
  double zipf_alpha = 0.99;

  internal::DataInstruction data_instruction{internal::DataInstruction::SIMD};
  internal::PersistInstruction persist_instruction{internal::PersistInstruction::NoCache};

  double write_ratio = 0.0;

  uint64_t pause_frequency = 0;
  uint64_t pause_length_micros = 1000;  // 1 ms

  uint16_t number_partitions = 1;

  uint16_t number_threads = 1;

  bool raw_results = false;

  bool prefault_file = true;

  static BenchmarkConfig decode(YAML::Node& raw_config_data);
  void validate() const;
};

struct ThreadRunConfig {
  char* partition_start_addr;
  const size_t partition_size;
  const size_t num_threads_per_partition;
  const size_t thread_num;
  const size_t num_ops;
  std::vector<internal::Measurement>* raw_measurements;
  std::vector<internal::Latency>* latencies;
  const BenchmarkConfig& config;

  ThreadRunConfig(char* partition_start_addr, const size_t partition_size, const size_t num_threads_per_partition,
                  const size_t thread_num, const size_t num_ops, std::vector<internal::Measurement>* raw_measurements,
                  std::vector<internal::Latency>* latencies, const BenchmarkConfig& config)
      : partition_start_addr(partition_start_addr),
        partition_size(partition_size),
        num_threads_per_partition(num_threads_per_partition),
        thread_num(thread_num),
        num_ops(num_ops),
        raw_measurements(raw_measurements),
        latencies(latencies),
        config(config) {}
};

struct BenchmarkResult {
  explicit BenchmarkResult(const BenchmarkConfig& config);
  ~BenchmarkResult();

  nlohmann::json get_result_as_json() const;

  std::vector<std::vector<internal::Measurement>> raw_measurements;
  std::vector<std::vector<internal::Latency>> latencies;
  hdr_histogram* latency_hdr = nullptr;
  const BenchmarkConfig config;
};

class Benchmark {
 public:
  Benchmark(std::string benchmark_name, const BenchmarkConfig& config);
  Benchmark(std::string benchmark_name, const BenchmarkConfig& config, std::filesystem::path pmem_file);

  Benchmark(Benchmark&& other) = default;
  Benchmark(const Benchmark& other) = delete;
  Benchmark& operator=(const Benchmark& other) = delete;
  Benchmark& operator=(Benchmark&& other) = delete;

  /** Main run method which executes the benchmark. `setup()` should be called before this. */
  void run();

  /**
   * Generates the data needed for the benchmark.
   * This is probably the first method to be called so that a virtual
   * address space is available to generate the IO addresses.
   */
  void create_data_file();

  /** Create all the IO addresses ahead of time to avoid unnecessary ops during the actual benchmark. */
  void set_up();

  /** Clean up after te benchmark */
  void tear_down(bool force = false);

  /** Return the results as a JSON to be exported to the user and visualization. */
  nlohmann::json get_result_as_json();

  const std::string& benchmark_name() const;
  const BenchmarkConfig& get_benchmark_config() const;
  const std::filesystem::path& get_pmem_file() const;
  const char* get_pmem_data() const;
  const std::vector<ThreadRunConfig>& get_thread_configs() const;
  const BenchmarkResult& get_benchmark_result() const;
  bool owns_pmem_file() const;

  ~Benchmark() { tear_down(); }

 private:
  nlohmann::json get_json_config();
  void run_in_thread(ThreadRunConfig& thread_config);

  const std::string benchmark_name_;
  const std::filesystem::path pmem_file_;
  bool owns_pmem_file_;
  char* pmem_data_{nullptr};

  const BenchmarkConfig config_;
  std::unique_ptr<BenchmarkResult> result_;
  std::vector<ThreadRunConfig> thread_configs_;
  std::vector<std::thread> pool_;
};

}  // namespace perma
