#include <yaml-cpp/yaml.h>

#include "benchmark_factory.hpp"
#include "gtest/gtest.h"

namespace perma {

constexpr auto TEST_CONFIG_FILE_SEQ = "test_seq.yaml";
constexpr auto TEST_CONFIG_FILE_RANDOM = "test_random.yaml";

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::filesystem::path test_config_path = std::filesystem::current_path() / "resources" / "configs";
    config_file_seq = test_config_path / TEST_CONFIG_FILE_SEQ;
    config_file_random = test_config_path / TEST_CONFIG_FILE_RANDOM;
  }

  std::filesystem::path config_file_seq;
  std::filesystem::path config_file_random;
  BenchmarkConfig bm_config;
};

TEST_F(ConfigTest, DecodeSequential) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  ASSERT_NO_THROW({ benchmarks = BenchmarkFactory::create_benchmarks("/tmp/foo", config_file_seq); });
  ASSERT_EQ(benchmarks.size(), 1);
  bm_config = benchmarks.at(0)->get_benchmark_config();

  BenchmarkConfig bm_config_default{};

  EXPECT_EQ(bm_config.total_memory_range, 67108864);
  EXPECT_EQ(bm_config.access_size, 256);
  EXPECT_EQ(bm_config.number_operations, bm_config_default.number_operations);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Sequential);

  EXPECT_EQ(bm_config.random_distribution, bm_config_default.random_distribution);
  EXPECT_EQ(bm_config.zipf_alpha, bm_config_default.zipf_alpha);

  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.persist_instruction, bm_config_default.persist_instruction);

  EXPECT_EQ(bm_config.write_ratio, bm_config_default.write_ratio);
  EXPECT_EQ(bm_config.read_ratio, bm_config_default.read_ratio);

  EXPECT_EQ(bm_config.pause_frequency, 1024);
  EXPECT_EQ(bm_config.pause_length_micros, 10);

  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.number_threads, 2);
}

TEST_F(ConfigTest, DecodeRandom) {
  std::vector<std::unique_ptr<Benchmark>> benchmarks;
  ASSERT_NO_THROW({ benchmarks = BenchmarkFactory::create_benchmarks("/tmp/foo", config_file_random); });
  ASSERT_EQ(benchmarks.size(), 1);
  bm_config = benchmarks.at(0)->get_benchmark_config();

  BenchmarkConfig bm_config_default{};

  EXPECT_EQ(bm_config.total_memory_range, bm_config_default.total_memory_range);
  EXPECT_EQ(bm_config.access_size, bm_config_default.access_size);
  EXPECT_EQ(bm_config.number_operations, bm_config_default.number_operations);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Random);

  EXPECT_EQ(bm_config.random_distribution, internal::RandomDistribution::Zipf);
  EXPECT_EQ(bm_config.zipf_alpha, 0.9);

  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.persist_instruction, bm_config_default.persist_instruction);

  EXPECT_EQ(bm_config.write_ratio, 1);
  EXPECT_EQ(bm_config.read_ratio, 0);

  EXPECT_EQ(bm_config.pause_frequency, bm_config_default.pause_frequency);
  EXPECT_EQ(bm_config.pause_length_micros, bm_config_default.pause_length_micros);

  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.number_threads, bm_config_default.number_threads);
}

TEST_F(ConfigTest, CheckDefaultConfig) { EXPECT_NO_THROW(bm_config.validate()); }

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
