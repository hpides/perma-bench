#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <read_write_ops.hpp>

#include "gtest/gtest.h"
#include "test_utils.hpp"

namespace perma {

constexpr size_t TMP_FILE_SIZE = 131072;  // 128 KiB
constexpr size_t ACCESS_SIZE = 512;       // 512 byte

class ReadWriteTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_file_ = std::tmpnam(nullptr);

    fd = open(temp_file_.c_str(), (O_CREAT | O_TRUNC | O_RDWR), (S_IRWXU | S_IRGRP | S_IROTH));
    ASSERT_GE(fd, 0);

    off_t position = lseek(fd, TMP_FILE_SIZE - 1, SEEK_SET);
    ASSERT_EQ(position, TMP_FILE_SIZE - 1);

    int8_t written_bytes = write(fd, "", 1);
    ASSERT_EQ(written_bytes, 1);

    addr = static_cast<char*>(mmap(nullptr, TMP_FILE_SIZE, PROT_WRITE, MAP_PRIVATE, fd, 0));
    ASSERT_NE(addr, MAP_FAILED);
  }

  void TearDown() override {
    munmap(addr, TMP_FILE_SIZE);
    close(fd);
  }

  std::filesystem::path temp_file_;
  char* addr;
  int64_t fd;
};

#ifdef HAS_AVX
TEST_F(ReadWriteTest, SIMDNoneWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::simd_write_none(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::simd_write_clwb(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, SIMDCacheLineFlushOptWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::simd_write_clflush(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, SIMDNonTemporalWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::simd_write_nt(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}
#endif

TEST_F(ReadWriteTest, MOVNoneWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::mov_write_none(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, MOVNonTemporalWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::mov_write_nt(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, MOVCacheLineWriteBackWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::mov_write_clwb(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

TEST_F(ReadWriteTest, MOVCacheLineFlushOptWrite) {
  const char* to = addr + TMP_FILE_SIZE;
  for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
    rw_ops::mov_write_clflush(mem_addr, ACCESS_SIZE);
  }
  ASSERT_GE(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
  check_file_written(temp_file_, TMP_FILE_SIZE);
}

}  // namespace perma
