#pragma once

#include <yaml-cpp/yaml.h>

#include "../benchmark.hpp"

namespace nvmbm {

struct ReadBenchmarkConfig {
  uint32_t access_size_{512};
  uint32_t target_size_{1024};
  uint32_t number_operations_{10000};
  internal::Mode exec_mode_{internal::Mode::Sequential};
  uint32_t pause_frequency_{1000};
  uint32_t pause_length_{1000};

  static ReadBenchmarkConfig decode(const YAML::Node& init_data);

  template <typename T>
  static void getIfPresent(const std::string& name, const YAML::Node& data, T& attribute) {
    if (data[name] != nullptr) {
      attribute = data[name].as<T>();
    }
  };
};

class ReadBenchmark : public Benchmark {
 public:
  explicit ReadBenchmark(const ReadBenchmarkConfig& config) : Benchmark(), config_(config){};

  void getResult() override;
  void SetUp() override;
  void TearDown() override;

 private:
  ReadBenchmarkConfig config_;
};
}  // namespace nvmbm
