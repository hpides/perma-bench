#pragma once

#include <thread>
#include <vector>

#include "read_write_ops.hpp"

namespace perma {

namespace internal {

enum Mode : uint8_t { Sequential, Sequential_Desc, Random };
enum RandomDistribution : uint8_t { Uniform, Zipf };

enum DataInstruction : uint8_t { MOV, SIMD };
enum PersistInstruction : uint8_t { Cache, NoCache, None };

enum OpType : uint8_t { Read, Write, Pause };

constexpr size_t MIN_IO_CHUNK_SIZE = 16 * 1024u;

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

  static IoOperation ReadOp(std::vector<char*>&& op_addresses, uint32_t access_size,
                            internal::DataInstruction data_instruction) {
    return IoOperation{std::move(op_addresses), access_size, internal::Read, data_instruction, internal::None};
  }

  static IoOperation WriteOp(std::vector<char*>&& op_addresses, uint32_t access_size,
                             internal::DataInstruction data_instruction,
                             internal::PersistInstruction persist_instruction) {
    return IoOperation{std::move(op_addresses), access_size, internal::Write, data_instruction, persist_instruction};
  }

  static IoOperation PauseOp(uint32_t duration) {
    return IoOperation{{}, duration, internal::Pause, internal::SIMD, internal::None};
  }

 private:
  IoOperation(std::vector<char*>&& op_addresses, uint32_t access_size, internal::OpType op_type,
              internal::DataInstruction data_instruction, internal::PersistInstruction persist_instruction)
      : op_addresses_{std::move(op_addresses)},
        access_size_{access_size},
        op_type_{op_type},
        data_instruction_{data_instruction},
        persist_instruction_{persist_instruction} {}

  void run_read() {
#ifdef HAS_AVX
    if (data_instruction_ == internal::SIMD) {
      switch (access_size_) {
        case 64:
          return rw_ops::simd_read(op_addresses_, access_size_);
        case 128:
          return rw_ops::simd_read_128(op_addresses_, access_size_);
        case 256:
          return rw_ops::simd_read_256(op_addresses_, access_size_);
        default:
          return rw_ops::simd_read_512(op_addresses_, access_size_);
      }
    }
#endif
    switch (access_size_) {
      case 64:
        return rw_ops::mov_read(op_addresses_, access_size_);
      case 128:
        return rw_ops::mov_read_128(op_addresses_, access_size_);
      case 256:
        return rw_ops::mov_read_256(op_addresses_, access_size_);
      default:
        return rw_ops::mov_read_512(op_addresses_, access_size_);
    }
  }

  void run_write() {
#ifdef HAS_AVX
    if (data_instruction_ == internal::SIMD) {
      switch (persist_instruction_) {
#ifdef HAS_CLWB
        case internal::PersistInstruction::Cache: {
          return rw_ops::simd_write_clwb(op_addresses_, access_size_);
        }
#endif
        case internal::PersistInstruction::NoCache: {
          return rw_ops::simd_write_nt(op_addresses_, access_size_);
        }
        case internal::PersistInstruction::None: {
          return rw_ops::simd_write_none(op_addresses_, access_size_);
        }
      }
    }
#endif
    switch (persist_instruction_) {
#ifdef HAS_CLWB
      case internal::PersistInstruction::Cache: {
        return rw_ops::mov_write_clwb(op_addresses_, access_size_);
      }
#endif
      case internal::PersistInstruction::NoCache: {
        return rw_ops::mov_write_nt(op_addresses_, access_size_);
      }
      case internal::PersistInstruction::None: {
        return rw_ops::mov_write_none(op_addresses_, access_size_);
      }
    }
  }

  // The order here is important. At the moment, we can fit this into 16 Byte. Reorder with care.
  const std::vector<char*> op_addresses_;
  union {
    const uint32_t access_size_;
    const uint32_t duration_;
  };
  const internal::OpType op_type_;
  const internal::DataInstruction data_instruction_;
  const internal::PersistInstruction persist_instruction_;
};

}  // namespace perma
