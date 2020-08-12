#pragma once

#include <thread>
#include <vector>

namespace nvmbm {

namespace internal {
  static const char WRITE_DATA[64] __attribute__((aligned(64))) =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";

}  // namespace internal

class IoOperation {
  public:
    virtual void run() = 0;
    virtual bool is_active() { return false; }
};

class ActiveIoOperation : public IoOperation {
  public:
    ActiveIoOperation(void* start_addr, void* end_addr, uint32_t num_ops, uint32_t access_size, bool random);
    bool is_active() override;

  protected:
    void* start_addr_;
    void* end_addr_;
    std::vector<char*> read_addresses;
    const uint32_t number_ops_;
    const uint32_t access_size_;
    const bool random_;
};

class Pause: public IoOperation {
  public:
    explicit Pause(uint32_t length) : IoOperation(), length_{length} {}

    void run() final;

  protected:
    const uint32_t length_;
};

class Read : public ActiveIoOperation {
  public:
    void run() override;

};

class Write : public ActiveIoOperation {
  public:
    void run() override;
};

}  // namespace nvmbm
