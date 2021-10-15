#include "utils.hpp"
#include "gtest/gtest.h"
#include <fstream>
#include "read_write_ops.hpp"

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

}  // namespace perma
