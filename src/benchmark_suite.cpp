#include "benchmark_suite.hpp"

#include <iostream>
#include <json.hpp>

#include "benchmark_factory.hpp"

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::string& file_name) {
  benchmarks_ = BenchmarkFactory::create_benchmarks(file_name);
  for (std::unique_ptr<Benchmark>& benchmark : benchmarks_) {
    benchmark->generate_data();
    benchmark->set_up();
    benchmark->run();
    // TODO: Handle get_result
    nlohmann::json result = benchmark->get_result();
    std::cout << result.dump() << std::endl;
    benchmark->tear_down();
  }
}

}  // namespace perma
