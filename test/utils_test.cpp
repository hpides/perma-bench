#include "utils.hpp"

#include <sys/mman.h>

#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"

namespace perma {

constexpr auto FILE_SIZE = 8388608u;  // 8 MiB

class UtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmp_file_name_create = std::tmpnam(nullptr);
    std::ofstream temp_stream_create{tmp_file_name_create};
    temp_stream_create.close();

    tmp_file_name_read = std::tmpnam(nullptr);
    std::ofstream temp_stream_read{tmp_file_name_read};
    temp_stream_read.close();
    std::filesystem::resize_file(tmp_file_name_read, FILE_SIZE);

    internal::setPMEM_MAP_FLAGS(MAP_SHARED);
  }

  std::filesystem::path tmp_file_name_create;
  std::filesystem::path tmp_file_name_read;
};

/**
 * Verifies whether the first 100'000 zipfian generated values are in between the given boundaries.
 */
TEST_F(UtilsTest, ZipfBound) {
  for (uint32_t i = 0; i < 100'000; i++) {
    const uint64_t value = zipf(0.99, 1000);
    EXPECT_GE(value, 0);
    EXPECT_LT(value, 1000);
  }
}

/**
 * Verifies whether the memory mapped file is the same size as the file.
 */
TEST_F(UtilsTest, ReadMapFileCorrectSizePmem) { EXPECT_NO_THROW(map_file(tmp_file_name_read, false, FILE_SIZE)); }

/**
 * Verifies whether the memory mapped file can be mapped without a backing file.
 */
TEST_F(UtilsTest, ReadMapFileCorrectSizeDRAM) { EXPECT_NO_THROW(map_file(tmp_file_name_read, true, FILE_SIZE)); }

/**
 * Verifies whether the created memory mapped file is of the correct size.
 */
TEST_F(UtilsTest, CreateMapFileSize) {
  ASSERT_NO_THROW(create_file(tmp_file_name_create, false, FILE_SIZE));
  size_t file_size = std::filesystem::file_size(tmp_file_name_create);
  EXPECT_EQ(file_size, FILE_SIZE);
}

}  // namespace perma
