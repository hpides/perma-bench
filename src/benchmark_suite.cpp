#include "benchmark_suite.hpp"

#include <iostream>
#include <json.hpp>

#include "benchmark_factory.hpp"

namespace perma {

void BenchmarkSuite::run_benchmarks(const std::filesystem::path& pmem_directory,
                                    const std::filesystem::path& config_file) {
  Benchmark* previous_bm = nullptr;
  std::vector<std::unique_ptr<Benchmark>> benchmarks = BenchmarkFactory::create_benchmarks(pmem_directory, config_file);

  std::cout << "Running benchmarks..." << std::endl;
  for (size_t i = 0; i < benchmarks.size(); ++i) {
    Benchmark* benchmark = benchmarks[i].get();
    if (previous_bm && previous_bm->benchmark_name() != benchmark->benchmark_name()) {
      // Started new benchmark, force delete old data in case it was a matrix.
      // If it is not a matrix, this does nothing.
      previous_bm->tear_down(/*force=*/true);
    }

    benchmark->generate_data();
    benchmark->set_up();
    benchmark->run();

    // TODO: Handle get_result
    nlohmann::json result = benchmark->get_result();
//        std::cout << result.dump() << std::endl;
    std::cout << result["results"].size() << std::endl;

    benchmark->tear_down();
    previous_bm = benchmark;

    std::cout << "Completed " << (i + 1) << "/" << benchmarks.size() << " benchmark(s).\n";
  }

  // Final clean up
  previous_bm->tear_down(/*force=*/true);
}

}  // namespace perma
