#pragma once

#include <immintrin.h>
#include <xmmintrin.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace perma::rw_ops {

// This tells the compiler to keep whatever x is and not optimize it away.
// Inspired by Google Benchmark's DoNotOptimize and this talk
// https://youtu.be/nXaxk27zwlk?t=2441.
#define KEEP(x) asm volatile("" : : "g"(x) : "memory")

#define READ_SIMD_512(mem_addr, offset) _mm512_stream_load_si512((void*)(mem_addr + (offset * CACHE_LINE_SIZE)))

#define READ_MOV_512(mem_addr, offset) \
  asm volatile(                        \
      "movq 0*8(%[addr]), %%r8  \n\t"  \
      "movq 1*8(%[addr]), %%r8  \n\t"  \
      "movq 2*8(%[addr]), %%r8  \n\t"  \
      "movq 3*8(%[addr]), %%r8  \n\t"  \
      "movq 4*8(%[addr]), %%r8  \n\t"  \
      "movq 5*8(%[addr]), %%r8  \n\t"  \
      "movq 6*8(%[addr]), %%r8  \n\t"  \
      "movq 7*8(%[addr]), %%r8  \n\t"  \
      :                                \
      : [ addr ] "r"(mem_addr + (offset * CACHE_LINE_SIZE)))

#define WRITE_SIMD_NT_512(mem_addr, offset, data) \
  _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr + (offset * CACHE_LINE_SIZE)), data)

#define WRITE_SIMD_512(mem_addr, offset, data) \
  _mm512_store_si512(reinterpret_cast<__m512i*>(mem_addr + (offset * CACHE_LINE_SIZE)), data)

#define WRITE_MOV_NT_512(mem_addr, offset)                                                             \
  _mm_stream_pi((__m64*)(mem_addr + 0 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[0 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 1 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[1 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 2 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[2 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 3 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[3 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 4 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[4 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 5 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[5 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 6 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[6 * 8]); \
  _mm_stream_pi((__m64*)(mem_addr + 7 * 8 + (offset * CACHE_LINE_SIZE)), *(__m64*)&WRITE_DATA[7 * 8]);

#define WRITE_MOV_512(mem_addr, offset)                                                  \
  std::memcpy(mem_addr + (0 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (0 * 8), 8); \
  std::memcpy(mem_addr + (1 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (1 * 8), 8); \
  std::memcpy(mem_addr + (2 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (2 * 8), 8); \
  std::memcpy(mem_addr + (3 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (3 * 8), 8); \
  std::memcpy(mem_addr + (4 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (4 * 8), 8); \
  std::memcpy(mem_addr + (5 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (5 * 8), 8); \
  std::memcpy(mem_addr + (6 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (6 * 8), 8); \
  std::memcpy(mem_addr + (7 * 8) + (offset * CACHE_LINE_SIZE), WRITE_DATA + (7 * 8), 8);

// Exactly 64 characters to write in one cache line.
static const char WRITE_DATA[] __attribute__((aligned(64))) =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

static constexpr size_t CACHE_LINE_SIZE = 64;

// FLush and barrier operations are from https://github.com/pmem/pmdk/tree/master/src/libpmem2

typedef void flush_fn(const void*, const size_t);

/*
 * flush the CPU cache using clwb.
 */
#ifdef HAS_CLWB
inline void flush_clwb(const void* addr, const size_t len) {
  uintptr_t uptr;

  for (uptr = (uintptr_t)addr; uptr < (uintptr_t)addr + len; uptr += CACHE_LINE_SIZE) {
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char*)(uptr)));
  }
}
#endif

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
    WRITE_SIMD_512(mem_addr, 0, *data);
  }
}

inline void simd_write_512(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                           barrier_fn barrier) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
      // Write 512 Bit (64 Byte) and persist it.
      WRITE_SIMD_512(mem_addr, 0, *data);
      WRITE_SIMD_512(mem_addr, 1, *data);
      WRITE_SIMD_512(mem_addr, 2, *data);
      WRITE_SIMD_512(mem_addr, 3, *data);
      WRITE_SIMD_512(mem_addr, 4, *data);
      WRITE_SIMD_512(mem_addr, 5, *data);
      WRITE_SIMD_512(mem_addr, 6, *data);
      WRITE_SIMD_512(mem_addr, 7, *data);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write_256(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                           barrier_fn barrier) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
      // Write 512 Bit (64 Byte) and persist it.
      WRITE_SIMD_512(mem_addr, 0, *data);
      WRITE_SIMD_512(mem_addr, 1, *data);
      WRITE_SIMD_512(mem_addr, 2, *data);
      WRITE_SIMD_512(mem_addr, 3, *data);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write_128(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                           barrier_fn barrier) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
      // Write 512 Bit (64 Byte) and persist it.
      WRITE_SIMD_512(mem_addr, 0, *data);
      WRITE_SIMD_512(mem_addr, 1, *data);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                       barrier_fn barrier) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    simd_write_data(addr, access_end_addr);
    flush(addr, access_size);
    barrier();
  }
}

inline void simd_write_nt_512(const std::vector<char*>& addresses, const size_t access_size) {
  auto data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
      // Write 512 byte.
      WRITE_SIMD_NT_512(mem_addr, 0, *data);
      WRITE_SIMD_NT_512(mem_addr, 1, *data);
      WRITE_SIMD_NT_512(mem_addr, 2, *data);
      WRITE_SIMD_NT_512(mem_addr, 3, *data);
      WRITE_SIMD_NT_512(mem_addr, 4, *data);
      WRITE_SIMD_NT_512(mem_addr, 5, *data);
      WRITE_SIMD_NT_512(mem_addr, 6, *data);
      WRITE_SIMD_NT_512(mem_addr, 7, *data);
    }
    sfence_barrier();
  }
}

inline void simd_write_nt_256(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
      // Write 256 Byte.
      WRITE_SIMD_NT_512(mem_addr, 0, *data);
      WRITE_SIMD_NT_512(mem_addr, 1, *data);
      WRITE_SIMD_NT_512(mem_addr, 2, *data);
      WRITE_SIMD_NT_512(mem_addr, 3, *data);
    }
    sfence_barrier();
  }
}

inline void simd_write_nt_128(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
      // Write 128 Byte.
      WRITE_SIMD_NT_512(mem_addr, 0, *data);
      WRITE_SIMD_NT_512(mem_addr, 1, *data);
    }
    sfence_barrier();
  }
}

inline void simd_write_nt(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Write 512 Bit (64 Byte) and persist it.
      WRITE_SIMD_NT_512(mem_addr, 0, *data);
    }
    sfence_barrier();
  }
}

#ifdef HAS_CLWB
inline void simd_write_clwb_512(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_512(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_256(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_256(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_128(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_128(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write(addresses, access_size, flush_clwb, sfence_barrier);
}
#endif

inline void simd_write_none_512(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_512(addresses, access_size, no_flush, no_barrier);
}

inline void simd_write_none_256(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_256(addresses, access_size, no_flush, no_barrier);
}

inline void simd_write_none_128(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write_128(addresses, access_size, no_flush, no_barrier);
}

inline void simd_write_none(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write(addresses, access_size, no_flush, no_barrier);
}

inline void simd_read_512(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i res0, res1, res2, res3, res4, res5, res6, res7;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      const char* access_end_addr = addr + access_size;
      for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
        // Read 512 Byte
        res0 = READ_SIMD_512(mem_addr, 0);
        res1 = READ_SIMD_512(mem_addr, 1);
        res2 = READ_SIMD_512(mem_addr, 2);
        res3 = READ_SIMD_512(mem_addr, 3);
        res4 = READ_SIMD_512(mem_addr, 4);
        res5 = READ_SIMD_512(mem_addr, 5);
        res6 = READ_SIMD_512(mem_addr, 6);
        res7 = READ_SIMD_512(mem_addr, 7);
      }
    }
    return res0 + res1 + res2 + res3 + res4 + res5 + res6 + res7;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read_256(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i res0, res1, res2, res3;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      const char* access_end_addr = addr + access_size;
      for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
        // Read 256 Byte
        res0 = READ_SIMD_512(mem_addr, 0);
        res1 = READ_SIMD_512(mem_addr, 1);
        res2 = READ_SIMD_512(mem_addr, 2);
        res3 = READ_SIMD_512(mem_addr, 3);
      }
    }
    return res0 + res1 + res2 + res3;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read_128(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i res0, res1;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      const char* access_end_addr = addr + access_size;
      for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
        // Read 128 Byte
        res0 = READ_SIMD_512(mem_addr, 0);
        res1 = READ_SIMD_512(mem_addr, 1);
      }
    }
    return res0 + res1;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i res;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      const char* access_end_addr = addr + access_size;
      for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
        // Read 512 Bit (64 Byte)
        res = READ_SIMD_512(mem_addr, 0);
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

inline void mov_write_nt_512(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
      // Write 512 Byte
      WRITE_MOV_NT_512(mem_addr, 0);
      WRITE_MOV_NT_512(mem_addr, 1);
      WRITE_MOV_NT_512(mem_addr, 2);
      WRITE_MOV_NT_512(mem_addr, 3);
      WRITE_MOV_NT_512(mem_addr, 4);
      WRITE_MOV_NT_512(mem_addr, 5);
      WRITE_MOV_NT_512(mem_addr, 6);
      WRITE_MOV_NT_512(mem_addr, 7);
    }
    sfence_barrier();
  }
}

inline void mov_write_nt_256(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
      // Write 256 Byte
      WRITE_MOV_NT_512(mem_addr, 0);
      WRITE_MOV_NT_512(mem_addr, 1);
      WRITE_MOV_NT_512(mem_addr, 2);
      WRITE_MOV_NT_512(mem_addr, 3);
    }
    sfence_barrier();
  }
}

inline void mov_write_nt_128(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
      // Write 128 Byte
      WRITE_MOV_NT_512(mem_addr, 0);
      WRITE_MOV_NT_512(mem_addr, 1);
    }
    sfence_barrier();
  }
}

inline void mov_write_nt(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Write 512 Bit (64 Byte)
      WRITE_MOV_NT_512(mem_addr, 0);
    }
    sfence_barrier();
  }
}

inline void mov_write_data(char* from, const char* to) {
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte)
    WRITE_MOV_512(mem_addr, 0);
  }
}

inline void mov_write_512(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                          barrier_fn barrier) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
      // Write 512 Byte
      WRITE_MOV_512(mem_addr, 0);
      WRITE_MOV_512(mem_addr, 1);
      WRITE_MOV_512(mem_addr, 2);
      WRITE_MOV_512(mem_addr, 3);
      WRITE_MOV_512(mem_addr, 4);
      WRITE_MOV_512(mem_addr, 5);
      WRITE_MOV_512(mem_addr, 6);
      WRITE_MOV_512(mem_addr, 7);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void mov_write_256(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                          barrier_fn barrier) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
      // Write 256 Byte
      WRITE_MOV_512(mem_addr, 0);
      WRITE_MOV_512(mem_addr, 1);
      WRITE_MOV_512(mem_addr, 2);
      WRITE_MOV_512(mem_addr, 3);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void mov_write_128(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                          barrier_fn barrier) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
      // Write 128 Byte
      WRITE_MOV_512(mem_addr, 0);
      WRITE_MOV_512(mem_addr, 1);
    }
    flush(addr, access_size);
    barrier();
  }
}

inline void mov_write(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                      barrier_fn barrier) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Write 512 Bit (64 Byte)
      WRITE_MOV_512(mem_addr, 0);
    }
    flush(addr, access_size);
    barrier();
  }
}

#ifdef HAS_CLWB
inline void mov_write_clwb_512(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_512(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void mov_write_clwb_256(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_256(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void mov_write_clwb_128(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_128(addresses, access_size, flush_clwb, sfence_barrier);
}

inline void mov_write_clwb(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write(addresses, access_size, flush_clwb, sfence_barrier);
}
#endif

inline void mov_write_none_512(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_512(addresses, access_size, no_flush, no_barrier);
}

inline void mov_write_none_256(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_256(addresses, access_size, no_flush, no_barrier);
}

inline void mov_write_none_128(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write_128(addresses, access_size, no_flush, no_barrier);
}

inline void mov_write_none(const std::vector<char*>& addresses, const size_t access_size) {
  mov_write(addresses, access_size, no_flush, no_barrier);
}

inline void mov_read_512(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
      // Read 512 Byte
      READ_MOV_512(mem_addr, 0);
      READ_MOV_512(mem_addr, 1);
      READ_MOV_512(mem_addr, 2);
      READ_MOV_512(mem_addr, 3);
      READ_MOV_512(mem_addr, 4);
      READ_MOV_512(mem_addr, 5);
      READ_MOV_512(mem_addr, 6);
      READ_MOV_512(mem_addr, 7);
    }
  }
}

inline void mov_read_256(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (4 * CACHE_LINE_SIZE)) {
      // Read 256 Byte
      READ_MOV_512(mem_addr, 0);
      READ_MOV_512(mem_addr, 1);
      READ_MOV_512(mem_addr, 2);
      READ_MOV_512(mem_addr, 3);
    }
  }
}

inline void mov_read_128(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (2 * CACHE_LINE_SIZE)) {
      // Read 128 Byte
      READ_MOV_512(mem_addr, 0);
      READ_MOV_512(mem_addr, 1);
    }
  }
}

inline void mov_read(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    const char* access_end_addr = addr + access_size;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte)
      READ_MOV_512(mem_addr, 0);
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
