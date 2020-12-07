#include "io_operation.hpp"

#include "read_write_ops.hpp"

namespace perma {

uint32_t Pause::get_length() const { return length_; }

void Pause::run() { std::this_thread::sleep_for(std::chrono::microseconds(length_)); }

void Read::run() {
#ifdef HAS_AVX
  if (data_instruction_ == internal::SIMD) {
    return rw_ops::simd_read(op_addresses_, access_size_);
  }
#endif
  return rw_ops::mov_read(op_addresses_, access_size_);
}

void Write::run() {
#ifdef HAS_AVX
  if (data_instruction_ == internal::SIMD) {
    switch (persist_instruction_) {
      case internal::PersistInstruction::CLWB: {
        return rw_ops::simd_write_clwb(op_addresses_, access_size_);
      }
      case internal::PersistInstruction::NTSTORE: {
        return rw_ops::simd_write_nt(op_addresses_, access_size_);
      }
      case internal::PersistInstruction::CLFLUSH: {
        return rw_ops::simd_write_clflush(op_addresses_, access_size_);
      }
      case internal::PersistInstruction::None: {
        return rw_ops::simd_write_none(op_addresses_, access_size_);
      }
    }
  }
#endif
  switch (persist_instruction_) {
    case internal::PersistInstruction::CLWB: {
      return rw_ops::mov_write_clwb(op_addresses_, access_size_);
    }
    case internal::PersistInstruction::NTSTORE: {
      return rw_ops::mov_write_nt(op_addresses_, access_size_);
    }
    case internal::PersistInstruction::CLFLUSH: {
      return rw_ops::mov_write_clflush(op_addresses_, access_size_);
    }
    case internal::PersistInstruction::None: {
      return rw_ops::mov_write_none(op_addresses_, access_size_);
    }
  }
}

}  // namespace perma
