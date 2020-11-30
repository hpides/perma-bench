#include "utils.hpp"

#include <fstream>

#include "gtest/gtest.h"

namespace perma {

constexpr auto FILE_SIZE = 8388608u;  // 8 MiB

class UtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmp_file_name_create = std::tmpnam(nullptr);
    create_file = std::fopen(tmp_file_name_create.c_str(), "w+");
    tmp_file_name_read = std::tmpnam(nullptr);
    read_file = std::fopen(tmp_file_name_read.c_str(), "w+");

    std::fseek(read_file, FILE_SIZE - 1, SEEK_SET);
    std::fputc('a', read_file);
    std::fflush(read_file);
  }

  void TearDown() {
    std::fclose(create_file);
    std::fclose(read_file);
  }

  std::FILE* create_file;
  std::FILE* read_file;
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
TEST_F(UtilsTest, ReadMapFileCorrectSize) { EXPECT_NO_THROW(map_pmem_file(tmp_file_name_read, FILE_SIZE)); }

/**
 * Verifies whether the memory mapped file is not the same size as the file.
 */
TEST_F(UtilsTest, ReadMapFileIncorrectSize) {
  EXPECT_THROW(map_pmem_file(tmp_file_name_read, FILE_SIZE - 1), std::runtime_error);
}

/**
 * Verifies whether the created memory mapped file is of the correct size.
 */
TEST_F(UtilsTest, CreateMapFileSize) {
  ASSERT_NO_THROW(create_pmem_file(tmp_file_name_create, FILE_SIZE));
  size_t file_size = std::filesystem::file_size(tmp_file_name_create);
  EXPECT_EQ(file_size, FILE_SIZE);
}

}  // namespace perma
