#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace perma {

namespace internal {

enum BenchmarkOptions { InvalidBenchmark, readBenchmark };

static const std::map<std::string, BenchmarkOptions> optionStrings{{"read_benchmark", BenchmarkOptions::readBenchmark}};

BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option);
}  // namespace internal

class Benchmark {
 public:
  void run();
  void generate_data();
  virtual void get_result() = 0;
  virtual void set_up() = 0;
  virtual void tear_down() = 0;

 protected:
  virtual size_t get_length() = 0;
  char* pmem_file_;
  std::vector<std::unique_ptr<IoOperation>> io_operations_;
};

}  // namespace perma
