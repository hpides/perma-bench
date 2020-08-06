#pragma once

namespace nvmbm {

class Benchmark {
public:
    void run();

    void getResult();

private:
    std::list<IoOperation> io_operations;
};

}  // namespace nvmbm
