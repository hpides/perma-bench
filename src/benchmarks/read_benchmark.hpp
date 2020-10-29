#pragma once

#include "../benchmark.hpp"

namespace perma {

struct ReadBenchmarkConfig {
  internal::Mode exec_mode_{internal::Mode::Sequential};
  uint32_t access_size_{512};
  uint32_t target_size_{1024};
  uint32_t number_operations_{10000};
  uint32_t pause_frequency_{1000};
  uint32_t pause_length_{1000};
  uint16_t number_threads_{1};

  static ReadBenchmarkConfig decode(const YAML::Node& raw_config_data);
};

class ReadBenchmark : public Benchmark {
 public:
  explicit ReadBenchmark(const ReadBenchmarkConfig& config) : Benchmark("readBenchmark"), config_(config){};

  void set_up() override;

 protected:
  nlohmann::json get_config() override;
  size_t get_length() override;
  uint16_t get_number_threads() override;

 private:
  ReadBenchmarkConfig config_;
};
}  // namespace perma
