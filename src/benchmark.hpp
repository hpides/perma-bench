#pragma once
#include <libpmem.h>
#include <search.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <json.hpp>
#include <map>
#include <vector>

#include "io_operation.hpp"
#include "thread_manager.hpp"

namespace perma {

namespace internal {

enum BenchmarkOptions { invalidBenchmark, readBenchmark, writeBenchmark };

static const std::map<std::string, BenchmarkOptions> optionStrings{
    {"read_benchmark", BenchmarkOptions::readBenchmark}, {"write_benchmark", BenchmarkOptions::writeBenchmark}};

BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option);

template <typename T>
static void get_if_present(const YAML::Node& data, const std::string& name, T* attribute) {
  if (data[name] != nullptr) {
    *attribute = data[name].as<T>();
  }
}

static const size_t BYTE_IN_MEBIBYTE = pow(1024, 2);
static const size_t BYTE_IN_GIGABYTE = 1e9;
static const size_t NANOSECONDS_IN_SECONDS = 1e9;

}  // namespace internal

class Benchmark {
 public:
  void run();
  void generate_data();
  virtual nlohmann::json get_result();
  virtual void set_up() = 0;
  virtual void tear_down() {
    if (pmem_file_ != nullptr) {
      pmem_unmap(pmem_file_, get_length());
    }
    std::filesystem::remove("/mnt/nvram-nvmbm/read_benchmark.file");
  }

  std::vector<std::vector<std::unique_ptr<IoOperation>>> io_operations_;

 protected:
  explicit Benchmark(std::string benchmark_name) : benchmark_name_(std::move(benchmark_name)) {}
  virtual size_t get_length() = 0;
  virtual nlohmann::json get_config() = 0;
  virtual uint16_t get_number_threads() = 0;

  const std::string benchmark_name_;
  char* pmem_file_{nullptr};
  std::vector<std::thread> pool_;
  std::vector<std::vector<internal::Measurement>> measurements_;
};

}  // namespace perma
