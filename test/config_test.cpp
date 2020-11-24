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
    const std::filesystem::path test_config_path = std::filesystem::current_path() / "resources/configs";
    config_file_seq = test_config_path / TEST_CONFIG_FILE_SEQ;
    config_file_random = test_config_path / TEST_CONFIG_FILE_RANDOM;
  }

  std::filesystem::path config_file_seq;
  std::filesystem::path config_file_random;
  BenchmarkConfig bm_config;
};

TEST_F(ConfigTest, DecodeSequential) {
  ASSERT_NO_THROW({
    YAML::Node config = YAML::LoadFile(config_file_seq);
    bm_config = BenchmarkConfig::decode(config.begin()->second);
  });

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

  EXPECT_EQ(bm_config.pause_frequency, 1024);
  EXPECT_EQ(bm_config.pause_length_micros, 10);

  EXPECT_EQ(bm_config.number_partitions, 1);
  EXPECT_EQ(bm_config.number_threads, 2);
}

TEST_F(ConfigTest, DecodeRandom) {
  ASSERT_NO_THROW({
    YAML::Node config = YAML::LoadFile(config_file_random);
    bm_config = BenchmarkConfig::decode(config.begin()->second);
  });

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

TEST_F(ConfigTest, InvalidHighReadWriteRatio) {
  bm_config.write_ratio = 1.0;
  bm_config.read_ratio = 1.0;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidLowReadWriteRatio) {
  bm_config.write_ratio = 0.0;
  bm_config.read_ratio = 0.0;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidReadWriteRatio) {
  bm_config.write_ratio = 1.0;
  bm_config.read_ratio = 1.0;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidSmallAccessSize) {
  bm_config.access_size = 32;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidPowerAccessSize) {
  bm_config.access_size = 100;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidMemoryRangeAccessSizeMultiple) {
  bm_config.total_memory_range = 100000;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidNumberThreads) {
  bm_config.number_threads = 0;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidNumberPartitions) {
  bm_config.number_partitions = 0;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidThreadPartitionRatio) {
  bm_config.number_partitions = 2;
  bm_config.number_threads = 1;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidPauseFrequency) {
  bm_config.pause_frequency = 256;
  bm_config.access_size = internal::MIN_IO_OP_SIZE / bm_config.pause_frequency / 2;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

}  // namespace perma
