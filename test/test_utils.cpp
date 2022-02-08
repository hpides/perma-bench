#include "test_utils.hpp"

#include <gmock/gmock-matchers.h>

#include <fstream>

#include "gtest/gtest.h"
#include "read_write_ops.hpp"
#include "utils.hpp"

namespace perma {

void check_file_written(const std::filesystem::path& pmem_file, const size_t total_size, const size_t size_written) {
  ASSERT_EQ(std::filesystem::file_size(pmem_file), total_size);
  std::string data;
  std::ifstream pmem_stream{pmem_file};
  pmem_stream >> data;
  ASSERT_EQ(data.size(), total_size);

  char* const raw_data = data.data();
  const std::string_view expected_data{rw_ops::WRITE_DATA, rw_ops::CACHE_LINE_SIZE};
  for (size_t offset = 0; offset < size_written; offset += rw_ops::CACHE_LINE_SIZE) {
    ASSERT_EQ(std::string_view(raw_data + offset, rw_ops::CACHE_LINE_SIZE), expected_data)
        << "Failed at position " << std::to_string(offset);
  }

  if (size_written < total_size) {
    // Check that we did not write too much data
    ASSERT_NE(std::string_view(raw_data + size_written, rw_ops::CACHE_LINE_SIZE), expected_data)
        << "Wrote beyond access size " << std::to_string(size_written);
  }
}

void check_file_written(const std::filesystem::path& pmem_file, const size_t total_size) {
  check_file_written(pmem_file, total_size, total_size);
}

void check_json_result(const nlohmann::json& result_json, uint64_t total_bytes, double expected_bandwidth,
                       uint64_t num_threads, double expected_per_thread_bandwidth, double expected_per_thread_stddev) {
  ASSERT_JSON_EQ(result_json, size(), 1);
  ASSERT_JSON_TRUE(result_json, contains("results"));

  const nlohmann::json& results_json = result_json["results"];
  ASSERT_JSON_EQ(results_json, size(), 6);
  ASSERT_JSON_TRUE(results_json, contains("bandwidth"));
  ASSERT_JSON_TRUE(results_json, at("bandwidth").is_number());
  EXPECT_NEAR(results_json.at("bandwidth").get<double>(), expected_bandwidth, 0.001);

  ASSERT_JSON_TRUE(results_json, contains("thread_bandwidth_avg"));
  EXPECT_NEAR(results_json.at("thread_bandwidth_avg").get<double>(), expected_per_thread_bandwidth, 0.001);
  ASSERT_JSON_TRUE(results_json, contains("thread_bandwidth_std_dev"));
  EXPECT_NEAR(results_json.at("thread_bandwidth_std_dev").get<double>(), expected_per_thread_stddev, 0.001);

  ASSERT_JSON_TRUE(results_json, contains("execution_time"));
  EXPECT_GT(results_json.at("execution_time").get<double>(), 0);

  ASSERT_JSON_TRUE(results_json, contains("accessed_bytes"));
  EXPECT_EQ(results_json.at("accessed_bytes").get<double>(), total_bytes);

  ASSERT_JSON_TRUE(results_json, contains("threads"));
  EXPECT_EQ(results_json.at("threads").size(), num_threads);
}

}  // namespace perma
