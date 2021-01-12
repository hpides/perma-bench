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

  template <class WriteFn>
  void run_write_test(WriteFn write_fn) {
    const size_t num_writes = TMP_FILE_SIZE / ACCESS_SIZE;
    const char* last_op = addr + TMP_FILE_SIZE;
    std::vector<char*> op_addresses{};
    op_addresses.reserve(num_writes);

    for (char* write_addr = addr; write_addr < last_op; write_addr += ACCESS_SIZE) {
      op_addresses.emplace_back(write_addr);
    }

    write_fn(op_addresses, ACCESS_SIZE);
    ASSERT_EQ(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
    check_file_written(temp_file_, TMP_FILE_SIZE);
  }

  std::filesystem::path temp_file_;
  char* addr;
  int64_t fd;
};

#ifdef HAS_AVX
TEST_F(ReadWriteTest, SIMDNoneWrite_64) { run_write_test(rw_ops::simd_write_none); }

TEST_F(ReadWriteTest, SIMDNoneWrite_128) { run_write_test(rw_ops::simd_write_none_128); }

TEST_F(ReadWriteTest, SIMDNoneWrite_256) { run_write_test(rw_ops::simd_write_none_256); }

TEST_F(ReadWriteTest, SIMDNoneWrite_512) { run_write_test(rw_ops::simd_write_none_512); }

#ifdef HAS_CLWB
TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite_64) { run_write_test(rw_ops::simd_write_clwb); }

TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite_128) { run_write_test(rw_ops::simd_write_clwb_128); }

TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite_256) { run_write_test(rw_ops::simd_write_clwb_256); }

TEST_F(ReadWriteTest, SIMDCacheLineWriteBackWrite_512) { run_write_test(rw_ops::simd_write_clwb_512); }
#endif

TEST_F(ReadWriteTest, SIMDNonTemporalWrite_64) { run_write_test(rw_ops::simd_write_nt); }

TEST_F(ReadWriteTest, SIMDNonTemporalWrite_128) { run_write_test(rw_ops::simd_write_nt_128); }

TEST_F(ReadWriteTest, SIMDNonTemporalWrite_256) { run_write_test(rw_ops::simd_write_nt_256); }

TEST_F(ReadWriteTest, SIMDNonTemporalWrite_512) { run_write_test(rw_ops::simd_write_nt_512); }
#endif

TEST_F(ReadWriteTest, MOVNoneWrite) { run_write_test(rw_ops::mov_write_none); }

#ifdef HAS_CLWB
TEST_F(ReadWriteTest, MOVCacheLineWriteBackWrite) { run_write_test(rw_ops::mov_write_clwb); }
#endif

TEST_F(ReadWriteTest, MOVNonTemporalWrite) { run_write_test(rw_ops::mov_write_nt); }

}  // namespace perma
