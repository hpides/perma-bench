#include "src/benchmark.hpp"
#include "src/io_operation.hpp"
#include <iostream>

int main() {
    char *a, *b, *c, *d, *e, *f;
    nvmbm::Read io1 = nvmbm::Read(a, b,0,0,false);
    nvmbm::Read io2 = nvmbm::Read(c,d,0,0,false);
    nvmbm::Write io3 = nvmbm::Write(e,f,0,0,false);
    nvmbm::Pause io4 = nvmbm::Pause(2000000);

    std::vector<nvmbm::IoOperation *> io_operations;
    io_operations.push_back(&io1);
    io_operations.push_back(&io2);
    io_operations.push_back(&io3);
    io_operations.push_back(&io4);
    nvmbm::Benchmark bm = nvmbm::Benchmark(io_operations);
    bm.run();
    return 0;
}
