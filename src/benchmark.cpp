#include "benchmark.hpp"

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
  measurements_.reserve(io_operations_.size());

  for (std::unique_ptr<IoOperation>& io_op : io_operations_) {
    const auto start_ts = std::chrono::high_resolution_clock::now();
    io_op->run();
    const auto end_ts = std::chrono::high_resolution_clock::now();
    measurements_.push_back(internal::Measurement{start_ts, end_ts});
  }
}

void Benchmark::generate_data() {
  size_t length = get_length();
  pmem_file_ = create_pmem_file("/mnt/nvram-nvmbm/read_benchmark.file", length);
  write_data(pmem_file_, pmem_file_ + length);
}
}  // namespace perma
