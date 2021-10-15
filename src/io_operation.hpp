#pragma once

#include <thread>
#include <vector>

#include "fast_random.hpp"
#include "read_write_ops.hpp"
#include "spdlog/spdlog.h"
#include "utils.hpp"

namespace perma {

namespace internal {

enum Mode : uint8_t { Sequential, Sequential_Desc, Random, Custom };
enum RandomDistribution : uint8_t { Uniform, Zipf };

enum PersistInstruction : uint8_t { Cache, NoCache, None };

enum OpType : uint8_t { Read, Write, Pause };

enum NumaPattern : uint8_t { Near, Far };

}  // namespace internal

class IoOperation {
  friend class Benchmark;

 public:
  inline void run() {
    switch (op_type_) {
      case (internal::Read): {
        return run_read();
      }
      case (internal::Write): {
        return run_write();
      }
      case (internal::Pause): {
        return std::this_thread::sleep_for(std::chrono::microseconds(duration_));
      }
      default: {
        throw std::runtime_error("Unknown OpType!");
      }
    }
  }

  inline bool is_active() const { return op_type_ != internal::Pause; }
  inline bool is_read() const { return op_type_ == internal::Read; }
  inline bool is_write() const { return op_type_ == internal::Write; }
  inline bool is_pause() const { return op_type_ == internal::Pause; }

  static IoOperation ReadOp(const std::vector<char*>& op_addresses, uint32_t access_size) {
    return IoOperation{op_addresses, access_size, internal::Read, internal::None};
  }

  static IoOperation WriteOp(const std::vector<char*>& op_addresses, uint32_t access_size,
                             internal::PersistInstruction persist_instruction) {
    return IoOperation{op_addresses, access_size, internal::Write, persist_instruction};
  }

  static IoOperation PauseOp(uint32_t duration) {
    static std::vector<char*> op_addresses{};
    return IoOperation{op_addresses, duration, internal::Pause, internal::None};
  }

 private:
  IoOperation(const std::vector<char*>& op_addresses, uint32_t access_size, internal::OpType op_type,
              internal::PersistInstruction persist_instruction)
      : op_addresses_{op_addresses},
        access_size_{access_size},
        op_type_{op_type},
        persist_instruction_{persist_instruction} {}

  void run_read() {
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
  }

  void run_write() {
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

struct ChainedOperation {
  internal::OpType type;
  size_t access_size;
  char* range_start;
  size_t range_size;
  internal::PersistInstruction persist_instruction;
  ChainedOperation* next = nullptr;
  size_t range_mask = range_size - 1;

  void run(char* addr) {
    char* next_addr = addr;
    if (type == internal::OpType::Read) {
      next_addr = run_read(addr);
    } else {
      run_write(addr);
    }

    if (next) {
      return next->run(next_addr);
    }
  }

  char* run_read(char* addr) {
    __m512i read_value;
    switch (access_size) {
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
        read_value = rw_ops::simd_read(addr, access_size);
    }
    // Read from both ends to avoid narrowing of the 512 Bit instruction.
    uint64_t read_addr = read_value[0] + read_value[7];
    return range_start + ((read_addr + lehmer64()) & range_mask);
  }

  void run_write(char* addr) {
    switch (persist_instruction) {
#ifdef HAS_CLWB
      case internal::PersistInstruction::Cache: {
        switch (access_size) {
          case 64:
            return rw_ops::simd_write_clwb_64(addr);
          case 128:
            return rw_ops::simd_write_clwb_128(addr);
          case 256:
            return rw_ops::simd_write_clwb_256(addr);
          case 512:
            return rw_ops::simd_write_clwb_512(addr);
          default:
            return rw_ops::simd_write_clwb(addr, access_size);
        }
      }
#endif
      case internal::PersistInstruction::NoCache: {
        switch (access_size) {
          case 64:
            return rw_ops::simd_write_nt_64(addr);
          case 128:
            return rw_ops::simd_write_nt_128(addr);
          case 256:
            return rw_ops::simd_write_nt_256(addr);
          case 512:
            return rw_ops::simd_write_nt_512(addr);
          default:
            return rw_ops::simd_write_nt(addr, access_size);
        }
      }
      case internal::PersistInstruction::None: {
        switch (access_size) {
          case 64:
            return rw_ops::simd_write_none_64(addr);
          case 128:
            return rw_ops::simd_write_none_128(addr);
          case 256:
            return rw_ops::simd_write_none_256(addr);
          case 512:
            return rw_ops::simd_write_none_512(addr);
          default:
            return rw_ops::simd_write_none(addr, access_size);
        }
      }
    }
  }
};

}  // namespace perma
