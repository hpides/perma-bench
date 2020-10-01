#include "benchmark.hpp"

#include <immintrin.h>
#include <libpmem.h>

#include <map>

#include "utils.hpp"

namespace perma {

namespace internal {
BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option) {
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

void Benchmark::generate_data() {
  size_t length = get_length();
  pmem_file_ = create_pmem_file("/mnt/nvram-nvmbm/read_benchmark.file", length);
  write_data(pmem_file_, pmem_file_ + length);
}
}  // namespace perma
