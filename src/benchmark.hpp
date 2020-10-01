#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace perma {

namespace internal {

enum BenchmarkOptions { InvalidBenchmark, readBenchmark };

static const std::map<std::string, BenchmarkOptions> optionStrings{{"read_benchmark", BenchmarkOptions::readBenchmark}};

BenchmarkOptions resolveBenchmarkOption(const std::string& benchmark_option);
}  // namespace internal

class Benchmark {
 public:
  void run();
  void generateData();
  virtual void getResult() { std::cerr << "getResult not implemented for super class Benchmark" << std::endl; };
  virtual void SetUp() { std::cerr << "SetUp not implemented for super class Benchmark" << std::endl; };
  virtual void TearDown() { std::cerr << "TearDown not implemented for super class Benchmark" << std::endl; };

 protected:
  virtual size_t getLength() { return 0; };
  char* pmem_file_;
  std::vector<std::unique_ptr<IoOperation>> io_operations_;
};

}  // namespace perma
