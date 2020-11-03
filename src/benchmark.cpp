#include "benchmark.hpp"

#include <memory>
#include <thread>

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

void run_in_thread(Benchmark* benchmark, const uint16_t thread_id) {
  for (const std::unique_ptr<IoOperation>& io_op : benchmark->io_operations_[thread_id]) {
    const auto start_ts = std::chrono::high_resolution_clock::now();
    io_op->run();
    const auto end_ts = std::chrono::high_resolution_clock::now();
    benchmark->measurements_[thread_id].emplace_back(internal::Measurement{start_ts, end_ts});
  }
}

void Benchmark::run() {
  for (size_t thread_index = 0; thread_index < get_number_threads() - 1; thread_index++) {
    pool_.emplace_back(std::thread(&run_in_thread, this, thread_index + 1));
  }

  run_in_thread(this, 0);

  // wait for all threads
  for (std::thread& thread : pool_) {
    thread.join();
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
  for (int i = 0; i < get_number_threads(); i++) {
    for (int j = 0; j < io_operations_.size(); j++) {
      const internal::Measurement measurement = measurements_[i].at(j);
      const IoOperation* io_op = io_operations_[i].at(j).get();

      if (io_op->is_active()) {
        uint64_t latency = duration_to_nanoseconds(measurement.end_ts - measurement.start_ts);
        uint64_t start_ts = duration_to_nanoseconds(measurement.start_ts.time_since_epoch());
        uint64_t end_ts = duration_to_nanoseconds(measurement.end_ts.time_since_epoch());
        double data_size = dynamic_cast<const ActiveIoOperation*>(io_op)->get_io_size() /
                           static_cast<double>(internal::BYTE_IN_GIGABYTE);
        double bandwidth = data_size / (latency / static_cast<double>(internal::NANOSECONDS_IN_SECONDS));

        std::string type;
        if (typeid(*io_op) == typeid(Read)) {
          type = "read";
        } else if (typeid(*io_op) == typeid(Write)) {
          type = "write";
        } else {
          throw std::runtime_error{"Unknown IO operation in results"};
        }

        result_points += {{"type", type},           {"latency", latency},          {"bandwidth", bandwidth},
                          {"data_size", data_size}, {"start_timestamp", start_ts}, {"end_timestamp", end_ts},
                          {"thread_id", i}};
      } else {
        result_points +=
            {{"type", "pause"}, {"length", dynamic_cast<const Pause*>(io_op)->get_length()}, {"thread_id", i}};
      }
    }
  }
  result["results"] = result_points;
  return result;
}

}  // namespace perma
