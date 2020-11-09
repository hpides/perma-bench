#include "benchmark_suite.hpp"


using namespace perma;

constexpr auto DEFAULT_CONFIG_PATH = "configs/dev-config.yaml";

int main(int argc, char** argv) {
  std::filesystem::path config_file = std::filesystem::current_path() / ".." / DEFAULT_CONFIG_PATH;
  if (argc == 2) {
    config_file = argv[1];
  }
  BenchmarkSuite bm_suite{};
  bm_suite.run_benchmarks(config_file);

  return 0;
}
