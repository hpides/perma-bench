#pragma once

#include <thread>
#include <vector>

namespace perma {

namespace internal {

// Exactly 64 characters to write in one cache line.
static const char WRITE_DATA[] __attribute__((aligned(64))) =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

static constexpr uint32_t NUM_IO_OPS_PER_CHUNK = 1000;

static constexpr size_t CACHE_LINE_SIZE = 64;

enum Mode { Sequential, Random };

}  // namespace internal

class IoOperation {
 public:
  virtual void run() = 0;
  virtual bool is_active() const { return false; }
};

class ActiveIoOperation : public IoOperation {
  friend class Benchmark;

 public:
  explicit ActiveIoOperation(uint32_t access_size) : access_size_(access_size){};
  bool is_active() const override { return true; }
  uint64_t get_io_size() const { return internal::NUM_IO_OPS_PER_CHUNK * access_size_; };

 protected:
  std::array<char*, internal::NUM_IO_OPS_PER_CHUNK> op_addresses_;
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

void write_data(char* from, const char* to);

}  // namespace perma
