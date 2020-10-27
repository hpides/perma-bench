#pragma once

#include <yaml-cpp/yaml.h>

#include "../benchmark.hpp"

namespace perma {
struct WriteBenchmarkConfig {
  internal::Mode exec_mode_{internal::Mode::Sequential};
  uint32_t access_size_{512};
  uint32_t target_size_{1024};
  uint32_t number_operations_{10000};
  uint32_t pause_frequency_{1000};
  uint32_t pause_length_{1000};
  uint16_t number_threads_{1};

  static WriteBenchmarkConfig decode(const YAML::Node& raw_config_data);
};
class WriteBenchmark : public Benchmark {
 public:
  explicit WriteBenchmark(const WriteBenchmarkConfig& config) : Benchmark("writeBenchmark"), config_(config){};

  void set_up() override;

 protected:
  nlohmann::json get_config() override;
  size_t get_length() override;

 private:
  WriteBenchmarkConfig config_;
};

}  // namespace perma
