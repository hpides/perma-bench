#pragma once

#include <immintrin.h>
#include <vector>

#include <libpmem.h>

namespace perma::rw_ops {

// This tells the compiler to keep whatever x is and not optimize it away.
// Inspired by Google Benchmark's DoNotOptimize and this talk
// https://youtu.be/nXaxk27zwlk?t=2441.
#define KEEP(x) asm volatile("" : : "g"(x) : "memory")

// Exactly 64 characters to write in one cache line.
static const char WRITE_DATA[] __attribute__((aligned(64))) =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

static constexpr size_t CACHE_LINE_SIZE = 64;

inline void simd_write_data(char* from, const char* to) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
    pmem_persist(mem_addr, CACHE_LINE_SIZE);
  }
}

inline void simd_write(const std::vector<char*>& op_addresses, const size_t access_size) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    simd_write_data(addr, access_end_addr);
  }
}

inline void mov_write(const std::vector<char*>& op_addresses, const size_t access_size) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte)
      asm volatile(
        "mov    0*8(%[write_data]), 0*8(%[addr]) \n\t"
        "mov    1*8(%[write_data]), 1*8(%[addr]) \n\t"
        "mov    2*8(%[write_data]), 2*8(%[addr]) \n\t"
        "mov    3*8(%[write_data]), 3*8(%[addr]) \n\t"
        "mov    4*8(%[write_data]), 4*8(%[addr]) \n\t"
        "mov    5*8(%[write_data]), 5*8(%[addr]) \n\t"
        "mov    6*8(%[write_data]), 6*8(%[addr]) \n\t"
        "mov    7*8(%[write_data]), 7*8(%[addr]) \n\t"
        :
        : [addr] "r" (mem_addr), [write_data] "r" (WRITE_DATA)
      );
    }
  }
}

inline void simd_read(const std::vector<char*>& op_addresses, const size_t access_size) {
  auto simd_fn = [&]() {
    __m512i res;
    for (char* addr : op_addresses) {
      const char* access_end_addr = addr + access_size;
      for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
        // Read 512 Bit (64 Byte)
        res = _mm512_stream_load_si512(mem_addr);
      }
    }
    return res;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void mov_read(const std::vector<char*>& op_addresses, const size_t access_size) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte)
      asm volatile(
        "mov    0*8(%[addr]), %%r8  \n\t"
        "mov    1*8(%[addr]), %%r8  \n\t"
        "mov    2*8(%[addr]), %%r8  \n\t"
        "mov    3*8(%[addr]), %%r8  \n\t"
        "mov    4*8(%[addr]), %%r8  \n\t"
        "mov    5*8(%[addr]), %%r8  \n\t"
        "mov    6*8(%[addr]), %%r8  \n\t"
        "mov    7*8(%[addr]), %%r8  \n\t"
        :
        : [addr] "r" (mem_addr)
      );
    }
  }
}

}  // namespace perma::rw_ops
