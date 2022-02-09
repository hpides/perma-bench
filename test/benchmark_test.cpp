#include <fcntl.h>

#include <benchmark.hpp>
#include <fstream>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "parallel_benchmark.hpp"
#include "single_benchmark.hpp"
#include "test_utils.hpp"

namespace perma {

using ::testing::ElementsAre;

constexpr size_t TEST_FILE_SIZE = 1 * BYTES_IN_MEGABYTE;  // 1 MiB
constexpr size_t TEST_CHUNK_SIZE = TEST_FILE_SIZE / 8;    // 128 KiB

class BenchmarkTest : public ::testing::Test {
 protected:
  void SetUp() override {
    base_config_.pmem_directory = temp_dir_;
    base_config_.memory_range = TEST_FILE_SIZE;
    base_config_.min_io_chunk_size = TEST_CHUNK_SIZE;

    utils::setPMEM_MAP_FLAGS(MAP_SHARED);
  }

  BenchmarkConfig base_config_{};
  std::vector<std::unique_ptr<BenchmarkExecution>> base_executions_{};
  std::vector<std::unique_ptr<BenchmarkResult>> base_results_{};
  const std::string bm_name_ = "test_bm";
  const std::filesystem::path temp_dir_ = std::filesystem::temp_directory_path();
};

TEST_F(BenchmarkTest, CreateSingleBenchmark) {
  ASSERT_NO_THROW(SingleBenchmark("test_bm1", base_config_, std::move(base_executions_), std::move(base_results_)));
  ASSERT_NO_THROW(
      SingleBenchmark("test_bm2", base_config_, std::move(base_executions_), std::move(base_results_), "/tmp/foo/bar"));
}

TEST_F(BenchmarkTest, CreateParallelBenchmark) {
  ASSERT_NO_THROW(ParallelBenchmark("test_bm1", "sub_bm_1_1", "sub_bm_1_2", base_config_, base_config_,
                                    std::move(base_executions_), std::move(base_results_)));
  ASSERT_NO_THROW(ParallelBenchmark("test_bm2", "sub_bm_2_1", "sub_bm_2_2", base_config_, base_config_,
                                    std::move(base_executions_), std::move(base_results_), "/tmp/foo/bar"));
  ASSERT_NO_THROW(ParallelBenchmark("test_bm3", "sub_bm_3_1", "sub_bm_3_2", base_config_, base_config_,
                                    std::move(base_executions_), std::move(base_results_), "/tmp/foo/bar1",
                                    "/tmp/foo/bar2"));
}

#ifdef HAS_AVX
TEST_F(BenchmarkTest, CreateSingleNewDataFile) {
  base_config_.operation = Operation::Write;
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};
  const std::filesystem::path pmem_file = bm.get_pmem_file(0);

  ASSERT_FALSE(std::filesystem::exists(pmem_file));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), TEST_FILE_SIZE);
  EXPECT_TRUE(bm.owns_pmem_file(0));
  EXPECT_NE(bm.get_pmem_data()[0], nullptr);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);
}

TEST_F(BenchmarkTest, CreateParallelNewDataFile) {
  base_config_.operation = Operation::Write;
  ParallelBenchmark bm{bm_name_,
                       "sub_bm_1",
                       "sub_bm_2",
                       base_config_,
                       base_config_,
                       std::move(base_executions_),
                       std::move(base_results_)};
  const std::filesystem::path pmem_file_one = bm.get_pmem_file(0);
  const std::filesystem::path pmem_file_two = bm.get_pmem_file(1);

  ASSERT_FALSE(std::filesystem::exists(pmem_file_one));
  ASSERT_FALSE(std::filesystem::exists(pmem_file_two));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file_one));
  ASSERT_TRUE(std::filesystem::exists(pmem_file_two));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file_one));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file_two));
  EXPECT_EQ(std::filesystem::file_size(pmem_file_one), TEST_FILE_SIZE);
  EXPECT_EQ(std::filesystem::file_size(pmem_file_two), TEST_FILE_SIZE);
  EXPECT_TRUE(bm.owns_pmem_file(0));
  EXPECT_TRUE(bm.owns_pmem_file(1));
  EXPECT_NE(bm.get_pmem_data()[0], nullptr);
  EXPECT_NE(bm.get_pmem_data()[1], nullptr);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);
  EXPECT_EQ(bm.get_dram_data()[1], nullptr);
}

TEST_F(BenchmarkTest, CreateExistingDataFile) {
  base_config_.operation = Operation::Write;

  const std::string test_string = "test123456789";
  const std::filesystem::path existing_pmem_file = utils::generate_random_file_name(temp_dir_);
  std::ofstream temp_stream{existing_pmem_file, std::ios::out};
  temp_stream << test_string << std::endl;
  temp_stream.flush();
  temp_stream.close();
  std::filesystem::resize_file(existing_pmem_file, TEST_FILE_SIZE);

  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_), existing_pmem_file};
  const std::filesystem::path pmem_file = bm.get_pmem_file(0);

  ASSERT_EQ(pmem_file, existing_pmem_file);
  ASSERT_TRUE(std::filesystem::exists(pmem_file));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), TEST_FILE_SIZE);
  EXPECT_FALSE(bm.owns_pmem_file(0));
  const char* pmem_data = bm.get_pmem_data()[0];
  ASSERT_NE(pmem_data, nullptr);
  const std::string_view mapped_data{pmem_data, test_string.size()};
  EXPECT_EQ(mapped_data, test_string);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);

  bm.tear_down(true);
}

TEST_F(BenchmarkTest, CreateSingleReadDataFile) {
  base_config_.operation = Operation::Read;
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};
  const std::filesystem::path pmem_file = bm.get_pmem_file(0);

  ASSERT_FALSE(std::filesystem::exists(pmem_file));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), TEST_FILE_SIZE);

  EXPECT_TRUE(bm.owns_pmem_file(0));

  const char* pmem_data = bm.get_pmem_data()[0];
  ASSERT_NE(pmem_data, nullptr);
  const size_t data_size = rw_ops::CACHE_LINE_SIZE;
  const std::string_view mapped_data{pmem_data, data_size};
  const std::string_view expected_data{rw_ops::WRITE_DATA, data_size};
  EXPECT_EQ(mapped_data, expected_data);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);
}

TEST_F(BenchmarkTest, CreateParallelReadDataFile) {
  base_config_.operation = Operation::Read;
  ParallelBenchmark bm{bm_name_,
                       "sub_bm_1",
                       "sub_bm_2",
                       base_config_,
                       base_config_,
                       std::move(base_executions_),
                       std::move(base_results_)};
  const std::filesystem::path pmem_file_one = bm.get_pmem_file(0);
  const std::filesystem::path pmem_file_two = bm.get_pmem_file(1);

  ASSERT_FALSE(std::filesystem::exists(pmem_file_one));
  ASSERT_FALSE(std::filesystem::exists(pmem_file_two));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file_one));
  ASSERT_TRUE(std::filesystem::exists(pmem_file_two));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file_one));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file_two));
  EXPECT_EQ(std::filesystem::file_size(pmem_file_one), TEST_FILE_SIZE);
  EXPECT_EQ(std::filesystem::file_size(pmem_file_two), TEST_FILE_SIZE);

  EXPECT_TRUE(bm.owns_pmem_file(0));
  EXPECT_TRUE(bm.owns_pmem_file(1));

  const char* pmem_data_one = bm.get_pmem_data()[0];
  const char* pmem_data_two = bm.get_pmem_data()[1];
  ASSERT_NE(pmem_data_one, nullptr);
  ASSERT_NE(pmem_data_two, nullptr);
  const size_t data_size = rw_ops::CACHE_LINE_SIZE;
  const std::string_view mapped_data_one{pmem_data_one, data_size};
  const std::string_view mapped_data_two{pmem_data_two, data_size};
  const std::string_view expected_data{rw_ops::WRITE_DATA, data_size};
  EXPECT_EQ(mapped_data_one, expected_data);
  EXPECT_EQ(mapped_data_two, expected_data);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);
  EXPECT_EQ(bm.get_dram_data()[1], nullptr);
}

TEST_F(BenchmarkTest, CreateParallelReadDataFileMixed) {
  base_config_.operation = Operation::Read;
  BenchmarkConfig base_config_two = base_config_;
  base_config_two.operation = Operation::Write;
  ParallelBenchmark bm{bm_name_,
                       "sub_bm_1",
                       "sub_bm_2",
                       base_config_two,
                       base_config_,
                       std::move(base_executions_),
                       std::move(base_results_)};
  const std::filesystem::path pmem_file = bm.get_pmem_file(1);

  ASSERT_FALSE(std::filesystem::exists(pmem_file));

  bm.create_data_files();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), TEST_FILE_SIZE);

  EXPECT_TRUE(bm.owns_pmem_file(1));

  const char* pmem_data = bm.get_pmem_data()[1];
  ASSERT_NE(pmem_data, nullptr);
  const size_t data_size = rw_ops::CACHE_LINE_SIZE;
  const std::string_view mapped_data{pmem_data, data_size};
  const std::string_view expected_data{rw_ops::WRITE_DATA, data_size};
  EXPECT_EQ(mapped_data, expected_data);
  EXPECT_EQ(bm.get_dram_data()[0], nullptr);
}

TEST_F(BenchmarkTest, SetUpSingleThread) {
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};
  bm.create_data_files();
  bm.set_up();

  const std::vector<ThreadRunConfig>& thread_configs = bm.get_thread_configs()[0];
  ASSERT_EQ(thread_configs.size(), 1);
  const ThreadRunConfig& thread_config = thread_configs[0];

  EXPECT_EQ(thread_config.thread_num, 0);
  EXPECT_EQ(thread_config.num_threads_per_partition, 1);
  EXPECT_EQ(thread_config.partition_size, TEST_FILE_SIZE);
  EXPECT_EQ(thread_config.dram_partition_size, 0);
  EXPECT_EQ(thread_config.num_ops_per_chunk, TEST_CHUNK_SIZE / 256);
  EXPECT_EQ(thread_config.num_chunks, 8);
  EXPECT_EQ(thread_config.partition_start_addr, bm.get_pmem_data()[0]);
  EXPECT_EQ(thread_config.dram_partition_start_addr, bm.get_dram_data()[0]);
  EXPECT_EQ(&thread_config.config, &bm.get_benchmark_configs()[0]);

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_EQ(thread_config.total_operation_duration, &op_durations[0]);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  ASSERT_EQ(op_sizes.size(), 1);
  EXPECT_EQ(thread_config.total_operation_size, &op_sizes[0]);

  bm.get_benchmark_results()[0]->config.validate();
}

TEST_F(BenchmarkTest, SetUpMultiThreadCustomPartition) {
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.number_partitions = 2;
  base_config_.access_size = 512;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};
  bm.create_data_files();
  bm.set_up();

  const size_t partition_size = TEST_FILE_SIZE / 2;
  const std::vector<ThreadRunConfig>& thread_configs = bm.get_thread_configs()[0];
  ASSERT_EQ(thread_configs.size(), num_threads);
  const ThreadRunConfig& thread_config0 = thread_configs[0];
  const ThreadRunConfig& thread_config1 = thread_configs[1];
  const ThreadRunConfig& thread_config2 = thread_configs[2];
  const ThreadRunConfig& thread_config3 = thread_configs[3];

  EXPECT_EQ(thread_config0.thread_num, 0);
  EXPECT_EQ(thread_config1.thread_num, 1);
  EXPECT_EQ(thread_config2.thread_num, 2);
  EXPECT_EQ(thread_config3.thread_num, 3);

  EXPECT_EQ(thread_config0.partition_start_addr, bm.get_pmem_data()[0]);
  EXPECT_EQ(thread_config1.partition_start_addr, bm.get_pmem_data()[0]);
  EXPECT_EQ(thread_config2.partition_start_addr, bm.get_pmem_data()[0] + partition_size);
  EXPECT_EQ(thread_config3.partition_start_addr, bm.get_pmem_data()[0] + partition_size);

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), num_threads);
  EXPECT_EQ(thread_config0.total_operation_duration, &op_durations[0]);
  EXPECT_EQ(thread_config1.total_operation_duration, &op_durations[1]);
  EXPECT_EQ(thread_config2.total_operation_duration, &op_durations[2]);
  EXPECT_EQ(thread_config3.total_operation_duration, &op_durations[3]);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  ASSERT_EQ(op_sizes.size(), num_threads);
  EXPECT_EQ(thread_config0.total_operation_size, &op_sizes[0]);
  EXPECT_EQ(thread_config1.total_operation_size, &op_sizes[1]);
  EXPECT_EQ(thread_config2.total_operation_size, &op_sizes[2]);
  EXPECT_EQ(thread_config3.total_operation_size, &op_sizes[3]);

  // These values are the same for all threads
  for (const ThreadRunConfig& tc : thread_configs) {
    EXPECT_EQ(tc.num_threads_per_partition, 2);
    EXPECT_EQ(tc.partition_size, partition_size);
    EXPECT_EQ(tc.num_ops_per_chunk, TEST_CHUNK_SIZE / 512);
    EXPECT_EQ(tc.num_chunks, 8);
    EXPECT_EQ(&tc.config, &bm.get_benchmark_configs()[0]);
  }
  bm.get_benchmark_results()[0]->config.validate();
}

TEST_F(BenchmarkTest, SetUpMultiThreadDefaultPartition) {
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.number_partitions = 0;
  base_config_.access_size = 256;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};
  bm.create_data_files();
  bm.set_up();

  const size_t partition_size = TEST_FILE_SIZE / 4;
  const std::vector<ThreadRunConfig>& thread_configs = bm.get_thread_configs()[0];
  ASSERT_EQ(thread_configs.size(), num_threads);
  const ThreadRunConfig& thread_config0 = thread_configs[0];
  const ThreadRunConfig& thread_config1 = thread_configs[1];
  const ThreadRunConfig& thread_config2 = thread_configs[2];
  const ThreadRunConfig& thread_config3 = thread_configs[3];

  EXPECT_EQ(thread_config0.thread_num, 0);
  EXPECT_EQ(thread_config1.thread_num, 1);
  EXPECT_EQ(thread_config2.thread_num, 2);
  EXPECT_EQ(thread_config3.thread_num, 3);

  EXPECT_EQ(thread_config0.partition_start_addr, bm.get_pmem_data()[0] + (0 * partition_size));
  EXPECT_EQ(thread_config1.partition_start_addr, bm.get_pmem_data()[0] + (1 * partition_size));
  EXPECT_EQ(thread_config2.partition_start_addr, bm.get_pmem_data()[0] + (2 * partition_size));
  EXPECT_EQ(thread_config3.partition_start_addr, bm.get_pmem_data()[0] + (3 * partition_size));

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), num_threads);
  EXPECT_EQ(thread_config0.total_operation_duration, &op_durations[0]);
  EXPECT_EQ(thread_config1.total_operation_duration, &op_durations[1]);
  EXPECT_EQ(thread_config2.total_operation_duration, &op_durations[2]);
  EXPECT_EQ(thread_config3.total_operation_duration, &op_durations[3]);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  ASSERT_EQ(op_sizes.size(), num_threads);
  EXPECT_EQ(thread_config0.total_operation_size, &op_sizes[0]);
  EXPECT_EQ(thread_config1.total_operation_size, &op_sizes[1]);
  EXPECT_EQ(thread_config2.total_operation_size, &op_sizes[2]);
  EXPECT_EQ(thread_config3.total_operation_size, &op_sizes[3]);

  // These values are the same for all threads
  for (const ThreadRunConfig& tc : thread_configs) {
    EXPECT_EQ(tc.num_threads_per_partition, 1);
    EXPECT_EQ(tc.partition_size, partition_size);
    EXPECT_EQ(tc.num_ops_per_chunk, TEST_CHUNK_SIZE / 256);
    EXPECT_EQ(tc.num_chunks, 8);
    EXPECT_EQ(&tc.config, &bm.get_benchmark_configs()[0]);
  }
  bm.get_benchmark_results()[0]->config.validate();
}

TEST_F(BenchmarkTest, RunSingeThreadRead) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = 256 * num_ops;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_GT(op_durations[0].begin, start_test_ts);
  EXPECT_GT(op_durations[0].end, start_test_ts);
  EXPECT_LT(op_durations[0].begin, op_durations[0].end);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_THAT(op_sizes, ElementsAre(TEST_FILE_SIZE));
}

TEST_F(BenchmarkTest, RunSingeThreadReadDRAM) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = 256 * num_ops;
  base_config_.is_pmem = false;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_GT(op_durations[0].begin, start_test_ts);
  EXPECT_GT(op_durations[0].end, start_test_ts);
  EXPECT_LT(op_durations[0].begin, op_durations[0].end);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_THAT(op_sizes, ElementsAre(TEST_FILE_SIZE));
}

TEST_F(BenchmarkTest, RunSingleThreadWrite) {
  const size_t num_ops = TEST_FILE_SIZE / 512;
  base_config_.number_threads = 1;
  base_config_.access_size = 512;
  base_config_.operation = Operation::Write;
  base_config_.memory_range = TEST_FILE_SIZE;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_GT(op_durations[0].begin, start_test_ts);
  EXPECT_GT(op_durations[0].end, start_test_ts);
  EXPECT_LT(op_durations[0].begin, op_durations[0].end);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_THAT(op_sizes, ElementsAre(TEST_FILE_SIZE));

  check_file_written(bm.get_pmem_file(0), TEST_FILE_SIZE);
}

TEST_F(BenchmarkTest, RunSingleThreadWriteDRAM) {
  const size_t num_ops = TEST_FILE_SIZE / 64;
  const size_t total_size = 64 * num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 64;
  base_config_.operation = Operation::Write;
  base_config_.memory_range = total_size;
  base_config_.is_pmem = false;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_GT(op_durations[0].begin, start_test_ts);
  EXPECT_GT(op_durations[0].end, start_test_ts);
  EXPECT_LT(op_durations[0].begin, op_durations[0].end);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_THAT(op_sizes, ElementsAre(total_size));
}

// TODO(#167): Change "mixed" to DRAM/PMem
TEST_F(BenchmarkTest, DISABLED_RunSingeThreadMixed) {
  const size_t ops_per_chunk = TEST_FILE_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  // TODO: change
  //  base_config_.write_ratio = 0.5;
  base_config_.number_operations = num_ops;
  base_config_.exec_mode = Mode::Random;
  base_config_.memory_range = 256 * num_ops;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  ASSERT_EQ(op_durations.size(), 1);
  EXPECT_GT(op_durations[0].begin, start_test_ts);
  EXPECT_GT(op_durations[0].end, start_test_ts);
  EXPECT_LT(op_durations[0].begin, op_durations[0].end);

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  uint64_t dummy_value_to_fail = 12346;
  EXPECT_THAT(op_sizes, ElementsAre(dummy_value_to_fail));
}

TEST_F(BenchmarkTest, RunMultiThreadRead) {
  const size_t num_ops = TEST_FILE_SIZE / 1024;
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = 1024 * num_ops;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  EXPECT_EQ(op_durations.size(), num_threads);
  for (ExecutionDuration duration : op_durations) {
    EXPECT_GT(duration.begin, start_test_ts);
    EXPECT_GT(duration.end, start_test_ts);
    EXPECT_LE(duration.begin, duration.end);
  }

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_EQ(op_durations.size(), num_threads);
  EXPECT_EQ(std::accumulate(op_sizes.begin(), op_sizes.end(), 0ul), TEST_FILE_SIZE);
  for (uint64_t size : op_sizes) {
    EXPECT_EQ(size % TEST_CHUNK_SIZE, 0);
  }
}

TEST_F(BenchmarkTest, RunMultiThreadWrite) {
  const size_t num_ops = TEST_FILE_SIZE / 512;
  const size_t num_threads = 16;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = total_size;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  EXPECT_EQ(op_durations.size(), num_threads);
  for (ExecutionDuration duration : op_durations) {
    EXPECT_GT(duration.begin, start_test_ts);
    EXPECT_GT(duration.end, start_test_ts);
    EXPECT_LE(duration.begin, duration.end);
  }

  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_EQ(op_sizes.size(), 16);
  EXPECT_EQ(std::accumulate(op_sizes.begin(), op_sizes.end(), 0ul), TEST_FILE_SIZE);
  for (uint64_t size : op_sizes) {
    EXPECT_EQ(size % TEST_CHUNK_SIZE, 0);
  }

  check_file_written(bm.get_pmem_file(0), total_size);
}

TEST_F(BenchmarkTest, RunMultiThreadReadDesc) {
  const size_t num_ops = TEST_FILE_SIZE / 1024;
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = 1024 * num_ops;
  base_config_.exec_mode = Mode::Sequential_Desc;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  EXPECT_EQ(op_durations.size(), num_threads);
  for (ExecutionDuration duration : op_durations) {
    EXPECT_GT(duration.begin, start_test_ts);
    EXPECT_GT(duration.end, start_test_ts);
    EXPECT_LE(duration.begin, duration.end);
  }

  const uint64_t per_thread_size = TEST_FILE_SIZE / num_threads;
  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_EQ(op_sizes.size(), 4);
  EXPECT_EQ(std::accumulate(op_sizes.begin(), op_sizes.end(), 0ul), TEST_FILE_SIZE);
  for (uint64_t size : op_sizes) {
    EXPECT_EQ(size % TEST_CHUNK_SIZE, 0);
  }
}

TEST_F(BenchmarkTest, RunMultiThreadWriteDesc) {
  const size_t num_ops = TEST_FILE_SIZE / 512;
  const size_t num_threads = 16;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = total_size;
  base_config_.exec_mode = Mode::Sequential_Desc;
  base_executions_.reserve(1);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(base_config_));
  base_results_.reserve(1);
  base_results_.push_back(std::make_unique<BenchmarkResult>(base_config_));
  SingleBenchmark bm{bm_name_, base_config_, std::move(base_executions_), std::move(base_results_)};

  const auto start_test_ts = std::chrono::steady_clock::now();

  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = *bm.get_benchmark_results()[0];

  const std::vector<ExecutionDuration>& op_durations = bm.get_benchmark_results()[0]->total_operation_durations;
  EXPECT_EQ(op_durations.size(), num_threads);
  for (ExecutionDuration duration : op_durations) {
    EXPECT_GT(duration.begin, start_test_ts);
    EXPECT_GT(duration.end, start_test_ts);
    EXPECT_LE(duration.begin, duration.end);
  }

  const uint64_t per_thread_size = TEST_FILE_SIZE / num_threads;
  const std::vector<uint64_t>& op_sizes = bm.get_benchmark_results()[0]->total_operation_sizes;
  EXPECT_EQ(op_sizes.size(), 16);
  EXPECT_EQ(std::accumulate(op_sizes.begin(), op_sizes.end(), 0ul), TEST_FILE_SIZE);
  for (uint64_t size : op_sizes) {
    EXPECT_EQ(size % TEST_CHUNK_SIZE, 0);
  }

  check_file_written(bm.get_pmem_file(0), total_size);
}

TEST_F(BenchmarkTest, ResultsSingleThreadRead) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = TEST_FILE_SIZE;

  BenchmarkResult bm_result{base_config_};

  const uint64_t total_op_duration = 1000000;
  const auto start = std::chrono::steady_clock::now();
  const auto end = start + std::chrono::nanoseconds(total_op_duration);
  bm_result.total_operation_durations.push_back({start, end});
  bm_result.total_operation_sizes.emplace_back(TEST_FILE_SIZE);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  check_json_result(result_json, TEST_FILE_SIZE, 0.9765625, 1, 0.9765625, 0.0);
}

TEST_F(BenchmarkTest, ResultsSingleThreadWrite) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.operation = Operation::Write;
  base_config_.memory_range = TEST_FILE_SIZE;

  BenchmarkResult bm_result{base_config_};
  const uint64_t total_op_duration = 2000000;
  const auto start = std::chrono::steady_clock::now();
  const auto end = start + std::chrono::nanoseconds(total_op_duration);
  bm_result.total_operation_durations.push_back({start, end});
  bm_result.total_operation_sizes.emplace_back(TEST_FILE_SIZE);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  check_json_result(result_json, TEST_FILE_SIZE, 0.48828125, 1, 0.48828125, 0.0);
}

// TODO(#167): Change "mixed" to DRAM/PMem
TEST_F(BenchmarkTest, DISABLED_ResultsSingleThreadMixed) {
  const size_t ops_per_chunk = TEST_FILE_SIZE / 512;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t total_size = 512 * num_ops;
  base_config_.number_operations = num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 512;
  // TODO: change
  //   base_config_.write_ratio = 0.5;
  base_config_.memory_range = total_size;
  base_config_.exec_mode = Mode::Random;

  BenchmarkResult bm_result{base_config_};
  //  std::vector<Latency> latencies{};
  //  latencies.reserve(num_ops);
  //  for (size_t i = 0; i < num_ops; ++i) {
  //    if (i % 2 == 0) {
  //      latencies.emplace_back(200 * ops_per_chunk, Operation::Write);
  //    } else {
  //      latencies.emplace_back(100 * ops_per_chunk, Operation::Read);
  //    }
  //  }
  //  bm_result.latencies.emplace_back(latencies);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 1);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 2);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_NEAR(bandwidth_json.at("read").get<double>(), 2.384185, 0.1);
  EXPECT_NEAR(bandwidth_json.at("write").get<double>(), 1.192095, 0.1);
}

TEST_F(BenchmarkTest, ResultsMultiThreadRead) {
  const size_t num_ops = TEST_FILE_SIZE / 1024;
  const size_t num_threads = 4;
  const size_t num_ops_per_thread = num_ops / num_threads;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = TEST_FILE_SIZE;

  BenchmarkResult bm_result{base_config_};
  const auto start = std::chrono::steady_clock::now();
  for (size_t thread = 0; thread < num_threads; ++thread) {
    const uint64_t thread_dur = (250000 + (10000 * thread));
    const auto end = start + std::chrono::nanoseconds(thread_dur);
    bm_result.total_operation_durations.push_back({start, end});
    bm_result.total_operation_sizes.emplace_back(TEST_FILE_SIZE / num_threads);
  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  check_json_result(result_json, TEST_FILE_SIZE, 3.48772321, 4, 0.8719308, 0.0741378);
}

TEST_F(BenchmarkTest, ResultsMultiThreadWrite) {
  const size_t num_ops = TEST_FILE_SIZE / 512;
  const size_t num_threads = 8;
  const size_t num_ops_per_thread = num_ops / num_threads;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.operation = Operation::Write;
  base_config_.memory_range = TEST_FILE_SIZE;

  BenchmarkResult bm_result{base_config_};
  const auto start = std::chrono::steady_clock::now();
  for (size_t thread = 0; thread < num_threads; ++thread) {
    const uint64_t thread_dur = (250000 + (10000 * thread));
    const auto end = start + std::chrono::nanoseconds(thread_dur);
    bm_result.total_operation_durations.push_back({start, end});
    bm_result.total_operation_sizes.emplace_back(TEST_FILE_SIZE / num_threads);
  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  check_json_result(result_json, TEST_FILE_SIZE, 3.0517578, 8, 0.38146972, 0.0648887);
}

// TODO(#167): Change "mixed" to DRAM/PMem
TEST_F(BenchmarkTest, DISABLED_ResultsMultiThreadMixed) {
  const size_t ops_per_chunk = TEST_FILE_SIZE / 512;
  const size_t num_chunks = 64;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 16;
  const size_t num_ops_per_thread = num_ops / num_threads;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.number_operations = num_ops;
  base_config_.access_size = 512;
  // TODO: change
  //   base_config_.write_ratio = 0.5;
  base_config_.memory_range = total_size;
  base_config_.exec_mode = Mode::Random;

  BenchmarkResult bm_result{base_config_};
  //  for (size_t thread = 0; thread < num_threads; ++thread) {
  //    std::vector<Latency> latencies{};
  //    for (size_t i = 0; i < num_ops_per_thread; ++i) {
  //      if (i % 2 == 0) {
  //        latencies.emplace_back(500 * ops_per_chunk, Operation::Write);
  //      } else {
  //        latencies.emplace_back(400 * ops_per_chunk, Operation::Read);
  //      }
  //    }
  //    bm_result.latencies.emplace_back(latencies);
  //  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 1);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 2);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_NEAR(bandwidth_json.at("read").get<double>(), 9.535, 0.1);
  EXPECT_NEAR(bandwidth_json.at("write").get<double>(), 7.625, 0.1);
}

TEST_F(BenchmarkTest, RunParallelSingleThreadRead) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.operation = Operation::Read;
  base_config_.memory_range = 256 * num_ops;
  base_config_.min_io_chunk_size = TEST_CHUNK_SIZE;
  base_config_.run_time = 1;

  BenchmarkConfig config_one = base_config_;
  BenchmarkConfig config_two = base_config_;

  config_one.exec_mode = Mode::Sequential;
  config_two.exec_mode = Mode::Random;
  config_two.number_operations = num_ops;

  base_executions_.reserve(2);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(config_one));
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(config_two));

  base_results_.reserve(2);
  base_results_.push_back(std::make_unique<BenchmarkResult>(config_one));
  base_results_.push_back(std::make_unique<BenchmarkResult>(config_two));

  ParallelBenchmark bm{
      bm_name_, "sub_bm_1", "sub_bm_2", config_one, config_two, std::move(base_executions_), std::move(base_results_)};
  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result_one = *bm.get_benchmark_results()[0];
  const BenchmarkResult& result_two = *bm.get_benchmark_results()[1];

  const std::vector<ExecutionDuration>& all_durations_one = result_one.total_operation_durations;
  const std::vector<ExecutionDuration>& all_durations_two = result_two.total_operation_durations;
  ASSERT_EQ(all_durations_one.size(), 1);
  EXPECT_GT(all_durations_one[0].end - all_durations_one[0].begin, std::chrono::seconds{1});
  ASSERT_EQ(all_durations_two.size(), 1);
  EXPECT_GT(all_durations_two[0].end - all_durations_two[0].begin, std::chrono::seconds{1});

  const std::vector<uint64_t>& all_sizes_one = result_one.total_operation_sizes;
  const std::vector<uint64_t>& all_sizes_two = result_two.total_operation_sizes;
  ASSERT_EQ(all_sizes_one.size(), 1);
  EXPECT_GT(all_sizes_one[0], 0);
  EXPECT_EQ(all_sizes_one[0] % TEST_CHUNK_SIZE, 0);  // can only increase in chunk-sized blocks
  ASSERT_EQ(all_sizes_two.size(), 1);
  EXPECT_GT(all_sizes_two[0], 0);
  EXPECT_EQ(all_sizes_two[0] % TEST_CHUNK_SIZE, 0);
}

TEST_F(BenchmarkTest, ResultsParallelSingleThreadMixed) {
  const size_t num_ops = TEST_FILE_SIZE / 256;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.memory_range = TEST_FILE_SIZE;
  base_config_.min_io_chunk_size = TEST_CHUNK_SIZE;
  base_config_.run_time = 1;

  BenchmarkConfig config_one = base_config_;
  BenchmarkConfig config_two = base_config_;

  config_one.exec_mode = Mode::Sequential;
  config_one.operation = Operation::Write;

  config_two.exec_mode = Mode::Random;
  config_two.operation = Operation::Read;
  config_two.number_operations = num_ops;

  base_executions_.reserve(2);
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(config_one));
  base_executions_.push_back(std::make_unique<BenchmarkExecution>(config_two));

  base_results_.reserve(2);
  base_results_.push_back(std::make_unique<BenchmarkResult>(config_one));
  base_results_.push_back(std::make_unique<BenchmarkResult>(config_two));

  ParallelBenchmark bm{
      bm_name_, "sub_bm_1", "sub_bm_2", config_one, config_two, std::move(base_executions_), std::move(base_results_)};
  bm.create_data_files();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result_one = *bm.get_benchmark_results()[0];
  const BenchmarkResult& result_two = *bm.get_benchmark_results()[1];

  const std::vector<ExecutionDuration>& all_durations_one = result_one.total_operation_durations;
  const std::vector<ExecutionDuration>& all_durations_two = result_two.total_operation_durations;
  ASSERT_EQ(all_durations_one.size(), 1);
  EXPECT_GT(all_durations_one[0].end - all_durations_one[0].begin, std::chrono::seconds{1});
  ASSERT_EQ(all_durations_two.size(), 1);
  EXPECT_GT(all_durations_two[0].end - all_durations_two[0].begin, std::chrono::seconds{1});

  const std::vector<uint64_t>& all_sizes_one = result_one.total_operation_sizes;
  const std::vector<uint64_t>& all_sizes_two = result_two.total_operation_sizes;
  ASSERT_EQ(all_sizes_one.size(), 1);
  EXPECT_GT(all_sizes_one[0], 0);
  EXPECT_EQ(all_sizes_one[0] % TEST_CHUNK_SIZE, 0);  // can only increase in chunk-sized blocks
  ASSERT_EQ(all_sizes_two.size(), 1);
  EXPECT_GT(all_sizes_two[0], 0);
  EXPECT_EQ(all_sizes_two[0] % TEST_CHUNK_SIZE, 0);

  check_file_written(bm.get_pmem_file(0), TEST_FILE_SIZE);
}

#endif

}  // namespace perma
