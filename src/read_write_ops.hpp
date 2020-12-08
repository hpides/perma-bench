#pragma once

#include <immintrin.h>
#include <libpmem.h>
#include <xmmintrin.h>

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

// FLush and barrier operations are from https://github.com/pmem/pmdk/tree/master/src/libpmem2

typedef void flush_fn(const void*, const size_t);

/*
 * flush the CPU cache using clflushopt.
 */
inline void flush_clflushopt(const void* addr, const size_t len) {
  uintptr_t uptr;

  for (uptr = (uintptr_t)addr; uptr < (uintptr_t)addr + len; uptr += CACHE_LINE_SIZE) {
    asm volatile(".byte 0x66; clflush %0" : "+m"(*(volatile char*)(uptr)));
  }
}

/*
 * flush the CPU cache using clwb.
 */
inline void flush_clwb(const void* addr, const size_t len) {
  uintptr_t uptr;

  for (uptr = (uintptr_t)addr; uptr < (uintptr_t)addr + len; uptr += CACHE_LINE_SIZE) {
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char*)(uptr)));
  }
}

/*
 * non-temporal hints used by avx-512f instructions.
 */
inline void no_flush(const void* addr, const size_t len) {}

typedef void barrier_fn();

/*
 * use sfence to guarantee strong memory order on x86. Earlier store operations cannot be reordered beyond this point.
 */
inline void sfence_barrier() { _mm_sfence(); }

/*
 * no memory order is guaranteed.
 */
inline void no_barrier() {}

#ifdef HAS_AVX
inline void simd_write_data(char* from, const char* to) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    _mm512_store_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
  }
}

inline void simd_write(const std::vector<char*>& op_addresses, const size_t access_size, flush_fn flush,
                       barrier_fn barrier) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    simd_write_data(addr, access_end_addr);
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write_data_nt(char* from, const char* to) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
  }
}

inline void simd_write_nt(const std::vector<char*>& op_addresses, const size_t access_size, flush_fn flush,
                          barrier_fn barrier) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    simd_write_data_nt(addr, access_end_addr);
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write_clwb(const std::vector<char*>& op_addresses, const size_t access_size) {
  simd_write(op_addresses, access_size, flush_clwb, sfence_barrier);
}

inline void simd_write_clflush(const std::vector<char*>& op_addresses, const size_t access_size) {
  simd_write(op_addresses, access_size, flush_clflushopt, sfence_barrier);
}

inline void simd_write_nt(const std::vector<char*>& op_addresses, const size_t access_size) {
  simd_write_nt(op_addresses, access_size, no_flush, sfence_barrier);
}

inline void simd_write_none(const std::vector<char*>& op_addresses, const size_t access_size) {
  simd_write(op_addresses, access_size, no_flush, no_barrier);
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
#endif

inline void mov_write_data_nt(char* from, const char* to) {
  __m64* data_0 = (__m64*)&WRITE_DATA[0 * 8];
  __m64* data_1 = (__m64*)&WRITE_DATA[1 * 8];
  __m64* data_2 = (__m64*)&WRITE_DATA[2 * 8];
  __m64* data_3 = (__m64*)&WRITE_DATA[3 * 8];
  __m64* data_4 = (__m64*)&WRITE_DATA[4 * 8];
  __m64* data_5 = (__m64*)&WRITE_DATA[5 * 8];
  __m64* data_6 = (__m64*)&WRITE_DATA[6 * 8];
  __m64* data_7 = (__m64*)&WRITE_DATA[7 * 8];
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte)
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 0 * 8), *data_0);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 1 * 8), *data_1);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 2 * 8), *data_2);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 3 * 8), *data_3);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 4 * 8), *data_4);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 5 * 8), *data_5);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 6 * 8), *data_6);
    _mm_stream_pi(reinterpret_cast<__m64*>(mem_addr + 7 * 8), *data_7);
  }
}

inline void mov_write_nt(const std::vector<char*>& op_addresses, const size_t access_size, flush_fn flush,
                         barrier_fn barrier) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    mov_write_data_nt(addr, access_end_addr);
    flush(addr, access_size);
    barrier();
  }
}

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
      : [ write_data ] "r"(WRITE_DATA)
      : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte)
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
        : [ addr ] "r"(mem_addr), [ write_data ] "r"(WRITE_DATA));
  }
}

inline void mov_write(const std::vector<char*>& op_addresses, const size_t access_size, flush_fn flush,
                      barrier_fn barrier) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    mov_write_data(addr, access_end_addr);
    flush(addr, access_size);
    barrier();
  }
}

inline void mov_write_clwb(const std::vector<char*>& op_addresses, const size_t access_size) {
  mov_write(op_addresses, access_size, flush_clwb, sfence_barrier);
}

inline void mov_write_clflush(const std::vector<char*>& op_addresses, const size_t access_size) {
  mov_write(op_addresses, access_size, flush_clflushopt, sfence_barrier);
}

inline void mov_write_nt(const std::vector<char*>& op_addresses, const size_t access_size) {
  mov_write_nt(op_addresses, access_size, no_flush, sfence_barrier);
}

inline void mov_write_none(const std::vector<char*>& op_addresses, const size_t access_size) {
  mov_write(op_addresses, access_size, no_flush, no_barrier);
}

inline void mov_read(const std::vector<char*>& op_addresses, const size_t access_size) {
  for (char* addr : op_addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte)
      asm volatile(
          "movq 0*8(%[addr]), %%r8  \n\t"
          "movq 1*8(%[addr]), %%r8  \n\t"
          "movq 2*8(%[addr]), %%r8  \n\t"
          "movq 3*8(%[addr]), %%r8  \n\t"
          "movq 4*8(%[addr]), %%r8  \n\t"
          "movq 5*8(%[addr]), %%r8  \n\t"
          "movq 6*8(%[addr]), %%r8  \n\t"
          "movq 7*8(%[addr]), %%r8  \n\t"
          :
          : [ addr ] "r"(mem_addr));
    }
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
