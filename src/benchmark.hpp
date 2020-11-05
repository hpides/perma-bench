#pragma once
#include <libpmem.h>
#include <search.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <json.hpp>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace perma {

namespace internal {

static const size_t BYTE_IN_MEBIBYTE = 1024ul * 1024;
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
  uint64_t total_memory_range = 10240;  // 10 GiB
  uint32_t access_size = 256;
  uint64_t number_operations = 10'000'000;
  internal::Mode exec_mode{internal::Mode::Sequential};

  internal::DataInstruction data_instruction{internal::DataInstruction::SIMD};
  internal::PersistInstruction persist_instruction{internal::PersistInstruction::NTSTORE};

  double write_ratio = 0.5;
  double read_ratio = 0.5;

  uint64_t pause_frequency = 0;
  uint64_t pause_length_micros = 1000;  // 1 ms

  uint16_t number_partitions = 1;

  uint16_t number_threads = 1;

  static BenchmarkConfig decode(const YAML::Node& raw_config_data);
};

class Benchmark {
 public:
  Benchmark(std::string benchmark_name, BenchmarkConfig config)
      : benchmark_name_(std::move(benchmark_name)), config_(config){};
  void run();
  void generate_data();
  nlohmann::json get_result();
  void set_up();
  void tear_down() {
    if (pmem_file_ != nullptr) {
      pmem_unmap(pmem_file_, get_length_in_bytes());
    }
    std::filesystem::remove("/mnt/nvram-nvmbm/read_benchmark.file");
  }

  std::vector<std::vector<std::unique_ptr<IoOperation>>> io_operations_;
  std::vector<std::vector<internal::Measurement>> measurements_;

 protected:
  size_t get_length_in_bytes() const;
  nlohmann::json get_config();

  const BenchmarkConfig config_;
  const std::string benchmark_name_;
  char* pmem_file_{nullptr};
  std::vector<std::thread> pool_;
};

}  // namespace perma
