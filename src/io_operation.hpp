#pragma once

#include <thread>
#include <vector>

namespace nvmbm {

namespace internal {

// Exactly 64 characters to write in one cache line.
static const char WRITE_DATA[] __attribute__((aligned(64))) =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

static const uint32_t number_ios = 1000;

}  // namespace internal

class IoOperation {
 public:
  virtual void run() = 0;
  virtual bool is_active() const { return false; }
};

class ActiveIoOperation : public IoOperation {
 public:
  ActiveIoOperation(char* start_addr, char* end_addr, uint32_t num_ops, uint32_t access_size, bool random);
  bool is_active() const override;

 protected:
  char* start_addr_;
  char* end_addr_;
  std::vector<char*> op_addresses_;
  const uint32_t number_ops_;
  const uint32_t access_size_;
  const bool random_;
};

class Pause : public IoOperation {
 public:
  explicit Pause(uint32_t length) : IoOperation(), length_{length} {}

  void run() final;

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

}  // namespace nvmbm
