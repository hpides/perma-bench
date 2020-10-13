#include "benchmark.hpp"

#include "utils.hpp"

namespace perma {

namespace internal {
BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option) {
  auto it = optionStrings.find(benchmark_option);
  if (it != optionStrings.end()) {
    return it->second;
  }
  return BenchmarkOptions::invalidBenchmark;
}
}  // namespace internal

void Benchmark::run() {
  measurements_.reserve(io_operations_.size());

  for (std::unique_ptr<IoOperation>& io_op : io_operations_) {
    const auto start_ts = std::chrono::high_resolution_clock::now();
    io_op->run();
    const auto end_ts = std::chrono::high_resolution_clock::now();
    measurements_.push_back(internal::Measurement{start_ts, end_ts});
  }
}

void Benchmark::generate_data() {
  size_t length = get_length();
  pmem_file_ = create_pmem_file("/mnt/nvram-nvmbm/read_benchmark.file", length);
  write_data(pmem_file_, pmem_file_ + length);
}

nlohmann::json Benchmark::get_result() {
  nlohmann::json result;
  result["benchmark_name"] = benchmark_name_;
  result["config"] = get_config();
  nlohmann::json result_points = nlohmann::json::array();
  for (int i = 0; i < io_operations_.size(); i++) {
    const internal::Measurement measurement = measurements_.at(i);
    const IoOperation* io_op = io_operations_.at(i).get();

    if (io_op->is_active()) {
      uint64_t latency = duration_to_nanoseconds(measurement.end_ts - measurement.start_ts);
      uint64_t start_ts = duration_to_nanoseconds(measurement.start_ts.time_since_epoch());
      uint64_t end_ts = duration_to_nanoseconds(measurement.end_ts.time_since_epoch());
      uint64_t data_size = dynamic_cast<const ActiveIoOperation*>(io_op)->get_io_size();
      double bandwidth = data_size / (latency / 1e9) / 1e9;  // First 1e9 nanoseconds to seconds. Second 1e9 B to GB

      std::string type;
      if (typeid(*io_op) == typeid(Read)) {
        type = "read";
      } else if (typeid(*io_op) == typeid(Write)) {
        type = "write";
      } else {
        throw std::runtime_error{"Unknown IO operation in results"};
      }

      result_points += {{"type", type},           {"latency", latency},          {"bandwidth", bandwidth},
                        {"data_size", data_size}, {"start_timestamp", start_ts}, {"end_timestamp", end_ts}};
    } else {
      result_points += {{"type", "pause"}, {"length", dynamic_cast<const Pause*>(io_op)->get_length()}};
    }
  }
  result["results"] = result_points;
  return result;
}

}  // namespace perma
