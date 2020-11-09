#pragma once

#include <thread>
#include <vector>

namespace perma {

namespace internal {

static constexpr uint32_t NUM_IO_OPS_PER_CHUNK = 1000;

enum Mode { Sequential, Sequential_Desc, Random };
enum DataInstruction { MOV, SIMD };
enum PersistInstruction { CLWB, NTSTORE, CLFLUSH };

}  // namespace internal

class IoOperation {
 public:
  virtual void run() = 0;
  virtual bool is_active() const { return false; }
};

class ActiveIoOperation : public IoOperation {
  friend class Benchmark;

 public:
  explicit ActiveIoOperation(uint32_t access_size)
      : access_size_(access_size), op_addresses_(internal::NUM_IO_OPS_PER_CHUNK){};

  bool is_active() const override { return true; }
  uint64_t get_io_size() const { return internal::NUM_IO_OPS_PER_CHUNK * access_size_; };

 protected:
  std::vector<char*> op_addresses_;
  const uint32_t access_size_;
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
  void run() override;
};

class Write : public ActiveIoOperation {
 public:
  using ActiveIoOperation::ActiveIoOperation;
  void run() override;
};

}  // namespace perma
