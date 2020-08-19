#include <iostream>

#include "benchmark.hpp"
#include "io_operation.hpp"

using namespace nvmbm;

int main() {
  char *a, *b, *c, *d, *e, *f;
  Read io1{a, b, 0, 0, false};
  Read io2{c, d, 0, 0, false};
  Write io3{e, f, 0, 0, false};
  Pause io4{1000};

  std::vector<IoOperation*> io_operations;
  io_operations.push_back(&io1);
  io_operations.push_back(&io2);
  io_operations.push_back(&io3);
  io_operations.push_back(&io4);
  Benchmark bm{std::move(io_operations)};
  bm.run();
  return 0;
}
