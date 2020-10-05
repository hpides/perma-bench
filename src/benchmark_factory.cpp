#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <string>

#include "benchmark.hpp"
#include "benchmarks/read_benchmark.hpp"
#include "benchmarks/write_benchmark.hpp"

namespace perma {

std::vector<std::unique_ptr<Benchmark>> BenchmarkFactory::create_benchmarks(const std::string& file_name) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  try {
    const YAML::Node& config = YAML::LoadFile(file_name);
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      switch (internal::resolve_benchmark_option(it->first.as<std::string>())) {
        case internal::readBenchmark: {
          ReadBenchmarkConfig read_benchmark_config = ReadBenchmarkConfig::decode(it->second);
          benchmarks.push_back(std::make_unique<ReadBenchmark>(read_benchmark_config));
          break;
        }
        case internal::writeBenchmark: {
          WriteBenchmarkConfig write_benchmark_config = WriteBenchmarkConfig::decode(it->second);
          benchmarks.push_back(std::make_unique<WriteBenchmark>(write_benchmark_config));
          break;
        }
        case internal::invalidBenchmark: {
          throw std::runtime_error{"Benchmark " + it->first.as<std::string>() + " is not implemented."};
        }
      }
    }
  } catch (const YAML::ParserException& e1) {
    throw std::runtime_error{"Exception during config parsing: " + e1.msg};
  } catch (const YAML::BadFile& e2) {
    throw std::runtime_error{"Exception during config parsing: " + e2.msg};
  }
  return benchmarks;
}

}  // namespace perma
