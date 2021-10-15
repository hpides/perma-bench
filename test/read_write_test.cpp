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

  using MultiWriteFn = void(const std::vector<char*>&);
  void run_multi_write_test(MultiWriteFn write_fn, const size_t access_size) {
    const size_t num_writes = TMP_FILE_SIZE / access_size;
    const char* last_op = addr + TMP_FILE_SIZE;
    std::vector<char*> op_addresses{};
    op_addresses.reserve(num_writes);

    for (char* write_addr = addr; write_addr < last_op; write_addr += access_size) {
      op_addresses.emplace_back(write_addr);
    }

    write_fn(op_addresses);
    ASSERT_EQ(msync(addr, TMP_FILE_SIZE, MS_SYNC), 0);
    check_file_written(temp_file_, TMP_FILE_SIZE);
  }

  using SingleWriteFn = void(char*);
  void run_single_write_test(SingleWriteFn write_fn, const size_t access_size) {
    write_fn(addr);
    ASSERT_EQ(msync(addr, access_size, MS_SYNC), 0);
    check_file_written(temp_file_, TMP_FILE_SIZE, access_size);
  }

  std::filesystem::path temp_file_;
  char* addr;
  int64_t fd;
};

#ifdef HAS_AVX
TEST_F(ReadWriteTest, SingleSIMDNoneWrite_64) { run_single_write_test(rw_ops::simd_write_none_64, 64); }
TEST_F(ReadWriteTest, SingleSIMDNoneWrite_128) { run_single_write_test(rw_ops::simd_write_none_128, 128); }
TEST_F(ReadWriteTest, SingleSIMDNoneWrite_256) { run_single_write_test(rw_ops::simd_write_none_256, 256); }
TEST_F(ReadWriteTest, SingleSIMDNoneWrite_512) { run_single_write_test(rw_ops::simd_write_none_512, 512); }

TEST_F(ReadWriteTest, MultiSIMDNoneWrite_64) { run_multi_write_test(rw_ops::simd_write_none_64, 64); }
TEST_F(ReadWriteTest, MultiSIMDNoneWrite_128) { run_multi_write_test(rw_ops::simd_write_none_128, 128); }
TEST_F(ReadWriteTest, MultiSIMDNoneWrite_256) { run_multi_write_test(rw_ops::simd_write_none_256, 256); }
TEST_F(ReadWriteTest, MultiSIMDNoneWrite_512) { run_multi_write_test(rw_ops::simd_write_none_512, 512); }

#ifdef HAS_CLWB
TEST_F(ReadWriteTest, SingleSIMDClwbWrite_64) { run_single_write_test(rw_ops::simd_write_clwb_64, 64); }
TEST_F(ReadWriteTest, SingleSIMDClwbWrite_128) { run_single_write_test(rw_ops::simd_write_clwb_128, 128); }
TEST_F(ReadWriteTest, SingleSIMDClwbWrite_256) { run_single_write_test(rw_ops::simd_write_clwb_256, 256); }
TEST_F(ReadWriteTest, SingleSIMDClwbWrite_512) { run_single_write_test(rw_ops::simd_write_clwb_512, 512); }

TEST_F(ReadWriteTest, MultiSIMDClwbWrite_64) { run_multi_write_test(rw_ops::simd_write_clwb_64, 64); }
TEST_F(ReadWriteTest, MultiSIMDClwbWrite_128) { run_multi_write_test(rw_ops::simd_write_clwb_128, 128); }
TEST_F(ReadWriteTest, MultiSIMDClwbWrite_256) { run_multi_write_test(rw_ops::simd_write_clwb_256, 256); }
TEST_F(ReadWriteTest, MultiSIMDClwbWrite_512) { run_multi_write_test(rw_ops::simd_write_clwb_512, 512); }
#endif

TEST_F(ReadWriteTest, SingleSIMDNonTemporalWrite_64) { run_single_write_test(rw_ops::simd_write_nt_64, 64); }
TEST_F(ReadWriteTest, SingleSIMDNonTemporalWrite_128) { run_single_write_test(rw_ops::simd_write_nt_128, 128); }
TEST_F(ReadWriteTest, SingleSIMDNonTemporalWrite_256) { run_single_write_test(rw_ops::simd_write_nt_256, 256); }
TEST_F(ReadWriteTest, SingleSIMDNonTemporalWrite_512) { run_single_write_test(rw_ops::simd_write_nt_512, 512); }

TEST_F(ReadWriteTest, MultiSIMDNonTemporalWrite_64) { run_multi_write_test(rw_ops::simd_write_nt_64, 64); }
TEST_F(ReadWriteTest, MultiSIMDNonTemporalWrite_128) { run_multi_write_test(rw_ops::simd_write_nt_128, 128); }
TEST_F(ReadWriteTest, MultiSIMDNonTemporalWrite_256) { run_multi_write_test(rw_ops::simd_write_nt_256, 256); }
TEST_F(ReadWriteTest, MultiSIMDNonTemporalWrite_512) { run_multi_write_test(rw_ops::simd_write_nt_512, 512); }
#endif

}  // namespace perma
