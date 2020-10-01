#include "io_operation.hpp"

#include <immintrin.h>
#include <libpmem.h>

#include <iostream>
#include <random>
#include <vector>

namespace perma {

// This tells the compiler to keep whatever x is and not optimize it away.
// Inspired by Google Benchmark's DoNotOptimize and this talk
// https://youtu.be/nXaxk27zwlk?t=2441.
#define KEEP(x) asm volatile("" : : "g"(x) : "memory")

ActiveIoOperation::ActiveIoOperation(char* startAddr, char* endAddr, const uint32_t numberOps,
                                     const uint32_t accessSize, const internal::Mode mode)
    : start_addr_(startAddr), end_addr_(endAddr), number_ops_(numberOps), access_size_(accessSize), mode_(mode) {
  op_addresses_ = std::vector<char*>{number_ops_};

  if (mode_ == internal::Mode::Random) {
    const ptrdiff_t range = end_addr_ - start_addr_;
    const uint32_t num_accesses_in_range = range / access_size_;

    std::random_device rnd_device;
    std::default_random_engine rnd_generator{rnd_device()};
    std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range);

    // Random read
    for (uint32_t op = 0; op < number_ops_; ++op) {
      op_addresses_[op] = start_addr_ + (access_distribution(rnd_generator) * access_size_);
    }
  } else {
    // Sequential read
    for (uint32_t op = 0; op < number_ops_; ++op) {
      op_addresses_[op] = start_addr_ + (op * access_size_);
    }
  }
}

bool ActiveIoOperation::is_active() const { return true; }

void Pause::run() {
  std::cout << "Sleeping for " << length_ << " microseconds" << std::endl;
  std::this_thread::sleep_for(std::chrono::microseconds(length_));
  std::cout << "Slept for " << length_ << " microseconds" << std::endl;
}

void Read::run() {
  for (char* addr : op_addresses_) {
    const char* access_end_addr = addr + access_size_;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += internal::CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte) and do not optimize it out.
      //      KEEP(_mm512_stream_load_si512(mem_addr));
      __m512i x = _mm512_stream_load_si512(mem_addr);
      std::cout << "Read: " << (std::string{(char*)&x, 64}) << std::endl;
    }
  }
  std::cout << "Running read..." << std::endl;
}

void Write::run() {
  for (char* addr : op_addresses_) {
    const char* access_end_addr = addr + access_size_;
    __m512i* data = (__m512i*)(internal::WRITE_DATA);
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += internal::CACHE_LINE_SIZE) {
      // Write 512 Bit (64 Byte) and persist it.
      _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
      pmem_persist(mem_addr, internal::CACHE_LINE_SIZE);
    }
  }
  std::cout << "Running write..." << std::endl;
}
}  // namespace perma
