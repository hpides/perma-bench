#pragma once

#include <thread>
#include <vector>

namespace perma {

namespace internal {

// Ensure that we group io operations so that they are at least 128 KiB
// in order to avoid expensive timing operations on tiny access sizes.
static constexpr uint32_t MIN_IO_OP_SIZE = 128 * 1024u;

enum Mode { Sequential, Sequential_Desc, Random };
enum DataInstruction { MOV, SIMD };
enum PersistInstruction { CLWB, NTSTORE, CLFLUSH, None };
enum RandomDistribution { Uniform, Zipf };

}  // namespace internal

class IoOperation {
 public:
  virtual void run() = 0;
  virtual bool is_active() const { return false; }
};

class ActiveIoOperation : public IoOperation {
  friend class Benchmark;

 public:
  explicit ActiveIoOperation(uint32_t access_size, uint32_t num_ops, internal::DataInstruction data_instruction,
                             internal::PersistInstruction persist_instruction)
      : num_ops_{num_ops},
        access_size_(access_size),
        data_instruction_{data_instruction},
        persist_instruction_{persist_instruction},
        op_addresses_{num_ops} {};

  bool is_active() const override { return true; }
  uint64_t get_io_size() const { return num_ops_ * access_size_; };

 protected:
  const uint32_t num_ops_;
  const uint32_t access_size_;
  const internal::DataInstruction data_instruction_;
  const internal::PersistInstruction persist_instruction_;
  std::vector<char*> op_addresses_;
};

class Pause : public IoOperation {
 public:
  explicit Pause(uint32_t length) : IoOperation(), length_{length} {}

  void run() final;
  uint32_t get_length() const;

 protected:
  const uint32_t length_;
};

class Read : public ActiveIoOperation {
 public:
  using ActiveIoOperation::ActiveIoOperation;
  void run() final;
};

class Write : public ActiveIoOperation {
 public:
  using ActiveIoOperation::ActiveIoOperation;
  void run() final;
};

}  // namespace perma
