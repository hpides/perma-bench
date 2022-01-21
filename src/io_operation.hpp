#pragma once

#include <thread>
#include <vector>

#include "fast_random.hpp"
#include "read_write_ops.hpp"
#include "spdlog/spdlog.h"
#include "utils.hpp"

namespace perma {

namespace internal {

enum class Mode : uint8_t { Sequential, Sequential_Desc, Random, Custom };

enum class RandomDistribution : uint8_t { Uniform, Zipf };

enum class PersistInstruction : uint8_t { Cache, CacheInvalidate, NoCache, None };

enum class OpType : uint8_t { Read, Write, Pause, Custom };

enum class NumaPattern : uint8_t { Near, Far };

}  // namespace internal

struct CustomOp {
  internal::OpType type;
  size_t size;
  internal::PersistInstruction persist = internal::PersistInstruction::None;

  static CustomOp from_string(const std::string& str);
  static std::vector<CustomOp> all_from_string(const std::string& str);
  static std::string all_to_string(const std::vector<CustomOp>& ops);
  static std::string to_string(const CustomOp& op);
};

class IoOperation {
  friend class Benchmark;

 public:
  inline void run() {
    switch (op_type_) {
      case (internal::OpType::Read): {
        return run_read();
      }
      case (internal::OpType::Write): {
        return run_write();
      }
      case (internal::OpType::Pause): {
        return std::this_thread::sleep_for(std::chrono::microseconds(duration_));
      }
      default: {
        throw std::runtime_error("Unknown OpType!");
      }
    }
  }

  inline bool is_active() const { return op_type_ != internal::OpType::Pause; }
  inline bool is_read() const { return op_type_ == internal::OpType::Read; }
  inline bool is_write() const { return op_type_ == internal::OpType::Write; }
  inline bool is_pause() const { return op_type_ == internal::OpType::Pause; }

  static IoOperation ReadOp(const std::vector<char*>& op_addresses, uint32_t access_size) {
    return IoOperation{op_addresses, access_size, internal::OpType::Read, internal::PersistInstruction::None};
  }

  static IoOperation WriteOp(const std::vector<char*>& op_addresses, uint32_t access_size,
                             internal::PersistInstruction persist_instruction) {
    return IoOperation{op_addresses, access_size, internal::OpType::Write, persist_instruction};
  }

  static IoOperation PauseOp(uint32_t duration) {
    static std::vector<char*> op_addresses{};
    return IoOperation{op_addresses, duration, internal::OpType::Pause, internal::PersistInstruction::None};
  }

 private:
  IoOperation(const std::vector<char*>& op_addresses, uint32_t access_size, internal::OpType op_type,
              internal::PersistInstruction persist_instruction)
      : op_addresses_{op_addresses},
        access_size_{access_size},
        op_type_{op_type},
        persist_instruction_{persist_instruction} {}

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
      case internal::PersistInstruction::Cache: {
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
      case internal::PersistInstruction::CacheInvalidate: {
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
      case internal::PersistInstruction::NoCache: {
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
      case internal::PersistInstruction::None: {
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

  // The order here is important. At the moment, we can fit this into 16 Byte. Reorder with care.
  const std::vector<char*>& op_addresses_;
  union {
    const uint32_t access_size_;
    const uint32_t duration_;
  };
  const internal::OpType op_type_;
  const internal::PersistInstruction persist_instruction_;
};

class ChainedOperation {
 public:
  ChainedOperation(const CustomOp& op, char* range_start, const size_t range_size)
      : range_start_(range_start),
        access_size_(op.size),
        range_size_(range_size),
        align_(-access_size_),
        type_(op.type),
        persist_instruction_(op.persist) {}

  inline void run(char* current_addr, char* dependent_addr) {
    if (type_ == internal::OpType::Read) {
      current_addr = get_random_address(dependent_addr);
      dependent_addr = run_read(current_addr);
    } else {
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
      case internal::PersistInstruction::Cache: {
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
      case internal::PersistInstruction::CacheInvalidate: {
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
      case internal::PersistInstruction::NoCache: {
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
      case internal::PersistInstruction::None: {
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
  const internal::OpType type_;
  const internal::PersistInstruction persist_instruction_;
};

}  // namespace perma
