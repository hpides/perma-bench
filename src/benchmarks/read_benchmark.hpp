#pragma once

#include <yaml-cpp/yaml.h>

#include "../benchmark.hpp"

namespace nvmbm {

struct ReadBenchmarkConfig {
  unsigned int access_size_{512};
  unsigned int target_size_{1024};
  unsigned int number_operations_{10000};
  internal::Mode exec_mode_{internal::Mode::Sequential};
  unsigned int pause_frequency_{1000};
  unsigned int pause_length_{1000};

  static ReadBenchmarkConfig decode(const YAML::Node& init_data);

  template <typename T>
  static void getIfPresent(const std::string& name, const YAML::Node& data,
                           T& attribute) {
    if (data[name] != nullptr) {
      attribute = data[name].as<T>();
    }
  };
};

class ReadBenchmark : public Benchmark {
 public:
  explicit ReadBenchmark(const ReadBenchmarkConfig& config) : Benchmark(), config_(config) {};

  void getResult() override;
  void SetUp() override;
  void TearDown() override;

 private:
  ReadBenchmarkConfig config_;
};
}  // namespace nvmbm
