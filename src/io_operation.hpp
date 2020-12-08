#pragma once

#include <thread>
#include <vector>

#include "read_write_ops.hpp"

namespace perma {

namespace internal {

enum Mode : uint8_t { Sequential, Sequential_Desc, Random };
enum RandomDistribution : uint8_t { Uniform, Zipf };

enum DataInstruction : uint8_t { MOV, SIMD };
enum PersistInstruction : uint8_t { NTSTORE, CLWB, CLFLUSH, NONE };

enum OpType : uint8_t { Read, Write, Pause };

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

  static IoOperation ReadOp(char* op_addr, uint32_t access_size, internal::DataInstruction data_instruction) {
    return IoOperation{op_addr, access_size, internal::Read, data_instruction, internal::NONE};
  }

  static IoOperation WriteOp(char* op_addr, uint32_t access_size, internal::DataInstruction data_instruction, internal::PersistInstruction persist_instruction) {
    return IoOperation{op_addr, access_size, internal::Write, data_instruction, persist_instruction};
  }

  static IoOperation PauseOp(uint32_t duration) {
    return IoOperation{nullptr, duration, internal::Pause, internal::SIMD, internal::NONE};
  }

 private:
  IoOperation(char* opAddr, uint32_t accessSize, internal::OpType opType, internal::DataInstruction dataInstruction, internal::PersistInstruction persistInstruction)
      : op_addr_{opAddr},
        access_size_{accessSize},
        op_type_{opType},
        data_instruction_{dataInstruction},
        persist_instruction_{persistInstruction} {
    static_assert(sizeof(*this) <= 16, "IoOperation too big.");
  }

  void run_read() {
#ifdef HAS_AVX
    if (data_instruction_ == internal::SIMD) {
      return rw_ops::simd_read(op_addr_, access_size_);
    }
#endif
    return rw_ops::mov_read(op_addr_, access_size_);
  }

  void run_write() {
#ifdef HAS_AVX
    if (data_instruction_ == internal::SIMD) {
      return rw_ops::simd_write(op_addr_, access_size_);
    }
#endif
    return rw_ops::mov_write(op_addr_, access_size_);
  }

  // The order here is important. At the moment, we can fit this into 16 Byte. Reorder with care.
  char* op_addr_;
  union {
    const uint32_t access_size_;
    const uint32_t duration_;
  };
  const internal::OpType op_type_;
  const internal::DataInstruction data_instruction_;
  const internal::PersistInstruction persist_instruction_;
};

}  // namespace perma
