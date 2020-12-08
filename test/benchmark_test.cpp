#include <benchmark.hpp>
#include <fstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace perma {

constexpr size_t PMEM_FILE_SIZE = 131072;  // 128 KiB

class BenchmarkTest : public ::testing::Test {
 protected:
  void SetUp() override {
    base_config_.pmem_directory = temp_dir_;
    base_config_.total_memory_range = PMEM_FILE_SIZE;
  }

  BenchmarkConfig base_config_{};
  const std::string bm_name_ = "test_bm";
  const std::filesystem::path temp_dir_ = std::filesystem::temp_directory_path();
};

TEST_F(BenchmarkTest, CreateBenchmark) {
  ASSERT_NO_THROW(Benchmark("test_bm1", base_config_));
  ASSERT_NO_THROW(Benchmark("test_bm2", base_config_, "/tmp/foo/bar"));
}

TEST_F(BenchmarkTest, CreateNewDataFile) {
  base_config_.read_ratio = 0;
  base_config_.write_ratio = 1;
  Benchmark bm{bm_name_, base_config_};
  const std::filesystem::path pmem_file = bm.get_pmem_file();

  ASSERT_FALSE(std::filesystem::exists(pmem_file));

  bm.create_data_file();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), PMEM_FILE_SIZE);
  EXPECT_TRUE(bm.owns_pmem_file());
  EXPECT_NE(bm.get_pmem_data(), nullptr);
}

TEST_F(BenchmarkTest, CreateExistingDataFile) {
  base_config_.read_ratio = 0;
  base_config_.write_ratio = 1;

  const std::string test_string = "test123456789";
  const std::filesystem::path existing_pmem_file = generate_random_file_name(temp_dir_);
  std::ofstream temp_stream{existing_pmem_file, std::ios::out};
  temp_stream << test_string << std::endl;
  temp_stream.flush();
  temp_stream.close();
  std::filesystem::resize_file(existing_pmem_file, PMEM_FILE_SIZE);

  Benchmark bm{bm_name_, base_config_, existing_pmem_file};
  const std::filesystem::path pmem_file = bm.get_pmem_file();

  ASSERT_EQ(pmem_file, existing_pmem_file);
  ASSERT_TRUE(std::filesystem::exists(pmem_file));

  bm.create_data_file();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), PMEM_FILE_SIZE);
  EXPECT_FALSE(bm.owns_pmem_file());
  ASSERT_NE(bm.get_pmem_data(), nullptr);
  const char* pmem_data = bm.get_pmem_data();
  const std::string_view mapped_data{pmem_data, test_string.size()};
  EXPECT_EQ(mapped_data, test_string);

  bm.tear_down(true);
}

TEST_F(BenchmarkTest, CreateReadDataFile) {
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  Benchmark bm{bm_name_, base_config_};
  const std::filesystem::path pmem_file = bm.get_pmem_file();

  ASSERT_FALSE(std::filesystem::exists(pmem_file));

  bm.create_data_file();

  ASSERT_TRUE(std::filesystem::exists(pmem_file));
  ASSERT_TRUE(std::filesystem::is_regular_file(pmem_file));
  EXPECT_EQ(std::filesystem::file_size(pmem_file), PMEM_FILE_SIZE);
  EXPECT_TRUE(bm.owns_pmem_file());

  const char* pmem_data = bm.get_pmem_data();
  ASSERT_NE(pmem_data, nullptr);
  const size_t data_size = rw_ops::CACHE_LINE_SIZE;
  const std::string_view mapped_data{pmem_data, data_size};
  const std::string_view expected_data{rw_ops::WRITE_DATA, data_size};
  EXPECT_EQ(mapped_data, expected_data);
}

TEST_F(BenchmarkTest, SetUpSingleThread) {
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();

  const std::vector<ThreadRunConfig>& thread_configs = bm.get_thread_configs();
  ASSERT_EQ(thread_configs.size(), 1);
  const ThreadRunConfig& thread_config = thread_configs[0];

  EXPECT_EQ(thread_config.thread_num, 0);
  EXPECT_EQ(thread_config.num_threads_per_partition, 1);
  EXPECT_EQ(thread_config.partition_size, PMEM_FILE_SIZE);
  EXPECT_EQ(thread_config.num_ops, PMEM_FILE_SIZE / 256);
  EXPECT_EQ(thread_config.partition_start_addr, bm.get_pmem_data());

  const std::vector<std::vector<internal::Latency>>& latencies = bm.get_latencies();
  ASSERT_EQ(latencies.size(), 1);
  EXPECT_EQ(thread_config.latencies.data(), latencies[0].data());
}

TEST_F(BenchmarkTest, SetUpMultiThread) {
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.number_partitions = 2;
  base_config_.access_size = 512;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();

  const size_t partition_size = PMEM_FILE_SIZE / 2;
  const std::vector<ThreadRunConfig>& thread_configs = bm.get_thread_configs();
  ASSERT_EQ(thread_configs.size(), num_threads);
  const ThreadRunConfig& thread_config0 = thread_configs[0];
  const ThreadRunConfig& thread_config1 = thread_configs[1];
  const ThreadRunConfig& thread_config2 = thread_configs[2];
  const ThreadRunConfig& thread_config3 = thread_configs[3];

  EXPECT_EQ(thread_config0.thread_num, 0);
  EXPECT_EQ(thread_config1.thread_num, 1);
  EXPECT_EQ(thread_config2.thread_num, 0);
  EXPECT_EQ(thread_config3.thread_num, 1);

  EXPECT_EQ(thread_config0.partition_start_addr, bm.get_pmem_data());
  EXPECT_EQ(thread_config1.partition_start_addr, bm.get_pmem_data());
  EXPECT_EQ(thread_config2.partition_start_addr, bm.get_pmem_data() + partition_size);
  EXPECT_EQ(thread_config3.partition_start_addr, bm.get_pmem_data() + partition_size);

  const std::vector<std::vector<internal::Latency>>& latencies = bm.get_latencies();
  ASSERT_EQ(latencies.size(), num_threads);
  EXPECT_EQ(thread_config0.latencies.data(), latencies[0].data());
  EXPECT_EQ(thread_config1.latencies.data(), latencies[1].data());
  EXPECT_EQ(thread_config2.latencies.data(), latencies[2].data());
  EXPECT_EQ(thread_config3.latencies.data(), latencies[3].data());

  // These values are the same for all threads
  for (const ThreadRunConfig& tc : thread_configs) {
    EXPECT_EQ(tc.num_threads_per_partition, 2);
    EXPECT_EQ(tc.partition_size, partition_size);
    EXPECT_EQ(tc.num_ops, PMEM_FILE_SIZE / num_threads / 512);
  }
}

TEST_F(BenchmarkTest, RunSingeThread) {
  const size_t num_ops = 8;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = 256 * num_ops;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const std::vector<std::vector<internal::Latency>>& all_latencies = bm.get_latencies();
  ASSERT_EQ(all_latencies.size(), 1);
  const std::vector<internal::Latency>& latencies = all_latencies[0];

  ASSERT_EQ(latencies.size(), num_ops);
  uint64_t total_duration = 0;
  for (const internal::Latency latency : latencies) {
    EXPECT_EQ(latency.op_type, internal::Read);
    EXPECT_GT(latency.duration, 0);
    total_duration += latency.duration;
  }


}

TEST_F(BenchmarkTest, RunMultiThread) {}

}  // namespace perma
