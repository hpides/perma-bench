#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <string>

#include "benchmark.hpp"
#include "benchmarks/bm0.hpp"

namespace nvmbm {

std::vector<std::unique_ptr<Benchmark>> BenchmarkFactory::create_benchmarks(
    const std::string& file_name) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  try {
    YAML::Node config = YAML::LoadFile(file_name);
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      switch (internal::resolveBenchmarkOption(it->first.as<std::string>())) {
        case internal::bm0: {
          auto bm = std::make_unique<bm0>(it->second);
          // Where to run set up method?
          benchmarks.push_back(std::move(bm));
          break;
        }
        case internal::bm1: {
          // Run specific benchmark factory
          // Add to benchmarks
          break;
        }
        case internal::InvalidBenchmark: {
          std::cerr << "Benchmark " << it->first.as<std::string>()
                    << " not implemented." << std::endl;
          break;
        }
      }
    }
  } catch (const YAML::ParserException& e1) {
    std::cerr << "Exception during config parsing: " << e1.msg << std::endl;
  } catch (const YAML::BadFile& e2) {
    std::cerr << "Exception during config parsing: " << e2.msg << std::endl;
  }
  return benchmarks;
}

}  // namespace nvmbm
