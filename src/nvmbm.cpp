#include <libpmem.h>

#include <iostream>

#include "benchmark.hpp"
#include "benchmark_factory.hpp"
#include "io_operation.hpp"
#include "utils.hpp"

using namespace nvmbm;

int main() {
  std::vector<std::unique_ptr<Benchmark>> benchmarks =
      BenchmarkFactory::create_benchmarks("/hpi/fs00/home/leon.papke/nvmbm/benchmark_config.yaml");

  for (std::unique_ptr<Benchmark>& benchmark : benchmarks) {
    benchmark->SetUp();
    benchmark->run();
  }

  //  size_t mapped_size;
  //  void* read_addr = map_pmem_file("/mnt/nvram-nvmbm/test.file",
  //  &mapped_size); void* write_addr =
  //      create_pmem_file("/mnt/nvram-nvmbm/write-test.file", 1024l * 1024);
  //
  //  Read io1{read_addr, (char*)read_addr + 1024, 2, 64, false};
  //  Pause pause{1000};
  //  Read io2{read_addr, (char*)read_addr + 1024, 2, 64, false};
  //
  //  Write io3{write_addr, (char*)write_addr + 1024, 2, 64, false};
  //  Write io4{(char*)write_addr + 128, (char*)write_addr + 1024, 4, 64,
  //  false};
  //
  //  std::vector<IoOperation*> io_operations;
  //  io_operations.push_back(&io1);
  //  io_operations.push_back(&pause);
  //  io_operations.push_back(&io2);
  //  io_operations.push_back(&io3);
  //  io_operations.push_back(&io4);
  //  Benchmark bm{std::move(io_operations)};
  //  bm.run();
  return 0;
}
