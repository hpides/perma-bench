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

static const size_t BYTE_IN_MEBIBYTE = pow(1024, 2);
static const size_t BYTE_IN_GIGABYTE = 1e9;
static const size_t NANOSECONDS_IN_SECONDS = 1e9;

struct Measurement {
  Measurement(const std::chrono::high_resolution_clock::time_point start_ts,
              const std::chrono::high_resolution_clock::time_point end_ts)
      : start_ts_(start_ts), end_ts_(end_ts){};
  const std::chrono::high_resolution_clock::time_point start_ts_;
  const std::chrono::high_resolution_clock::time_point end_ts_;
};

template <typename T>
static void get_if_present(const YAML::Node& data, const std::string& name, T* attribute) {
  if (data[name] != nullptr) {
    *attribute = data[name].as<T>();
  }
}

}  // namespace internal

struct BenchmarkConfig {
  uint64_t total_memory_range_{1024};
  uint32_t access_size_{512};
  uint64_t number_operations_{10000};
  internal::Mode exec_mode_{internal::Mode::Sequential};

  double write_ratio_{0.5};
  double read_ratio_{0.5};

  uint64_t pause_frequency_{1000};
  uint64_t pause_length_micros_{1000};

  uint16_t number_partitions_{1};

  uint16_t number_threads_{1};

  static BenchmarkConfig decode(const YAML::Node& raw_config_data);
};

class Benchmark {
 public:
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
  Benchmark(std::string benchmark_name, BenchmarkConfig config)
      : benchmark_name_(std::move(benchmark_name)), config_(config){};
  size_t get_length_in_bytes();
  nlohmann::json get_config();

  const BenchmarkConfig config_;
  const std::string benchmark_name_;
  char* pmem_file_{nullptr};
  std::vector<std::thread> pool_;
};

}  // namespace perma
