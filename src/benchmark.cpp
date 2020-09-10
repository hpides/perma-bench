#include "benchmark.hpp"

#include <map>

namespace perma {

namespace internal {
BenchmarkOptions resolveBenchmarkOption(const std::string& benchmark_option) {
  auto it = optionStrings.find(benchmark_option);
  if (it != optionStrings.end()) {
    return it->second;
  }
  return BenchmarkOptions::InvalidBenchmark;
}
}  // namespace internal

void Benchmark::run() {
  for (std::unique_ptr<IoOperation>& io_op : io_operations_) {
    io_op->run();
  }
}
}  // namespace perma
