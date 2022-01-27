#pragma once

#include "benchmark.hpp"

namespace perma {

struct PermaOptions {
  const std::filesystem::path& pmem_directory;
  const std::filesystem::path& config_file;
  const std::filesystem::path& result_directory;
  const bool is_pmem;
};

class BenchmarkSuite {
 public:
  static void run_benchmarks(const PermaOptions& options);
};

}  // namespace perma
