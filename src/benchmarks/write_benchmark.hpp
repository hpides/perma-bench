#pragma once

#include <yaml-cpp/yaml.h>

#include "../benchmark.hpp"

namespace perma {
struct WriteBenchmarkConfig {
  uint32_t access_size_{512};
  uint32_t target_size_{1024};
  uint32_t number_operations_{10000};
  internal::Mode exec_mode_{internal::Mode::Sequential};
  uint32_t pause_frequency_{1000};
  uint32_t pause_length_{1000};

  static WriteBenchmarkConfig decode(const YAML::Node& raw_config_data);
};
class WriteBenchmark : public Benchmark {
 public:
  explicit WriteBenchmark(const WriteBenchmarkConfig& config) : config_(config){};

  void get_result() override;
  void set_up() override;

 protected:
  size_t get_length() override;

 private:
  WriteBenchmarkConfig config_;
};

}  // namespace perma
