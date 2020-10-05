#pragma once

#include <libpmem.h>

#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace perma {

namespace internal {

enum BenchmarkOptions { InvalidBenchmark, readBenchmark, writeBenchmark };

static const std::map<std::string, BenchmarkOptions> optionStrings{
    {"read_benchmark", BenchmarkOptions::readBenchmark}, {"write_benchmark", BenchmarkOptions::writeBenchmark}};

BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option);

struct Measurement {
  const std::chrono::high_resolution_clock::time_point start_ts;
  const std::chrono::high_resolution_clock::time_point end_ts;
};

}  // namespace internal

class Benchmark {
 public:
  void run();
  void generate_data();
  virtual void get_result() = 0;
  virtual void set_up() = 0;
  virtual void tear_down() {
    if (pmem_file_ != nullptr) {
      pmem_unmap(pmem_file_, get_length());
    }
    bool result = std::filesystem::remove("/mnt/nvram-nvmbm/read_benchmark.file");
  }

 protected:
  virtual size_t get_length() = 0;
  char* pmem_file_{nullptr};
  std::vector<std::unique_ptr<IoOperation>> io_operations_;
  std::vector<internal::Measurement> measurements_;
};

}  // namespace perma
