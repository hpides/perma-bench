#include "io_operation.hpp"

#include <immintrin.h>
#include <libpmem.h>

namespace perma {

// This tells the compiler to keep whatever x is and not optimize it away.
// Inspired by Google Benchmark's DoNotOptimize and this talk
// https://youtu.be/nXaxk27zwlk?t=2441.
#define KEEP(x) asm volatile("" : : "g"(x) : "memory")

uint32_t Pause::get_length() const { return length_; }

void Pause::run() { std::this_thread::sleep_for(std::chrono::microseconds(length_)); }

void Read::run() {
  for (char* addr : op_addresses_) {
    const char* access_end_addr = addr + access_size_;
    for (char* mem_addr = addr; mem_addr < access_end_addr; mem_addr += internal::CACHE_LINE_SIZE) {
      // Read 512 Bit (64 Byte) and do not optimize it out.
      KEEP(_mm512_stream_load_si512(mem_addr));
      // __m512i x = _mm512_stream_load_si512(mem_addr);
    }
  }
}

void Write::run() {
  for (char* addr : op_addresses_) {
    const char* access_end_addr = addr + access_size_;
    write_data(addr, access_end_addr);
  }
}

void write_data(char* from, const char* to) {
  __m512i* data = (__m512i*)(internal::WRITE_DATA);
  for (char* mem_addr = from; mem_addr < to; mem_addr += internal::CACHE_LINE_SIZE) {
    // Write 512 Bit (64 Byte) and persist it.
    _mm512_stream_si512(reinterpret_cast<__m512i*>(mem_addr), *data);
    pmem_persist(mem_addr, internal::CACHE_LINE_SIZE);
  }
}
}  // namespace perma
