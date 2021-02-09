#include <yaml-cpp/yaml.h>

#include "benchmark_factory.hpp"
#include "gtest/gtest.h"

namespace perma {

constexpr auto TEST_CONFIG_FILE_MATRIX = "test_matrix.yaml";
constexpr auto TEST_CONFIG_FILE_SEQ = "test_seq.yaml";
constexpr auto TEST_CONFIG_FILE_RANDOM = "test_random.yaml";

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::filesystem::path test_config_path = std::filesystem::current_path() / "resources" / "configs";
    config_file_matrix = test_config_path / TEST_CONFIG_FILE_MATRIX;
    config_file_seq = test_config_path / TEST_CONFIG_FILE_SEQ;
    config_file_random = test_config_path / TEST_CONFIG_FILE_RANDOM;
  }

  std::filesystem::path config_file_matrix;
  std::filesystem::path config_file_seq;
  std::filesystem::path config_file_random;
  BenchmarkConfig bm_config;
};

TEST_F(ConfigTest, DecodeSequential) {
  std::vector<SingleBenchmark> benchmarks = BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_file_seq);
  ASSERT_EQ(benchmarks.size(), 1);
  bm_config = benchmarks.at(0).get_benchmark_config();

  BenchmarkConfig bm_config_default{};

  EXPECT_EQ(bm_config.total_memory_range, 67108864);
  EXPECT_EQ(bm_config.access_size, 256);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Sequential);

  EXPECT_EQ(bm_config.pause_frequency, 1024);
  EXPECT_EQ(bm_config.pause_length_micros, 10);

  EXPECT_EQ(bm_config.number_threads, 2);

  EXPECT_EQ(bm_config.write_ratio, 0);

  EXPECT_EQ(bm_config.number_operations, bm_config_default.number_operations);
  EXPECT_EQ(bm_config.random_distribution, bm_config_default.random_distribution);
  EXPECT_EQ(bm_config.zipf_alpha, bm_config_default.zipf_alpha);
  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.persist_instruction, bm_config_default.persist_instruction);
  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.prefault_file, bm_config_default.prefault_file);
}

TEST_F(ConfigTest, DecodeRandom) {
  std::vector<SingleBenchmark> benchmarks = BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_file_random);
  ASSERT_EQ(benchmarks.size(), 1);
  bm_config = benchmarks.at(0).get_benchmark_config();

  BenchmarkConfig bm_config_default{};

  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Random);

  EXPECT_EQ(bm_config.random_distribution, internal::RandomDistribution::Zipf);
  EXPECT_EQ(bm_config.zipf_alpha, 0.9);

  EXPECT_EQ(bm_config.write_ratio, 0.3);

  EXPECT_EQ(bm_config.total_memory_range, bm_config_default.total_memory_range);
  EXPECT_EQ(bm_config.access_size, bm_config_default.access_size);
  EXPECT_EQ(bm_config.number_operations, bm_config_default.number_operations);
  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.persist_instruction, bm_config_default.persist_instruction);
  EXPECT_EQ(bm_config.pause_frequency, bm_config_default.pause_frequency);
  EXPECT_EQ(bm_config.pause_length_micros, bm_config_default.pause_length_micros);
  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.number_threads, bm_config_default.number_threads);
  EXPECT_EQ(bm_config.prefault_file, bm_config_default.prefault_file);
}

TEST_F(ConfigTest, DecodeMatrix) {
  const size_t num_bms = 6;
  std::vector<SingleBenchmark> benchmarks = BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_file_matrix);
  ASSERT_EQ(benchmarks.size(), num_bms);
  EXPECT_EQ(benchmarks[0].get_benchmark_config().number_threads, 1);
  EXPECT_EQ(benchmarks[0].get_benchmark_config().access_size, 256);
  EXPECT_EQ(benchmarks[1].get_benchmark_config().number_threads, 1);
  EXPECT_EQ(benchmarks[1].get_benchmark_config().access_size, 4096);
  EXPECT_EQ(benchmarks[2].get_benchmark_config().number_threads, 2);
  EXPECT_EQ(benchmarks[2].get_benchmark_config().access_size, 256);
  EXPECT_EQ(benchmarks[3].get_benchmark_config().number_threads, 2);
  EXPECT_EQ(benchmarks[3].get_benchmark_config().access_size, 4096);
  EXPECT_EQ(benchmarks[4].get_benchmark_config().number_threads, 4);
  EXPECT_EQ(benchmarks[4].get_benchmark_config().access_size, 256);
  EXPECT_EQ(benchmarks[5].get_benchmark_config().number_threads, 4);
  EXPECT_EQ(benchmarks[5].get_benchmark_config().access_size, 4096);

  BenchmarkConfig bm_config_default{};
  for (size_t i = 0; i < num_bms; ++i) {
    const SingleBenchmark& bm = benchmarks[i];
    const BenchmarkConfig& config = bm.get_benchmark_config();

    // Other args are identical for all configs
    EXPECT_EQ(config.total_memory_range, 67108864);
    EXPECT_EQ(config.exec_mode, internal::Mode::Sequential);
    EXPECT_EQ(config.write_ratio, 0);
    EXPECT_EQ(config.number_operations, bm_config_default.number_operations);
    EXPECT_EQ(config.random_distribution, bm_config_default.random_distribution);
    EXPECT_EQ(config.zipf_alpha, bm_config_default.zipf_alpha);
    EXPECT_EQ(config.data_instruction, bm_config_default.data_instruction);
    EXPECT_EQ(config.persist_instruction, bm_config_default.persist_instruction);
    EXPECT_EQ(config.number_partitions, bm_config_default.number_partitions);
    EXPECT_EQ(config.pause_frequency, bm_config_default.pause_frequency);
    EXPECT_EQ(config.pause_length_micros, bm_config_default.pause_length_micros);
    EXPECT_EQ(config.prefault_file, bm_config_default.prefault_file);
  }
}

TEST_F(ConfigTest, CheckDefaultConfig) { bm_config.validate(); }

TEST_F(ConfigTest, InvalidHighReadWriteRatio) {
  bm_config.write_ratio = 1.1;
  EXPECT_THROW(bm_config.validate(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidLowReadWriteRatio) {
  bm_config.write_ratio = -1.0;
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

}  // namespace perma
