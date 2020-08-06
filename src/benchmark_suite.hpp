#pragma once

namespace nvmbm {

class BenchmarkSuite {
private:
    std::list<Benchmark> benchmarks;
    BenchmarkFactory builder;
};

}  // namespace nvmbm

