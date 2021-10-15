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

#define READ_SIMD_512(mem_addr, offset) _mm512_load_si512((void*)((mem_addr) + ((offset)*CACHE_LINE_SIZE)))

#define WRITE_SIMD_NT_512(mem_addr, offset, data) \
  _mm512_stream_si512(reinterpret_cast<__m512i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

#define WRITE_SIMD_512(mem_addr, offset, data) \
  _mm512_store_si512(reinterpret_cast<__m512i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

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

inline void simd_write_data_range(char* from, const char* to) {
  __m512i* data = (__m512i*)(WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    WRITE_SIMD_512(mem_addr, 0, *data);
  }
}

/**
 * #####################################################
 * BASE STORE OPERATIONS
 * #####################################################
 */

inline void simd_write_64(char* addr, flush_fn flush, barrier_fn barrier) {
  // Write 512 Bit (64 Byte)
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_512(addr, 0, *data);
  flush(addr, 64);
  barrier();
}

inline void simd_write_128(char* addr, flush_fn flush, barrier_fn barrier) {
  // Write 128 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_512(addr, 0, *data);
  WRITE_SIMD_512(addr, 1, *data);
  flush(addr, 128);
  barrier();
}

inline void simd_write_256(char* addr, flush_fn flush, barrier_fn barrier) {
  // Write 256 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_512(addr, 0, *data);
  WRITE_SIMD_512(addr, 1, *data);
  WRITE_SIMD_512(addr, 2, *data);
  WRITE_SIMD_512(addr, 3, *data);
  flush(addr, 256);
  barrier();
}

inline void simd_write_512(char* addr, flush_fn flush, barrier_fn barrier) {
  // Write 512 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_512(addr, 0, *data);
  WRITE_SIMD_512(addr, 1, *data);
  WRITE_SIMD_512(addr, 2, *data);
  WRITE_SIMD_512(addr, 3, *data);
  WRITE_SIMD_512(addr, 4, *data);
  WRITE_SIMD_512(addr, 5, *data);
  WRITE_SIMD_512(addr, 6, *data);
  WRITE_SIMD_512(addr, 7, *data);
  flush(addr, 512);
  barrier();
}

inline void simd_write(char* addr, const size_t access_size, flush_fn flush, barrier_fn barrier) {
  // Write 512 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  const char* access_end_addr = addr + access_size;
  for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
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

inline void simd_write_64(const std::vector<char*>& addresses, flush_fn flush, barrier_fn barrier) {
  for (char* addr : addresses) {
    simd_write_64(addr, flush, barrier);
  }
}

inline void simd_write_128(const std::vector<char*>& addresses, flush_fn flush, barrier_fn barrier) {
  for (char* addr : addresses) {
    simd_write_128(addr, flush, barrier);
  }
}

inline void simd_write_256(const std::vector<char*>& addresses, flush_fn flush, barrier_fn barrier) {
  for (char* addr : addresses) {
    simd_write_256(addr, flush, barrier);
  }
}

inline void simd_write_512(const std::vector<char*>& addresses, flush_fn flush, barrier_fn barrier) {
  for (char* addr : addresses) {
    simd_write_512(addr, flush, barrier);
  }
}

inline void simd_write(const std::vector<char*>& addresses, const size_t access_size, flush_fn flush,
                       barrier_fn barrier) {
  for (char* addr : addresses) {
    simd_write(addr, access_size, flush, barrier);
  }
}

/**
 * #####################################################
 * STORE + CLWB OPERATIONS
 * #####################################################
 */

#ifdef HAS_CLWB
inline void simd_write_clwb_512(char* addr) { simd_write_512(addr, flush_clwb, sfence_barrier); }

inline void simd_write_clwb_256(char* addr) { simd_write_256(addr, flush_clwb, sfence_barrier); }

inline void simd_write_clwb_128(char* addr) { simd_write_128(addr, flush_clwb, sfence_barrier); }

inline void simd_write_clwb_64(char* addr) { simd_write_64(addr, flush_clwb, sfence_barrier); }

inline void simd_write_clwb(char* addr, const size_t access_size) {
  simd_write(addr, access_size, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_512(const std::vector<char*>& addresses) {
  simd_write_512(addresses, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_256(const std::vector<char*>& addresses) {
  simd_write_256(addresses, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_128(const std::vector<char*>& addresses) {
  simd_write_128(addresses, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb_64(const std::vector<char*>& addresses) {
  simd_write_64(addresses, flush_clwb, sfence_barrier);
}

inline void simd_write_clwb(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write(addresses, access_size, flush_clwb, sfence_barrier);
}
#endif

/**
 * #####################################################
 * STORE-ONLY OPERATIONS
 * #####################################################
 */

inline void simd_write_none_512(char* addr) { simd_write_512(addr, no_flush, no_barrier); }

inline void simd_write_none_256(char* addr) { simd_write_256(addr, no_flush, no_barrier); }

inline void simd_write_none_128(char* addr) { simd_write_128(addr, no_flush, no_barrier); }

inline void simd_write_none_64(char* addr) { simd_write_64(addr, no_flush, no_barrier); }

inline void simd_write_none(char* addr, const size_t access_size) {
  simd_write(addr, access_size, no_flush, no_barrier);
}

inline void simd_write_none_512(const std::vector<char*>& addresses) {
  simd_write_512(addresses, no_flush, no_barrier);
}

inline void simd_write_none_256(const std::vector<char*>& addresses) {
  simd_write_256(addresses, no_flush, no_barrier);
}

inline void simd_write_none_128(const std::vector<char*>& addresses) {
  simd_write_128(addresses, no_flush, no_barrier);
}

inline void simd_write_none_64(const std::vector<char*>& addresses) { simd_write_64(addresses, no_flush, no_barrier); }

inline void simd_write_none(const std::vector<char*>& addresses, const size_t access_size) {
  simd_write(addresses, access_size, no_flush, no_barrier);
}

/**
 * #####################################################
 * NON_TEMPORAL STORE OPERATIONS
 * #####################################################
 */

inline void simd_write_nt_64(char* addr) {
  // Write 512 Bit (64 Byte)
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_NT_512(addr, 0, *data);
  sfence_barrier();
}

inline void simd_write_nt_128(char* addr) {
  // Write 128 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_NT_512(addr, 0, *data);
  WRITE_SIMD_NT_512(addr, 1, *data);
  sfence_barrier();
}

inline void simd_write_nt_256(char* addr) {
  // Write 256 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_NT_512(addr, 0, *data);
  WRITE_SIMD_NT_512(addr, 1, *data);
  WRITE_SIMD_NT_512(addr, 2, *data);
  WRITE_SIMD_NT_512(addr, 3, *data);
  sfence_barrier();
}

inline void simd_write_nt_512(char* addr) {
  // Write 512 Byte
  __m512i* data = (__m512i*)(WRITE_DATA);
  WRITE_SIMD_NT_512(addr, 0, *data);
  WRITE_SIMD_NT_512(addr, 1, *data);
  WRITE_SIMD_NT_512(addr, 2, *data);
  WRITE_SIMD_NT_512(addr, 3, *data);
  WRITE_SIMD_NT_512(addr, 4, *data);
  WRITE_SIMD_NT_512(addr, 5, *data);
  WRITE_SIMD_NT_512(addr, 6, *data);
  WRITE_SIMD_NT_512(addr, 7, *data);
  sfence_barrier();
}

inline void simd_write_nt(char* addr, const size_t access_size) {
  __m512i* data = (__m512i*)(WRITE_DATA);
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

inline void simd_write_nt_64(const std::vector<char*>& addresses) {
  for (char* addr : addresses) {
    simd_write_nt_64(addr);
  }
}

inline void simd_write_nt_128(const std::vector<char*>& addresses) {
  for (char* addr : addresses) {
    simd_write_nt_128(addr);
  }
}

inline void simd_write_nt_256(const std::vector<char*>& addresses) {
  for (char* addr : addresses) {
    simd_write_nt_256(addr);
  }
}

inline void simd_write_nt_512(const std::vector<char*>& addresses) {
  for (char* addr : addresses) {
    simd_write_nt_512(addr);
  }
}

inline void simd_write_nt(const std::vector<char*>& addresses, const size_t access_size) {
  for (char* addr : addresses) {
    simd_write_nt(addr, access_size);
  }
}

/**
 * #####################################################
 * READ OPERATIONS
 * #####################################################
 */

inline __m512i simd_read_64(char* addr) { return READ_SIMD_512(addr, 0); }

inline __m512i simd_read_128(char* addr) {
  __m512i res0, res1;
  res0 = READ_SIMD_512(addr, 0);
  res1 = READ_SIMD_512(addr, 1);
  return res0 + res1;
}

inline __m512i simd_read_256(char* addr) {
  __m512i res0, res1, res2, res3;
  res0 = READ_SIMD_512(addr, 0);
  res1 = READ_SIMD_512(addr, 1);
  res2 = READ_SIMD_512(addr, 2);
  res3 = READ_SIMD_512(addr, 3);
  return res0 + res1 + res2 + res3;
}

inline __m512i simd_read_512(char* addr) {
  __m512i res0, res1, res2, res3, res4, res5, res6, res7;
  res0 = READ_SIMD_512(addr, 0);
  res1 = READ_SIMD_512(addr, 1);
  res2 = READ_SIMD_512(addr, 2);
  res3 = READ_SIMD_512(addr, 3);
  res4 = READ_SIMD_512(addr, 4);
  res5 = READ_SIMD_512(addr, 5);
  res6 = READ_SIMD_512(addr, 6);
  res7 = READ_SIMD_512(addr, 7);
  return res0 + res1 + res2 + res3 + res4 + res5 + res6 + res7;
}

inline __m512i simd_read(char* addr, const size_t access_size) {
  __m512i res0, res1, res2, res3, res4, res5, res6, res7;
  const char* access_end_addr = addr + access_size;
  for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
    res0 = READ_SIMD_512(addr, 0);
    res1 = READ_SIMD_512(addr, 1);
    res2 = READ_SIMD_512(addr, 2);
    res3 = READ_SIMD_512(addr, 3);
    res4 = READ_SIMD_512(addr, 4);
    res5 = READ_SIMD_512(addr, 5);
    res6 = READ_SIMD_512(addr, 6);
    res7 = READ_SIMD_512(addr, 7);
  }
  return res0 + res1 + res2 + res3 + res4 + res5 + res6 + res7;
}

inline void simd_read_64(const std::vector<char*>& addresses) {
  __m512i res;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      res = simd_read_64(addr);
    }
    return res;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read_128(const std::vector<char*>& addresses) {
  __m512i res0, res1;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      // Read 128 Byte
      res0 = READ_SIMD_512(addr, 0);
      res1 = READ_SIMD_512(addr, 1);
    }
    return res0 + res1;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read_256(const std::vector<char*>& addresses) {
  __m512i res0, res1, res2, res3;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      // Read 256 Byte
      res0 = READ_SIMD_512(addr, 0);
      res1 = READ_SIMD_512(addr, 1);
      res2 = READ_SIMD_512(addr, 2);
      res3 = READ_SIMD_512(addr, 3);
    }
    return res0 + res1 + res2 + res3;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read_512(const std::vector<char*>& addresses) {
  __m512i res0, res1, res2, res3, res4, res5, res6, res7;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      // Read 512 Byte
      res0 = READ_SIMD_512(addr, 0);
      res1 = READ_SIMD_512(addr, 1);
      res2 = READ_SIMD_512(addr, 2);
      res3 = READ_SIMD_512(addr, 3);
      res4 = READ_SIMD_512(addr, 4);
      res5 = READ_SIMD_512(addr, 5);
      res6 = READ_SIMD_512(addr, 6);
      res7 = READ_SIMD_512(addr, 7);
    }
    return res0 + res1 + res2 + res3 + res4 + res5 + res6 + res7;
  };
  // Do a single copy of the last read value to the stack from a zmm register. Otherwise, KEEP copies on each
  // invocation if we have KEEP in the loop because it cannot be sure how KEEP modifies the current zmm register.
  __m512i x = simd_fn();
  KEEP(&x);
}

inline void simd_read(const std::vector<char*>& addresses, const size_t access_size) {
  __m512i res0, res1, res2, res3, res4, res5, res6, res7;
  auto simd_fn = [&]() {
    for (char* addr : addresses) {
      const char* access_end_addr = addr + access_size;
      for (const char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += (8 * CACHE_LINE_SIZE)) {
        // Read in 512 Byte chunks
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

inline void write_data(char* from, const char* to) {
  return simd_write_data_range(from, to);
}

}  // namespace perma::rw_ops
