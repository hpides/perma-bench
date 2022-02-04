#pragma once

#include <thread>
#include <vector>

#include "benchmark_config.hpp"
#include "fast_random.hpp"
#include "read_write_ops.hpp"
#include "spdlog/spdlog.h"
#include "utils.hpp"

namespace perma {

class IoOperation {
  friend class Benchmark;

 public:
  IoOperation(std::vector<char*>&& op_addresses, uint32_t access_size, Operation op_type,
              PersistInstruction persist_instruction)
      : op_addresses_{std::move(op_addresses)},
        access_size_{access_size},
        op_type_{op_type},
        persist_instruction_{persist_instruction} {}

  IoOperation() : IoOperation{{}, 0, Operation::Read, PersistInstruction::None} {};

  IoOperation(const IoOperation&) = delete;
  IoOperation& operator=(const IoOperation&) = delete;
  IoOperation(IoOperation&&) = default;
  IoOperation& operator=(IoOperation&&) = default;
  ~IoOperation() = default;

  inline void run() {
    switch (op_type_) {
      case Operation::Read: {
        return run_read();
      }
      case Operation::Write: {
        return run_write();
      }
      default: {
        spdlog::critical("Invalid operation: {}", op_type_);
        utils::crash_exit();
      }
    }
  }

  inline bool is_read() const { return op_type_ == Operation::Read; }
  inline bool is_write() const { return op_type_ == Operation::Write; }

 private:
  void run_read() {
#ifdef HAS_AVX
    switch (access_size_) {
      case 64:
        return rw_ops::simd_read_64(op_addresses_);
      case 128:
        return rw_ops::simd_read_128(op_addresses_);
      case 256:
        return rw_ops::simd_read_256(op_addresses_);
      case 512:
        return rw_ops::simd_read_512(op_addresses_);
      default:
        return rw_ops::simd_read(op_addresses_, access_size_);
    }
#endif
  }

  void run_write() {
#ifdef HAS_AVX
    switch (persist_instruction_) {
#ifdef HAS_CLWB
      case PersistInstruction::Cache: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_clwb_64(op_addresses_);
          case 128:
            return rw_ops::simd_write_clwb_128(op_addresses_);
          case 256:
            return rw_ops::simd_write_clwb_256(op_addresses_);
          case 512:
            return rw_ops::simd_write_clwb_512(op_addresses_);
          default:
            return rw_ops::simd_write_clwb(op_addresses_, access_size_);
        }
      }
#endif
#ifdef HAS_CLFLUSHOPT
      case PersistInstruction::CacheInvalidate: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_clflushopt_64(op_addresses_);
          case 128:
            return rw_ops::simd_write_clflushopt_128(op_addresses_);
          case 256:
            return rw_ops::simd_write_clflushopt_256(op_addresses_);
          case 512:
            return rw_ops::simd_write_clflushopt_512(op_addresses_);
          default:
            return rw_ops::simd_write_clflushopt(op_addresses_, access_size_);
        }
      }
#endif
      case PersistInstruction::NoCache: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_nt_64(op_addresses_);
          case 128:
            return rw_ops::simd_write_nt_128(op_addresses_);
          case 256:
            return rw_ops::simd_write_nt_256(op_addresses_);
          case 512:
            return rw_ops::simd_write_nt_512(op_addresses_);
          default:
            return rw_ops::simd_write_nt(op_addresses_, access_size_);
        }
      }
      case PersistInstruction::None: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_none_64(op_addresses_);
          case 128:
            return rw_ops::simd_write_none_128(op_addresses_);
          case 256:
            return rw_ops::simd_write_none_256(op_addresses_);
          case 512:
            return rw_ops::simd_write_none_512(op_addresses_);
          default:
            return rw_ops::simd_write_none(op_addresses_, access_size_);
        }
      }
    }
#endif
  }

  std::vector<char*> op_addresses_;
  uint32_t access_size_;
  Operation op_type_;
  PersistInstruction persist_instruction_;
};

class ChainedOperation {
 public:
  ChainedOperation(const CustomOp& op, char* range_start, const size_t range_size)
      : range_start_(range_start),
        access_size_(op.size),
        range_size_(range_size),
        align_(-access_size_),
        type_(op.type),
        persist_instruction_(op.persist),
        offset_(op.offset) {}

  inline void run(char* current_addr, char* dependent_addr) {
    if (type_ == Operation::Read) {
      current_addr = get_random_address(dependent_addr);
      dependent_addr = run_read(current_addr);
    } else {
      current_addr += offset_;
      run_write(current_addr);
    }

    if (next_) {
      return next_->run(current_addr, dependent_addr);
    }
  }

  inline char* get_random_address(char* addr) {
    const uint64_t base = (uint64_t)addr;
    const uint64_t random_offset = base + lehmer64();
    const uint64_t offset_in_range = random_offset % range_size_;
    const uint64_t aligned_offset = offset_in_range & align_;
    return range_start_ + aligned_offset;
  }

  void set_next(ChainedOperation* next) { next_ = next; }

 private:
  inline char* run_read(char* addr) {
#ifdef HAS_AVX
    __m512i read_value;
    switch (access_size_) {
      case 64:
        read_value = rw_ops::simd_read_64(addr);
        break;
      case 128:
        read_value = rw_ops::simd_read_128(addr);
        break;
      case 256:
        read_value = rw_ops::simd_read_256(addr);
        break;
      case 512:
        read_value = rw_ops::simd_read_512(addr);
        break;
      default:
        read_value = rw_ops::simd_read(addr, access_size_);
    }

    // Make sure the compiler does not optimize the load away.
    KEEP(&read_value);
    return (char*)read_value[0];
#else
    // This is only to stop the compiler complaining about a missing return
    return addr;
#endif
  }

  inline void run_write(char* addr) {
#ifdef HAS_AVX
    switch (persist_instruction_) {
#ifdef HAS_CLWB
      case PersistInstruction::Cache: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_clwb_64(addr);
          case 128:
            return rw_ops::simd_write_clwb_128(addr);
          case 256:
            return rw_ops::simd_write_clwb_256(addr);
          case 512:
            return rw_ops::simd_write_clwb_512(addr);
          default:
            return rw_ops::simd_write_clwb(addr, access_size_);
        }
      }
#endif
#ifdef HAS_CLFLUSHOPT
      case PersistInstruction::CacheInvalidate: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_clflushopt_64(addr);
          case 128:
            return rw_ops::simd_write_clflushopt_128(addr);
          case 256:
            return rw_ops::simd_write_clflushopt_256(addr);
          case 512:
            return rw_ops::simd_write_clflushopt_512(addr);
          default:
            return rw_ops::simd_write_clflushopt(addr, access_size_);
        }
      }
#endif
      case PersistInstruction::NoCache: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_nt_64(addr);
          case 128:
            return rw_ops::simd_write_nt_128(addr);
          case 256:
            return rw_ops::simd_write_nt_256(addr);
          case 512:
            return rw_ops::simd_write_nt_512(addr);
          default:
            return rw_ops::simd_write_nt(addr, access_size_);
        }
      }
      case PersistInstruction::None: {
        switch (access_size_) {
          case 64:
            return rw_ops::simd_write_none_64(addr);
          case 128:
            return rw_ops::simd_write_none_128(addr);
          case 256:
            return rw_ops::simd_write_none_256(addr);
          case 512:
            return rw_ops::simd_write_none_512(addr);
          default:
            return rw_ops::simd_write_none(addr, access_size_);
        }
      }
    }
#endif
  }

 private:
  char* const range_start_;
  const size_t access_size_;
  const size_t range_size_;
  const size_t align_;
  ChainedOperation* next_ = nullptr;
  const Operation type_;
  const PersistInstruction persist_instruction_;
  const int64_t offset_;
};

}  // namespace perma
