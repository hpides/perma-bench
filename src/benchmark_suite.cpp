#include "benchmark_suite.hpp"

#include "benchmark_factory.hpp"

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::string& file_name) {
  benchmarks_ = BenchmarkFactory::create_benchmarks(file_name);
  for (std::unique_ptr<Benchmark>& benchmark : benchmarks_) {
    benchmark->generate_data();
    benchmark->set_up();
    benchmark->run();
    // TODO: Handle get_result
    benchmark->tear_down();
    benchmark->get_result();
  }
}

}  // namespace perma
