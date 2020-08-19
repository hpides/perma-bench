#pragma once

#include <vector>

#include "io_operation.hpp"

namespace nvmbm {

class Benchmark {
  public:
    Benchmark(std::vector<IoOperation *> io_operations);
    void run();

    void getResult();

  private:
    std::vector<IoOperation *> io_operations_;
};

}  // namespace nvmbm
