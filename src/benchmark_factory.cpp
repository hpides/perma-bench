#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <string>

#include "benchmark.hpp"

namespace perma {

std::vector<std::unique_ptr<Benchmark>> BenchmarkFactory::create_benchmarks(const std::string& file_name) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  try {
    const YAML::Node& config = YAML::LoadFile(file_name);
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      BenchmarkConfig bm_config = BenchmarkConfig::decode(it->second);
      benchmarks.push_back(std::make_unique<Benchmark>(it->first.as<std::string>(), bm_config));
    }
  } catch (const YAML::ParserException& e1) {
    throw std::runtime_error{"Exception during config parsing: " + e1.msg};
  } catch (const YAML::BadFile& e2) {
    throw std::runtime_error{"Exception during config parsing: " + e2.msg};
  }
  return benchmarks;
}

}  // namespace perma
