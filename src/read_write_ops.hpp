#pragma once

#include <immintrin.h>
#include <libpmem.h>

#include <vector>

namespace perma::rw_ops {

// This tells the compiler to keep whatever x is and not optimize it away.
// Inspired by Google Benchmark's DoNotOptimize and this talk
// https://youtu.be/nXaxk27zwlk?t=2441.
#define KEEP(x) asm volatile("" : : "g"(x) : "memory")

// Exactly 64 characters to write in one cache line.
static const char WRITE_DATA[] __attribute__((aligned(64))) =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

static constexpr size_t CACHE_LINE_SIZE = 64;

#ifdef HAS_AVX
inline void simd_write_data(char* from, const char* to) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
  }
}

inline void simd_write(char* addr, const size_t access_size) {
  const char* access_end_addr = addr + access_size;
  simd_write_data(addr, access_end_addr);
  pmem_persist(addr, access_size);
}

inline void simd_read(const char* addr, const size_t access_size) {
  auto simd_fn = [&]() {
    //    __m512i res;
    const char* access_end_addr = addr + access_size;
    for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte)
      //      res = _mm512_stream_load_si512((void*) mem_addr);
      asm volatile(
          "mov       %[m_addr],     %%rsi  \n"
          "vmovntdqa 0*64(%%rsi), %%zmm0 \n"
          :
          : [m_addr] "r"(mem_addr));
    }
    asm volatile("mfence \n");
    //    return res;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  //  __m512i x = simd_fn();
  //  KEEP(&x);
  simd_fn();
}
#endif

inline void mov_write_data(char* from, const char* to) {
  asm volatile(
      "movq 0*8(%[write_data]), %%r8  \n\t"
      "movq 1*8(%[write_data]), %%r9  \n\t"
      "movq 2*8(%[write_data]), %%r10 \n\t"
      "movq 3*8(%[write_data]), %%r11 \n\t"
      "movq 4*8(%[write_data]), %%r12 \n\t"
      "movq 5*8(%[write_data]), %%r13 \n\t"
      "movq 6*8(%[write_data]), %%r14 \n\t"
      "movq 7*8(%[write_data]), %%r15 \n\t"
      :
      : [write_data] "r"(WRITE_DATA)
      : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Read 512 Bit (64 Byte)
    asm volatile(
        "movq  %%r8, 0*8(%[addr]) \n\t"
        "movq  %%r9, 1*8(%[addr]) \n\t"
        "movq %%r10, 2*8(%[addr]) \n\t"
        "movq %%r11, 3*8(%[addr]) \n\t"
        "movq %%r12, 4*8(%[addr]) \n\t"
        "movq %%r13, 5*8(%[addr]) \n\t"
        "movq %%r14, 6*8(%[addr]) \n\t"
        "movq %%r15, 7*8(%[addr]) \n\t"
        :
        : [addr] "r"(mem_addr), [write_data] "r"(WRITE_DATA));
  }
}

inline void mov_write(char* addr, const size_t access_size) {
  const char* access_end_addr = addr + access_size;
  mov_write_data(addr, access_end_addr);
  pmem_persist(addr, access_size);
}

inline void mov_read(const char* addr, const size_t access_size) {
  const char* access_end_addr = addr + access_size;
  for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
    // Read 512 Bit (64 Byte)
    asm volatile(
        "movq 0*8(%[addr]), %%r8  \n\t"
        "movq 1*8(%[addr]), %%r9  \n\t"
        "movq 2*8(%[addr]), %%r10 \n\t"
        "movq 3*8(%[addr]), %%r11 \n\t"
        "movq 4*8(%[addr]), %%r12 \n\t"
        "movq 5*8(%[addr]), %%r13 \n\t"
        "movq 6*8(%[addr]), %%r14 \n\t"
        "movq 7*8(%[addr]), %%r15 \n\t"
        :
        : [addr] "r"(mem_addr));
  }
}

inline void write_data(char* from, const char* to) {
#ifdef HAS_AVX
  return simd_write_data(from, to);
#else
  return mov_write_data(from, to);
#endif
}

}  // namespace perma::rw_ops
