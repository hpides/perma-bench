#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>

#include "benchmark_factory.hpp"
#include "gtest/gtest.h"

namespace perma {

constexpr auto TEST_SINGLE_CONFIG_FILE_MATRIX = "test_matrix.yaml";
constexpr auto TEST_SINGLE_CONFIG_FILE_SEQ = "test_seq.yaml";
constexpr auto TEST_SINGLE_CONFIG_FILE_RANDOM = "test_random.yaml";
constexpr auto TEST_PAR_CONFIG_FILE_SEQ_RANDOM = "test_parallel_seq_random.yaml";
constexpr auto TEST_PAR_CONFIG_FILE_MATRIX = "test_parallel_matrix.yaml";

class ConfigTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    test_logger_path = std::filesystem::temp_directory_path() / "test-logger.log";
    auto file_logger = spdlog::basic_logger_mt("test-logger", test_logger_path.string());
    spdlog::set_default_logger(file_logger);
  }

  static void TearDownTestSuite() {
    std::filesystem::remove(test_logger_path);
  }

  void SetUp() override {
    const std::filesystem::path test_config_path = std::filesystem::current_path() / "resources" / "configs";
    config_single_file_matrix = BenchmarkFactory::get_config_files(test_config_path / TEST_SINGLE_CONFIG_FILE_MATRIX);
    config_single_file_seq = BenchmarkFactory::get_config_files(test_config_path / TEST_SINGLE_CONFIG_FILE_SEQ);
    config_single_file_random = BenchmarkFactory::get_config_files(test_config_path / TEST_SINGLE_CONFIG_FILE_RANDOM);
    config_par_file_seq_random = BenchmarkFactory::get_config_files(test_config_path / TEST_PAR_CONFIG_FILE_SEQ_RANDOM);
    config_par_file_matrix = BenchmarkFactory::get_config_files(test_config_path / TEST_PAR_CONFIG_FILE_MATRIX);
  }

  void TearDown() override {
    std::ofstream empty_log(test_logger_path, std::ostream::trunc);
  }

  static void check_log_for_critical(const std::string& expected_msg) {
    std::stringstream raw_log_content;
    std::ifstream log_checker(test_logger_path);
    raw_log_content << log_checker.rdbuf();
    std::string log_content = raw_log_content.str();

    if (log_content.find("critical") == std::string::npos) {
      FAIL() << "Did not find keyword 'critical' in log file.";
    }

    if (log_content.find(expected_msg) == std::string::npos) {
      FAIL() << "Did not find expected '" << expected_msg << "' in log file.";
    }
  }

  std::vector<YAML::Node> config_single_file_matrix;
  std::vector<YAML::Node> config_single_file_seq;
  std::vector<YAML::Node> config_single_file_random;
  std::vector<YAML::Node> config_par_file_seq_random;
  std::vector<YAML::Node> config_par_file_matrix;
  BenchmarkConfig bm_config;
  static std::filesystem::path test_logger_path;
};

std::filesystem::path ConfigTest::test_logger_path;

TEST_F(ConfigTest, SingleDecodeSequential) {
  std::vector<SingleBenchmark> benchmarks =
      BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_single_file_seq);
  std::vector<ParallelBenchmark> par_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks("/tmp/foo", config_single_file_seq);
  ASSERT_EQ(benchmarks.size(), 1);
  ASSERT_EQ(par_benchmarks.size(), 0);
  bm_config = benchmarks.at(0).get_benchmark_configs()[0];

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
  EXPECT_EQ(bm_config.numa_pattern, bm_config_default.numa_pattern);
}

TEST_F(ConfigTest, DecodeRandom) {
  std::vector<SingleBenchmark> benchmarks =
      BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_single_file_random);
  ASSERT_EQ(benchmarks.size(), 1);
  bm_config = benchmarks.at(0).get_benchmark_configs()[0];

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
  EXPECT_EQ(bm_config.numa_pattern, bm_config_default.numa_pattern);
}

TEST_F(ConfigTest, ParallelDecodeSequentialRandom) {
  std::vector<SingleBenchmark> benchmarks =
      BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_par_file_seq_random);
  std::vector<ParallelBenchmark> par_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks("/tmp/foo", config_par_file_seq_random);
  ASSERT_EQ(benchmarks.size(), 0);
  ASSERT_EQ(par_benchmarks.size(), 1);
  bm_config = par_benchmarks.at(0).get_benchmark_configs()[0];

  EXPECT_EQ(par_benchmarks.at(0).get_benchmark_name_one(), "buffer_read");
  EXPECT_EQ(par_benchmarks.at(0).get_benchmark_name_two(), "logging");

  BenchmarkConfig bm_config_default{};

  EXPECT_EQ(bm_config.total_memory_range, 10737418240);
  EXPECT_EQ(bm_config.access_size, 4096);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Random);
  EXPECT_EQ(bm_config.number_operations, 10000000);

  EXPECT_EQ(bm_config.number_threads, 8);

  EXPECT_EQ(bm_config.write_ratio, 0);

  EXPECT_EQ(bm_config.random_distribution, bm_config_default.random_distribution);
  EXPECT_EQ(bm_config.zipf_alpha, bm_config_default.zipf_alpha);
  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.persist_instruction, bm_config_default.persist_instruction);
  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.prefault_file, bm_config_default.prefault_file);
  EXPECT_EQ(bm_config.pause_frequency, bm_config_default.pause_frequency);
  EXPECT_EQ(bm_config.pause_length_micros, bm_config_default.pause_length_micros);
  EXPECT_EQ(bm_config.numa_pattern, bm_config_default.numa_pattern);

  bm_config = par_benchmarks.at(0).get_benchmark_configs()[1];

  EXPECT_EQ(bm_config.total_memory_range, 10737418240);
  EXPECT_EQ(bm_config.access_size, 256);
  EXPECT_EQ(bm_config.exec_mode, internal::Mode::Sequential);
  EXPECT_EQ(bm_config.persist_instruction, internal::PersistInstruction::NoCache);

  EXPECT_EQ(bm_config.number_threads, 16);

  EXPECT_EQ(bm_config.write_ratio, 1);

  EXPECT_EQ(bm_config.number_operations, bm_config_default.number_operations);
  EXPECT_EQ(bm_config.random_distribution, bm_config_default.random_distribution);
  EXPECT_EQ(bm_config.zipf_alpha, bm_config_default.zipf_alpha);
  EXPECT_EQ(bm_config.data_instruction, bm_config_default.data_instruction);
  EXPECT_EQ(bm_config.number_partitions, bm_config_default.number_partitions);
  EXPECT_EQ(bm_config.prefault_file, bm_config_default.prefault_file);
  EXPECT_EQ(bm_config.pause_frequency, bm_config_default.pause_frequency);
  EXPECT_EQ(bm_config.pause_length_micros, bm_config_default.pause_length_micros);
  EXPECT_EQ(bm_config.numa_pattern, bm_config_default.numa_pattern);
}

TEST_F(ConfigTest, DecodeMatrix) {
  const size_t num_bms = 6;
  std::vector<SingleBenchmark> benchmarks =
      BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_single_file_matrix);
  std::vector<ParallelBenchmark> par_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks("/tmp/foo", config_single_file_matrix);
  ASSERT_EQ(benchmarks.size(), num_bms);
  ASSERT_EQ(par_benchmarks.size(), 0);
  EXPECT_EQ(benchmarks[0].get_benchmark_configs()[0].number_threads, 1);
  EXPECT_EQ(benchmarks[0].get_benchmark_configs()[0].access_size, 256);
  EXPECT_EQ(benchmarks[1].get_benchmark_configs()[0].number_threads, 1);
  EXPECT_EQ(benchmarks[1].get_benchmark_configs()[0].access_size, 4096);
  EXPECT_EQ(benchmarks[2].get_benchmark_configs()[0].number_threads, 2);
  EXPECT_EQ(benchmarks[2].get_benchmark_configs()[0].access_size, 256);
  EXPECT_EQ(benchmarks[3].get_benchmark_configs()[0].number_threads, 2);
  EXPECT_EQ(benchmarks[3].get_benchmark_configs()[0].access_size, 4096);
  EXPECT_EQ(benchmarks[4].get_benchmark_configs()[0].number_threads, 4);
  EXPECT_EQ(benchmarks[4].get_benchmark_configs()[0].access_size, 256);
  EXPECT_EQ(benchmarks[5].get_benchmark_configs()[0].number_threads, 4);
  EXPECT_EQ(benchmarks[5].get_benchmark_configs()[0].access_size, 4096);

  BenchmarkConfig bm_config_default{};
  for (size_t i = 0; i < num_bms; ++i) {
    const SingleBenchmark& bm = benchmarks[i];
    const BenchmarkConfig& config = bm.get_benchmark_configs()[0];

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
    EXPECT_EQ(config.numa_pattern, bm_config_default.numa_pattern);
  }
}

TEST_F(ConfigTest, ParallelDecodeMatrix) {
  const uint8_t num_bms = 4;
  std::vector<SingleBenchmark> benchmarks =
      BenchmarkFactory::create_single_benchmarks("/tmp/foo", config_par_file_matrix);
  std::vector<ParallelBenchmark> par_benchmarks =
      BenchmarkFactory::create_parallel_benchmarks("/tmp/foo", config_par_file_matrix);
  ASSERT_EQ(benchmarks.size(), 0);
  ASSERT_EQ(par_benchmarks.size(), num_bms);

  EXPECT_EQ(par_benchmarks.at(0).get_benchmark_name_one(), "buffer_read");
  EXPECT_EQ(par_benchmarks.at(0).get_benchmark_name_two(), "logging");

  EXPECT_EQ(par_benchmarks[0].get_benchmark_configs()[0].number_threads, 8);
  EXPECT_EQ(par_benchmarks[0].get_benchmark_configs()[1].access_size, 64);
  EXPECT_EQ(par_benchmarks[1].get_benchmark_configs()[0].number_threads, 16);
  EXPECT_EQ(par_benchmarks[1].get_benchmark_configs()[1].access_size, 64);
  EXPECT_EQ(par_benchmarks[2].get_benchmark_configs()[0].number_threads, 8);
  EXPECT_EQ(par_benchmarks[2].get_benchmark_configs()[1].access_size, 256);
  EXPECT_EQ(par_benchmarks[3].get_benchmark_configs()[0].number_threads, 16);
  EXPECT_EQ(par_benchmarks[3].get_benchmark_configs()[1].access_size, 256);

  BenchmarkConfig bm_config_default{};
  for (size_t i = 0; i < num_bms; ++i) {
    const ParallelBenchmark& bm = par_benchmarks[i];
    const BenchmarkConfig& config_one = bm.get_benchmark_configs()[0];
    const BenchmarkConfig& config_two = bm.get_benchmark_configs()[1];

    // Other args are identical for all configs
    EXPECT_EQ(config_one.total_memory_range, 10737418240);
    EXPECT_EQ(config_one.access_size, 4096);
    EXPECT_EQ(config_one.exec_mode, internal::Mode::Random);
    EXPECT_EQ(config_one.number_operations, 10000000);
    EXPECT_EQ(config_one.write_ratio, 0);
    EXPECT_EQ(config_one.random_distribution, bm_config_default.random_distribution);
    EXPECT_EQ(config_one.zipf_alpha, bm_config_default.zipf_alpha);
    EXPECT_EQ(config_one.data_instruction, bm_config_default.data_instruction);
    EXPECT_EQ(config_one.persist_instruction, bm_config_default.persist_instruction);
    EXPECT_EQ(config_one.number_partitions, bm_config_default.number_partitions);
    EXPECT_EQ(config_one.prefault_file, bm_config_default.prefault_file);
    EXPECT_EQ(config_one.pause_frequency, bm_config_default.pause_frequency);
    EXPECT_EQ(config_one.pause_length_micros, bm_config_default.pause_length_micros);
    EXPECT_EQ(config_one.numa_pattern, bm_config_default.numa_pattern);

    EXPECT_EQ(config_two.total_memory_range, 10737418240);
    EXPECT_EQ(config_two.exec_mode, internal::Mode::Sequential);
    EXPECT_EQ(config_two.write_ratio, 1);
    EXPECT_EQ(config_two.number_threads, 16);
    EXPECT_EQ(config_two.persist_instruction, internal::PersistInstruction::NoCache);
    EXPECT_EQ(config_two.number_operations, bm_config_default.number_operations);
    EXPECT_EQ(config_two.random_distribution, bm_config_default.random_distribution);
    EXPECT_EQ(config_two.zipf_alpha, bm_config_default.zipf_alpha);
    EXPECT_EQ(config_two.data_instruction, bm_config_default.data_instruction);
    EXPECT_EQ(config_two.number_partitions, bm_config_default.number_partitions);
    EXPECT_EQ(config_two.pause_frequency, bm_config_default.pause_frequency);
    EXPECT_EQ(config_two.pause_length_micros, bm_config_default.pause_length_micros);
    EXPECT_EQ(config_two.prefault_file, bm_config_default.prefault_file);
    EXPECT_EQ(config_two.numa_pattern, bm_config_default.numa_pattern);
  }
}

TEST_F(ConfigTest, CheckDefaultConfig) { bm_config.validate(); }

TEST_F(ConfigTest, InvalidHighReadWriteRatio) {
  bm_config.write_ratio = 1.1;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("must be between");
}

TEST_F(ConfigTest, InvalidLowReadWriteRatio) {
  bm_config.write_ratio = -1.0;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("must be between");
}

TEST_F(ConfigTest, InvalidSmallAccessSize) {
  bm_config.access_size = 32;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("at least 64-byte");
}

TEST_F(ConfigTest, InvalidPowerAccessSize) {
  bm_config.access_size = 100;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("power of 2");
}

TEST_F(ConfigTest, InvalidMemoryRangeAccessSizeMultiple) {
  bm_config.total_memory_range = 100000;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("multiple of access size");
}

TEST_F(ConfigTest, InvalidNumberThreads) {
  bm_config.number_threads = 0;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("threads must be");
}

TEST_F(ConfigTest, InvalidNumberPartitions) {
  bm_config.number_partitions = 0;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("partitions must be");
}

TEST_F(ConfigTest, InvalidThreadPartitionRatio) {
  bm_config.number_partitions = 2;
  bm_config.number_threads = 1;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("threads must be a multiple of number partitions");
}

TEST_F(ConfigTest, BadNumberPartitionSplit) {
  bm_config.number_threads = 36;
  bm_config.number_partitions = 36;
  EXPECT_DEATH(bm_config.validate(), "");
  check_log_for_critical("evenly divisible into");
}

}  // namespace perma
