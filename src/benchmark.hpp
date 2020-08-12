#pragma once

#include <vector>

#include "io_operation.hpp"

namespace nvmbm {

class Benchmark {
  public:
    virtual void run() = 0;

    virtual void getResult() = 0;

  private:
    std::vector<IoOperation> io_operations_;
};

}  // namespace nvmbm
