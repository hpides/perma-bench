#include "benchmark.hpp"

#include <utility>

namespace nvmbm {

Benchmark::Benchmark(std::vector<IoOperation*> io_operations)
    : io_operations_(std::move(io_operations)) {}

void Benchmark::run() {
  for (IoOperation* io_op : io_operations_) {
    io_op->run();
  }
}
}  // namespace nvmbm
