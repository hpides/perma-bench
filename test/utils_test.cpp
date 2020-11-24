#include "utils.hpp"

#include <fstream>

#include "gtest/gtest.h"

namespace perma {

constexpr auto FILE_SIZE = 104857600;  // 100 MiB

class UtilsTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    const std::filesystem::path tmp_path = std::filesystem::path("/tmp");
    create_file = tmp_path / "create.file";
    read_file = tmp_path / "read.file";
    std::ofstream file(read_file);
    file.seekp(104857600 - 1);
    file << 'a';
    file.close();
  }

  static void TearDownTestSuite() {
    std::filesystem::remove(read_file);
    std::filesystem::remove(create_file);
  }

  static std::filesystem::path create_file;
  static std::filesystem::path read_file;
};

std::filesystem::path UtilsTest::create_file = std::filesystem::path();
std::filesystem::path UtilsTest::read_file = std::filesystem::path();

/**
 * Verifies whether the first 100'00 zipfian generated values are in between the given boundaries.
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
TEST_F(UtilsTest, ReadMapFileCorrectSize) { EXPECT_NO_THROW(map_pmem_file(read_file, FILE_SIZE)); }

/**
 * Verifies whether the memory mapped file is not the same size as the file.
 */
TEST_F(UtilsTest, ReadMapFileIncorrectSize) {
  EXPECT_THROW(map_pmem_file(read_file, FILE_SIZE - 1), std::runtime_error);
}

/**
 * Verifies whether the created memory mapped file is of the correct size.
 */
TEST_F(UtilsTest, CreateMapFileSize) {
  create_pmem_file(create_file, FILE_SIZE);
  size_t file_size = std::filesystem::file_size(create_file);
  EXPECT_EQ(file_size, FILE_SIZE);
}

}  // namespace perma
