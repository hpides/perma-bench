#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <string>

#include "benchmark.hpp"

namespace perma {

std::vector<std::unique_ptr<Benchmark>> BenchmarkFactory::create_benchmarks(const std::filesystem::path& pmem_directory,
                                                                            const std::filesystem::path& config_file) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  try {
    YAML::Node config = YAML::LoadFile(config_file);
    for (YAML::iterator it = config.begin(); it != config.end(); ++it) {
      BenchmarkConfig bm_config = BenchmarkConfig::decode(it->second);
      bm_config.pmem_directory = pmem_directory;
      benchmarks.push_back(std::make_unique<Benchmark>(it->first.as<std::string>(), std::move(bm_config)));
    }
  } catch (const YAML::ParserException& e1) {
    throw std::runtime_error{"Exception during config parsing: " + e1.msg};
  } catch (const YAML::BadFile& e2) {
    throw std::runtime_error{"Exception during config parsing: " + e2.msg};
  }
  return benchmarks;
}

}  // namespace perma
