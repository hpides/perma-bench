#include "benchmark_suite.hpp"

using namespace perma;

int main() {
  BenchmarkSuite bm_suite{};
  bm_suite.start_benchmarks("/mnt/nvram-nvmbm/benchmark_config.yaml");

  return 0;
}
