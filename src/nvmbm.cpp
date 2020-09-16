#include "benchmark_suite.hpp"

using namespace nvmbm;

int main() {
  BenchmarkSuite bm_suite{};
  bm_suite.start_benchmarks("/hpi/fs00/home/leon.papke/nvmbm/benchmark_config.yaml");

  return 0;
}
