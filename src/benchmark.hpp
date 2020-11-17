#pragma once
#include <libpmem.h>
#include <search.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <json.hpp>
#include <map>
#include <utility>
#include <vector>

#include "io_operation.hpp"
#include "utils.hpp"

namespace perma {

namespace internal {

static const size_t BYTE_IN_GIGABYTE = 1e9;
static const size_t NANOSECONDS_IN_SECONDS = 1e9;

struct Measurement {
  Measurement(const std::chrono::high_resolution_clock::time_point start_ts,
              const std::chrono::high_resolution_clock::time_point end_ts)
      : start_ts_(start_ts), end_ts_(end_ts){};
  const std::chrono::high_resolution_clock::time_point start_ts_;
  const std::chrono::high_resolution_clock::time_point end_ts_;
};

}  // namespace internal

struct BenchmarkConfig {
  // This field is required and has no default value,
  // i.e., it must be set as a command line argument.
  std::string pmem_directory{};

  uint64_t total_memory_range = 10'737'418'240;  // 10 GiB
  uint32_t access_size = 256;
  uint64_t number_operations = 10'000'000;
  internal::Mode exec_mode{internal::Mode::Sequential};

  internal::RandomDistribution random_distribution{internal::RandomDistribution::Uniform};
  // TODO: re-evaluate this value for real world access patterns
  double zipf_alpha = 0.99;

  internal::DataInstruction data_instruction{internal::DataInstruction::SIMD};
  internal::PersistInstruction persist_instruction{internal::PersistInstruction::NTSTORE};

  double write_ratio = 0.5;
  double read_ratio = 0.5;

  uint64_t pause_frequency = 0;
  uint64_t pause_length_micros = 1000;  // 1 ms

  uint16_t number_partitions = 1;

  uint16_t number_threads = 1;

  static BenchmarkConfig decode(YAML::Node& raw_config_data);
  void validate() const;
};

class Benchmark {
 public:
  Benchmark(std::string benchmark_name, BenchmarkConfig config)
      : benchmark_name_(std::move(benchmark_name)),
        config_(std::move(config)),
        pmem_file_{generate_random_file_name(config_.pmem_directory)},
        owns_pmem_file_{true} {};

  Benchmark(std::string benchmark_name, BenchmarkConfig config, std::filesystem::path pmem_file)
      : benchmark_name_(std::move(benchmark_name)),
        config_(std::move(config)),
        pmem_file_{std::move(pmem_file)},
        owns_pmem_file_{false} {};

  /** Main run method which executes the benchmark. `setup()` should be called before this. */
  void run();

  /**
   * Generates the data needed for the benchmark.
   * This is probably the first method to be called so that a virtual
   * address space is available to generate the IO addresses.
   */
  void generate_data();

  /** Create all the IO addresses ahead of time to avoid unnecessary ops during the actual benchmark. */
  void set_up();

  /** Clean up after te benchmark */
  void tear_down(bool force = false);

  /** Return the results as a JSON to be exported to the user and visualization. */
  nlohmann::json get_result();

  const std::string& benchmark_name() const;

  ~Benchmark() { tear_down(); }

 private:
  nlohmann::json get_config();
  void run_in_thread(uint16_t thread_id);

  const BenchmarkConfig config_;
  const std::string benchmark_name_;
  const std::filesystem::path pmem_file_;
  const bool owns_pmem_file_;
  char* pmem_data_{nullptr};
  std::vector<std::vector<std::unique_ptr<IoOperation>>> io_operations_;
  std::vector<std::vector<internal::Measurement>> measurements_;
  std::vector<std::thread> pool_;
};

}  // namespace perma
