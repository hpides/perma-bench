#pragma once

#include <yaml-cpp/yaml.h>

#include "../benchmark.hpp"

namespace nvmbm {
class bm0 : public Benchmark {
 public:
  bm0() = default;
  explicit bm0(const YAML::Node& init_data);

  void getResult() override;
  void SetUp() override;
  void TearDown() override;

 private:
  unsigned int access_size_{512};
  unsigned int target_size_{1024};
  unsigned int number_operations_{10000};
  internal::Mode exec_mode_{internal::Mode::Sequential};
  float read_ratio_{0.5};
  float write_ratio_{0.5};
  unsigned int pause_frequency_{1000};
  unsigned int pause_length_{1000};
};
}  // namespace nvmbm
