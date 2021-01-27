#include <benchmark.hpp>
#include <fstream>

#include "gtest/gtest.h"
#include "test_utils.hpp"

namespace perma {

constexpr size_t PMEM_FILE_SIZE = 1048576;  // 1 MiB

// Duplicate this instead of using global constant so that we notice when it is changed.
const size_t TEST_IO_OP_CHUNK_SIZE = 16 * 1024u;

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
  EXPECT_EQ(&thread_config.config, &bm.get_benchmark_config());

  const std::vector<std::vector<internal::Latency>>& latencies = bm.get_benchmark_result().latencies;
  ASSERT_EQ(latencies.size(), 1);
  EXPECT_EQ(thread_config.latencies->data(), latencies[0].data());

  bm.get_benchmark_result().config.validate();
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

  const std::vector<std::vector<internal::Latency>>& latencies = bm.get_benchmark_result().latencies;
  ASSERT_EQ(latencies.size(), num_threads);
  EXPECT_EQ(thread_config0.latencies->data(), latencies[0].data());
  EXPECT_EQ(thread_config1.latencies->data(), latencies[1].data());
  EXPECT_EQ(thread_config2.latencies->data(), latencies[2].data());
  EXPECT_EQ(thread_config3.latencies->data(), latencies[3].data());

  // These values are the same for all threads
  for (const ThreadRunConfig& tc : thread_configs) {
    EXPECT_EQ(tc.num_threads_per_partition, 2);
    EXPECT_EQ(tc.partition_size, partition_size);
    EXPECT_EQ(tc.num_ops, PMEM_FILE_SIZE / num_threads / 512);
    EXPECT_EQ(&tc.config, &bm.get_benchmark_config());
  }
  bm.get_benchmark_result().config.validate();
}

TEST_F(BenchmarkTest, RunSingeThreadRead) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = 256 * num_ops;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), 1);
  const std::vector<internal::Latency>& latencies = all_latencies[0];

  ASSERT_EQ(latencies.size(), num_chunks);
  for (const internal::Latency latency : latencies) {
    EXPECT_EQ(latency.op_type, internal::Read);
    EXPECT_GE(latency.duration, 0);
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);
}

TEST_F(BenchmarkTest, RunSingeThreadWrite) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t total_size = 256 * num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 0;
  base_config_.write_ratio = 1;
  base_config_.total_memory_range = total_size;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), 1);
  const std::vector<internal::Latency>& latencies = all_latencies[0];

  ASSERT_EQ(latencies.size(), num_chunks);
  for (const internal::Latency latency : latencies) {
    EXPECT_EQ(latency.op_type, internal::Write);
    EXPECT_GE(latency.duration, 0);
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);

  check_file_written(bm.get_pmem_file(), total_size);
}

TEST_F(BenchmarkTest, RunSingeThreadMixed) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 0.5;
  base_config_.write_ratio = 0.5;
  base_config_.number_operations = num_ops;
  base_config_.exec_mode = internal::Random;
  base_config_.total_memory_range = 256 * num_ops;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), 1);
  const std::vector<internal::Latency>& latencies = all_latencies[0];

  ASSERT_EQ(latencies.size(), num_chunks);
  for (const internal::Latency latency : latencies) {
    EXPECT_TRUE(latency.op_type == internal::Write || latency.op_type == internal::Read);
    EXPECT_GE(latency.duration, 0);
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);
}

TEST_F(BenchmarkTest, RunMultiThreadRead) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 1024;
  const size_t num_chunks = 32;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = 1024 * num_ops;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), num_threads);
  for (const std::vector<internal::Latency>& latencies : all_latencies) {
    ASSERT_EQ(latencies.size(), num_chunks / num_threads);
    for (const internal::Latency latency : latencies) {
      EXPECT_EQ(latency.op_type, internal::Read);
      EXPECT_GE(latency.duration, 0);
    }
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);
}

TEST_F(BenchmarkTest, RunMultiThreadWrite) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 128;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 16;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = total_size;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), num_threads);
  for (const std::vector<internal::Latency>& latencies : all_latencies) {
    ASSERT_EQ(latencies.size(), num_chunks / num_threads);
    for (const internal::Latency latency : latencies) {
      EXPECT_EQ(latency.op_type, internal::Read);
      EXPECT_GE(latency.duration, 0);
    }
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);

  check_file_written(bm.get_pmem_file(), total_size);
}

TEST_F(BenchmarkTest, RunMultiThreadReadDesc) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 1024;
  const size_t num_chunks = 32;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 4;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = 1024 * num_ops;
  base_config_.exec_mode = internal::Sequential_Desc;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), num_threads);
  for (const std::vector<internal::Latency>& latencies : all_latencies) {
    ASSERT_EQ(latencies.size(), num_chunks / num_threads);
    for (const internal::Latency latency : latencies) {
      EXPECT_EQ(latency.op_type, internal::Read);
      EXPECT_GE(latency.duration, 0);
    }
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);
}

TEST_F(BenchmarkTest, RunMultiThreadWriteDesc) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 128;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 16;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = total_size;
  base_config_.exec_mode = internal::Sequential_Desc;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Latency>>& all_latencies = result.latencies;
  ASSERT_EQ(all_latencies.size(), num_threads);
  for (const std::vector<internal::Latency>& latencies : all_latencies) {
    ASSERT_EQ(latencies.size(), num_chunks / num_threads);
    for (const internal::Latency latency : latencies) {
      EXPECT_EQ(latency.op_type, internal::Read);
      EXPECT_GE(latency.duration, 0);
    }
  }
  ASSERT_EQ(result.raw_measurements.size(), 0);

  check_file_written(bm.get_pmem_file(), total_size);
}

TEST_F(BenchmarkTest, RunMultiThreadWriteRaw) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 128;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 16;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.raw_results = true;
  base_config_.access_size = 512;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = total_size;
  Benchmark bm{bm_name_, base_config_};
  bm.create_data_file();
  bm.set_up();
  bm.run();

  const BenchmarkResult& result = bm.get_benchmark_result();

  const std::vector<std::vector<internal::Measurement>>& all_measurements = result.raw_measurements;
  ASSERT_EQ(all_measurements.size(), num_threads);
  for (const std::vector<internal::Measurement>& measurements : all_measurements) {
    ASSERT_EQ(measurements.size(), num_chunks / num_threads);
    for (const internal::Measurement measurement : measurements) {
      EXPECT_EQ(measurement.latency.op_type, internal::Read);
      EXPECT_GE(measurement.latency.duration, 0);
    }
  }
  ASSERT_EQ(result.latencies.size(), 0);

  check_file_written(bm.get_pmem_file(), total_size);
}

TEST_F(BenchmarkTest, ResultsSingleThreadRead) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t total_size = 256 * num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = total_size;

  BenchmarkResult bm_result{base_config_};
  std::vector<internal::Latency> latencies{};
  latencies.reserve(num_ops);
  for (size_t i = 0; i < num_ops; ++i) {
    latencies.emplace_back(ops_per_chunk * 100, internal::OpType::Read);
  }
  bm_result.latencies.emplace_back(latencies);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 1);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  EXPECT_EQ(bandwidth_json.at("read").get<double>(), 2.56);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 100.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 100.0);
}

TEST_F(BenchmarkTest, ResultsSingleThreadWrite) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 256;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t total_size = 256 * num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 256;
  base_config_.read_ratio = 0;
  base_config_.write_ratio = 1;
  base_config_.total_memory_range = total_size;

  BenchmarkResult bm_result{base_config_};
  std::vector<internal::Latency> latencies{};
  latencies.reserve(num_ops);
  for (size_t i = 0; i < num_ops; ++i) {
    latencies.emplace_back(ops_per_chunk * 100, internal::OpType::Write);
  }
  bm_result.latencies.emplace_back(latencies);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 1);
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_EQ(bandwidth_json.at("write").get<double>(), 2.56);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 100.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 100.0);
}

TEST_F(BenchmarkTest, ResultsSingleThreadMixed) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 8;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t total_size = 512 * num_ops;
  base_config_.number_operations = num_ops;
  base_config_.number_threads = 1;
  base_config_.access_size = 512;
  base_config_.read_ratio = 0.5;
  base_config_.write_ratio = 0.5;
  base_config_.total_memory_range = total_size;
  base_config_.exec_mode = internal::Random;

  BenchmarkResult bm_result{base_config_};
  std::vector<internal::Latency> latencies{};
  latencies.reserve(num_ops);
  for (size_t i = 0; i < num_ops; ++i) {
    if (i % 2 == 0) {
      latencies.emplace_back(200 * ops_per_chunk, internal::OpType::Write);
    } else {
      latencies.emplace_back(100 * ops_per_chunk, internal::OpType::Read);
    }
  }
  bm_result.latencies.emplace_back(latencies);

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 2);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_EQ(bandwidth_json.at("read").get<double>(), 5.12);
  EXPECT_EQ(bandwidth_json.at("write").get<double>(), 2.56);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  ASSERT_JSON_TRUE(duration_json, contains("std_dev"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 150.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 100.0);
  EXPECT_EQ(duration_json.at("std_dev").get<double>(), 50.0);
}

TEST_F(BenchmarkTest, ResultsMultiThreadRead) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 1024;
  const size_t num_chunks = 64;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 4;
  const size_t num_ops_per_thread = num_ops / num_threads;
  const size_t total_size = 1024 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 1024;
  base_config_.read_ratio = 1;
  base_config_.write_ratio = 0;
  base_config_.total_memory_range = total_size;

  BenchmarkResult bm_result{base_config_};
  for (size_t thread = 0; thread < num_threads; ++thread) {
    std::vector<internal::Latency> latencies{};
    for (size_t i = 0; i < num_ops_per_thread; ++i) {
      latencies.emplace_back((100 + (thread * 10)) * ops_per_chunk, internal::OpType::Read);
    }
    bm_result.latencies.emplace_back(latencies);
  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 1);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  EXPECT_NEAR(bandwidth_json.at("read").get<double>(), 35.6173, 0.01);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 115.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 110.0);
}

TEST_F(BenchmarkTest, ResultsMultiThreadWrite) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 64;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 8;
  const size_t num_ops_per_thread = num_ops / num_threads;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.access_size = 512;
  base_config_.read_ratio = 0;
  base_config_.write_ratio = 1;
  base_config_.total_memory_range = total_size;

  BenchmarkResult bm_result{base_config_};
  for (size_t thread = 0; thread < num_threads; ++thread) {
    std::vector<internal::Latency> latencies{};
    for (size_t i = 0; i < num_ops_per_thread; ++i) {
      latencies.emplace_back((100 + (thread * 10)) * ops_per_chunk, internal::OpType::Write);
    }
    bm_result.latencies.emplace_back(latencies);
  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 1);
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_NEAR(bandwidth_json.at("write").get<double>(), 30.3407, 0.01);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 135.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 130.0);
}

TEST_F(BenchmarkTest, ResultsMultiThreadMixed) {
  const size_t ops_per_chunk = TEST_IO_OP_CHUNK_SIZE / 512;
  const size_t num_chunks = 64;
  const size_t num_ops = num_chunks * ops_per_chunk;
  const size_t num_threads = 16;
  const size_t num_ops_per_thread = num_ops / num_threads;
  const size_t total_size = 512 * num_ops;
  base_config_.number_threads = num_threads;
  base_config_.number_operations = num_ops;
  base_config_.access_size = 512;
  base_config_.read_ratio = 0.5;
  base_config_.write_ratio = 0.5;
  base_config_.total_memory_range = total_size;
  base_config_.exec_mode = internal::Random;

  BenchmarkResult bm_result{base_config_};
  for (size_t thread = 0; thread < num_threads; ++thread) {
    std::vector<internal::Latency> latencies{};
    for (size_t i = 0; i < num_ops_per_thread; ++i) {
      if (i % 2 == 0) {
        latencies.emplace_back(500 * ops_per_chunk, internal::OpType::Write);
      } else {
        latencies.emplace_back(400 * ops_per_chunk, internal::OpType::Read);
      }
    }
    bm_result.latencies.emplace_back(latencies);
  }

  const nlohmann::json& result_json = bm_result.get_result_as_json();
  ASSERT_JSON_EQ(result_json, size(), 2);
  ASSERT_JSON_TRUE(result_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(result_json, contains("duration"));

  const nlohmann::json& bandwidth_json = result_json["bandwidth"];
  ASSERT_JSON_EQ(bandwidth_json, size(), 2);
  ASSERT_JSON_TRUE(bandwidth_json, contains("read"));
  ASSERT_JSON_TRUE(bandwidth_json, contains("write"));
  ASSERT_JSON_TRUE(bandwidth_json, at("read").is_number());
  ASSERT_JSON_TRUE(bandwidth_json, at("write").is_number());
  EXPECT_NEAR(bandwidth_json.at("read").get<double>(), 20.48, 0.01);
  EXPECT_NEAR(bandwidth_json.at("write").get<double>(), 16.384, 0.01);

  const nlohmann::json& duration_json = result_json["duration"];
  ASSERT_JSON_EQ(duration_json, size(), 12);
  ASSERT_JSON_TRUE(duration_json, contains("avg"));
  ASSERT_JSON_TRUE(duration_json, contains("median"));
  ASSERT_JSON_TRUE(duration_json, contains("std_dev"));
  EXPECT_EQ(duration_json.at("avg").get<double>(), 450.0);
  EXPECT_EQ(duration_json.at("median").get<double>(), 400.0);
  EXPECT_EQ(duration_json.at("std_dev").get<double>(), 50.0);
}

}  // namespace perma
