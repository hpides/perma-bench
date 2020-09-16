#include "benchmark_suite.hpp"

#include "benchmark_factory.hpp"

namespace perma {

void BenchmarkSuite::start_benchmarks(const std::string& file_name) {
  prepare_benchmarks(file_name);
  run_benchmarks();
}

void BenchmarkSuite::prepare_benchmarks(const std::string& file_name) {
  benchmarks_ = BenchmarkFactory::create_benchmarks(file_name);
  for (std::unique_ptr<Benchmark>& benchmark : benchmarks_) {
    benchmark->SetUp();
  }
}

void BenchmarkSuite::run_benchmarks() {
  for (std::unique_ptr<Benchmark>& benchmark : benchmarks_) {
    benchmark->run();
  }
}

}  // namespace perma
