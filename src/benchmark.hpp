#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace nvmbm {
namespace internal {
enum BenchmarkOptions { InvalidBenchmark, bm0, bm1 };

static const std::map<std::string, BenchmarkOptions> optionStrings{
    {"bm0", BenchmarkOptions::bm0}, {"bm1", BenchmarkOptions::bm1}};

enum Mode { Sequential, Random };
BenchmarkOptions resolveBenchmarkOption(const std::string& benchmark_option);
}  // namespace internal
class Benchmark {
 public:
  void run();
  virtual void getResult() {
    std::cerr << "getResult not implemented for super class Benchmark"
              << std::endl;
  };
  virtual void SetUp() {
    std::cerr << "SetUp not implemented for super class Benchmark" << std::endl;
  };
  virtual void TearDown() {
    std::cerr << "TearDown not implemented for super class Benchmark"
              << std::endl;
  };

 protected:
  std::vector<IoOperation*> io_operations_;
};

}  // namespace nvmbm
