#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fstream>
#include <read_write_ops.hpp>
#include <utils.hpp>

#include "gtest/gtest.h"
#include "test_utils.hpp"

namespace perma {

constexpr size_t TMP_FILE_SIZE = 131072;  // 128 KiB
constexpr size_t ACCESS_SIZE = 512;       // 512 byte

typedef void test_write_fn(char*, const size_t);

class ReadWriteTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_file_ = generate_random_file_name(std::filesystem::temp_directory_path());

    std::ofstream temp_stream{temp_file_};
    temp_stream.close();
    std::filesystem::resize_file(temp_file_, TMP_FILE_SIZE);

    fd = open(temp_file_.c_str(), O_RDWR, S_IRWXU);
    ASSERT_GE(fd, 0);

    addr = static_cast<char*>(mmap(nullptr, TMP_FILE_SIZE, PROT_WRITE, MAP_SHARED, fd, 0));
    ASSERT_NE(addr, MAP_FAILED);
  }

  void TearDown() override {
    munmap(addr, TMP_FILE_SIZE);
    close(fd);
  }

  void run_write_test(const test_write_fn fn) {
    const char* to = addr + TMP_FILE_SIZE;
    for (char* mem_addr = addr; mem_addr < to; mem_addr += ACCESS_SIZE) {
      fn(mem_addr, ACCESS_SIZE);
    }
    ASSERT_EQ(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
    check_file_written(temp_file_, TMP_FILE_SIZE);
  }

  std::filesystem::path temp_file_;
  char* addr;
  int64_t fd;
};

#ifdef HAS_AVX
TEST_F(ReadWriteTest, SIMDNoneWrite) { run_write_test(rw_ops::simd_write_none); }

TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite) { run_write_test(rw_ops::simd_write_clwb); }

TEST_F(ReadWriteTest, SIMDCacheLineFlushOptWrite) { run_write_test(rw_ops::simd_write_clflush); }

TEST_F(ReadWriteTest, SIMDNonTemporalWrite) { run_write_test(rw_ops::simd_write_nt); }
#endif

TEST_F(ReadWriteTest, MOVNoneWrite) { run_write_test(rw_ops::mov_write_none); }

TEST_F(ReadWriteTest, MOVCacheLineWriteBackWrite) { run_write_test(rw_ops::mov_write_clwb); }

TEST_F(ReadWriteTest, MOVCacheLineFlushOptWrite) { run_write_test(rw_ops::mov_write_clflush); }

TEST_F(ReadWriteTest, MOVNonTemporalWrite) { run_write_test(rw_ops::mov_write_nt); }

}  // namespace perma
