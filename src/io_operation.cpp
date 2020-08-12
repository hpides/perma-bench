#include "io_operation.hpp"

#include <libpmem.h>
#include <vector>
#include <random>

namespace nvmbm {

ActiveIoOperation::ActiveIoOperation(void* startAddr, void* endAddr, const uint32_t numberOps,
                                     const uint32_t accessSize, const bool random)
    : start_addr_(startAddr), end_addr_(endAddr), number_ops_(numberOps), access_size_(accessSize), random_(random) {
  read_addresses = std::vector<char*>{number_ops_};

  char* start_addr = static_cast<char*>(start_addr_);
  char* end_addr = static_cast<char*>(end_addr_);

  if (random_) {
    const ptrdiff_t range = end_addr - start_addr;
    const uint32_t num_accesses_in_range = range / access_size_;

    std::random_device rnd_device;
    std::default_random_engine rnd_generator{rnd_device()};
    std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range);

    // Random read
    for (uint32_t op = 0; op < number_ops_; ++op) {
      read_addresses[op] = start_addr + (access_distribution(rnd_generator) * access_size_);
    }
  } else {
    // Sequential read
    for (uint32_t op = 0; op < number_ops_; ++op) {
      read_addresses[op] = start_addr + (op * access_size_);
    }
  }
}

bool ActiveIoOperation::is_active() {
    return true;
}

void Pause::run() {
    std::this_thread::sleep_for(std::chrono::microseconds(length_));
}

void Read::run() {
    for (char* addr : read_addresses) {
        const char* access_end_addr = addr + access_size_;
        for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += 64) {
            // Read 512 Bit / 64 Byte
            asm volatile(
            "mov       %[addr],     %%rsi  \n"
            "vmovntdqa 0*64(%%rsi), %%zmm0 \n"
            :
            : [addr] "r" (mem_addr)
            );
            asm volatile("mfence \n");
        }
    }
}

void Write::run() {
    pmem_persist(nullptr, 10);
}
}  // namespace nvmbm