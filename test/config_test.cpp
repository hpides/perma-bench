#include <yaml-cpp/yaml.h>

#include "benchmark.hpp"
#include "gtest/gtest.h"
#include "io_operation.hpp"

namespace perma {

constexpr auto TEST_CONFIG_FILE_SEQ = "test_seq.yaml";
constexpr auto TEST_CONFIG_FILE_RANDOM = "test_random.yaml";

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::filesystem::path test_config_path =
        std::filesystem::current_path().parent_path() / "resources/test_data/configs";
    config_file_seq = test_config_path / TEST_CONFIG_FILE_SEQ;
    config_file_random = test_config_path / TEST_CONFIG_FILE_RANDOM;
  }

  std::filesystem::path config_file_seq;
  std::filesystem::path config_file_random;
};

TEST_F(ConfigTest, DecodeSequential) {
  YAML::Node config = YAML::LoadFile(config_file_seq);
  BenchmarkConfig bm_config = BenchmarkConfig::decode(config.begin()->second);
  EXPECT_EQ(bm_config.total_memory_range, 67108864);
  EXPECT_EQ(bm_config.access_size, 256);
  EXPECT_EQ(bm_config.number_operations, 10'000'000);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Sequential);

  EXPECT_EQ(bm_config.random_distribution, internal::RandomDistribution::Uniform);
  EXPECT_EQ(bm_config.zipf_alpha, 0.99);

  EXPECT_EQ(bm_config.data_instruction, internal::DataInstruction::SIMD);
  EXPECT_EQ(bm_config.persist_instruction, internal::PersistInstruction::NTSTORE);

  EXPECT_EQ(bm_config.write_ratio, 0.5);
  EXPECT_EQ(bm_config.read_ratio, 0.5);

  EXPECT_EQ(bm_config.pause_frequency, 1);
  EXPECT_EQ(bm_config.pause_length_micros, 10);

  EXPECT_EQ(bm_config.number_partitions, 1);
  EXPECT_EQ(bm_config.number_threads, 2);
}

TEST_F(ConfigTest, DecodeRandom) {
  YAML::Node config = YAML::LoadFile(config_file_random);
  BenchmarkConfig bm_config = BenchmarkConfig::decode(config.begin()->second);
  EXPECT_EQ(bm_config.total_memory_range, 10'737'418'240);
  EXPECT_EQ(bm_config.access_size, 256);
  EXPECT_EQ(bm_config.number_operations, 10'000'000);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Random);

  EXPECT_EQ(bm_config.random_distribution, internal::RandomDistribution::Zipf);
  EXPECT_EQ(bm_config.zipf_alpha, 0.9);

  EXPECT_EQ(bm_config.data_instruction, internal::DataInstruction::SIMD);
  EXPECT_EQ(bm_config.persist_instruction, internal::PersistInstruction::NTSTORE);

  EXPECT_EQ(bm_config.write_ratio, 0.5);
  EXPECT_EQ(bm_config.read_ratio, 0.5);

  EXPECT_EQ(bm_config.pause_frequency, 0);
  EXPECT_EQ(bm_config.pause_length_micros, 1000);

  EXPECT_EQ(bm_config.number_partitions, 1);
  EXPECT_EQ(bm_config.number_threads, 1);
}

}  // namespace perma
