#include "benchmark.hpp"

#include <memory>
#include <random>
#include <thread>

#include "utils.hpp"

namespace perma {

void run_in_thread(Benchmark* benchmark, const uint16_t thread_id) {
  for (const std::unique_ptr<IoOperation>& io_op : benchmark->io_operations_[thread_id]) {
    const auto start_ts = std::chrono::high_resolution_clock::now();
    io_op->run();
    const auto end_ts = std::chrono::high_resolution_clock::now();
    benchmark->measurements_[thread_id].emplace_back(start_ts, end_ts);
  }
}

void Benchmark::run() {
  for (size_t thread_index = 1; thread_index < config_.number_threads_; thread_index++) {
    pool_.emplace_back(&run_in_thread, this, thread_index);
  }

  run_in_thread(this, 0);

  // wait for all threads
  for (std::thread& thread : pool_) {
    thread.join();
  }
}

void Benchmark::generate_data() {
  size_t length = get_length_in_bytes();
  pmem_file_ = create_pmem_file("/mnt/nvram-nvmbm/read_benchmark.file", length);
  write_data(pmem_file_, pmem_file_ + length);
}

nlohmann::json Benchmark::get_result() {
  nlohmann::json result;
  result["benchmark_name"] = benchmark_name_;
  result["config"] = get_config();
  nlohmann::json result_points = nlohmann::json::array();
  for (int i = 0; i < config_.number_threads_; i++) {
    for (int j = 0; j < io_operations_.size(); j++) {
      const internal::Measurement measurement = measurements_[i].at(j);
      const IoOperation* io_op = io_operations_[i].at(j).get();

      if (io_op->is_active()) {
        uint64_t latency = duration_to_nanoseconds(measurement.end_ts_ - measurement.start_ts_);
        uint64_t start_ts = duration_to_nanoseconds(measurement.start_ts_.time_since_epoch());
        uint64_t end_ts = duration_to_nanoseconds(measurement.end_ts_.time_since_epoch());
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

void Benchmark::set_up() {
  const char* end_addr = pmem_file_ + get_length_in_bytes();
  const uint64_t num_io_chunks = config_.number_operations_ / internal::NUMBER_IO_OPERATIONS;

  io_operations_.reserve(config_.number_threads_);
  pool_.reserve(config_.number_threads_ - 1);
  measurements_.resize(config_.number_threads_);

  const uint16_t num_threads_per_partition = config_.number_threads_ / config_.number_partitions_;
  const uint64_t partition_size = get_length_in_bytes() / config_.number_partitions_;

  std::random_device rnd_device;
  std::mt19937_64 rnd_generator{rnd_device()};

  for (uint16_t partition_num = 0; partition_num < config_.number_partitions_; partition_num++) {
    char* partition_start = pmem_file_ + (partition_num * partition_size);
    const char* partition_end = partition_start + partition_size;

    for (uint16_t thread_num = 0; thread_num < num_threads_per_partition; thread_num++) {
      const uint32_t index = thread_num + (partition_num * num_threads_per_partition);
      measurements_[index].reserve(num_io_chunks);

      std::vector<std::unique_ptr<IoOperation>> io_ops{};
      const uint64_t num_pauses =
          config_.pause_frequency_ == 0 ? 0 : config_.number_operations_ / config_.pause_frequency_ - 1;
      io_ops.reserve(num_io_chunks + num_pauses);

      // Create IOOperations
      char* next_op_position = partition_start;

      std::uniform_real_distribution<double> io_mode_distribution(0, 1);

      for (uint32_t io_op = 1; io_op <= config_.number_operations_; io_op += internal::NUMBER_IO_OPERATIONS) {
        const double random_num = io_mode_distribution(rnd_generator);
        if (random_num < config_.read_ratio_) {
          io_ops.push_back(std::make_unique<Read>(config_.access_size_));
        } else {
          io_ops.push_back(std::make_unique<Write>(config_.access_size_));
        }

        std::array<char*, internal::NUMBER_IO_OPERATIONS>& op_addresses =
            dynamic_cast<ActiveIoOperation*>(io_ops.back().get())->op_addresses_;

        switch (config_.exec_mode_) {
          case internal::Mode::Random: {
            const ptrdiff_t range = partition_end - partition_start;
            const uint32_t num_accesses_in_range = range / config_.access_size_;

            std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range - 1);

            for (uint32_t op = 0; op < internal::NUMBER_IO_OPERATIONS; ++op) {
              op_addresses[op] = partition_start + (access_distribution(rnd_generator) * config_.access_size_);
            }
            break;
          }
          case internal::Mode::Sequential: {
            for (uint32_t op = 0; op < internal::NUMBER_IO_OPERATIONS; ++op) {
              op_addresses[op] = next_op_position + (op * config_.access_size_);
            }
            next_op_position += internal::NUMBER_IO_OPERATIONS * config_.access_size_;
            break;
          }
        }
        // Assumption: num_ops is multiple of internal::number_ios(1000)
        if (config_.pause_frequency_ != 0 && io_op % config_.pause_frequency_ == 0 &&
            io_op < config_.number_operations_) {
          // Assumption: pause_frequency is multiple of internal:: number_ios (1000)
          io_ops.push_back(std::make_unique<Pause>(config_.pause_length_micros_));
        }
      }
      io_operations_.push_back(std::move(io_ops));
    }
  }
}

size_t Benchmark::get_length_in_bytes() const { return config_.total_memory_range_ * internal::BYTE_IN_MEBIBYTE; }

nlohmann::json Benchmark::get_config() {
  return {{"total_memory_range", config_.total_memory_range_},
          {"access_size", config_.access_size_},
          {"number_operations", config_.number_operations_},
          {"exec_mode", config_.exec_mode_},
          {"write_ratio", config_.write_ratio_},
          {"read_ratio", config_.read_ratio_},
          {"pause_frequency", config_.pause_frequency_},
          {"pause_length_micros", config_.pause_length_micros_},
          {"number_threads", config_.number_threads_},
          {"number_threads", config_.number_threads_}};
}

BenchmarkConfig BenchmarkConfig::decode(const YAML::Node& raw_config_data) {
  BenchmarkConfig bm_config{};
  try {
    for (const YAML::Node& node : raw_config_data) {
      internal::get_if_present(node, "total_memory_range", &bm_config.total_memory_range_);
      internal::get_if_present(node, "access_size", &bm_config.access_size_);
      internal::get_if_present(node, "number_operations", &bm_config.number_operations_);

      internal::get_if_present(node, "write_ratio", &bm_config.write_ratio_);
      internal::get_if_present(node, "read_ratio", &bm_config.read_ratio_);

      internal::get_if_present(node, "pause_frequency", &bm_config.pause_frequency_);
      internal::get_if_present(node, "pause_length_micros", &bm_config.pause_length_micros_);

      internal::get_if_present(node, "number_partitions", &bm_config.number_partitions_);

      internal::get_if_present(node, "number_threads", &bm_config.number_threads_);

      if (node["exec_mode"] != nullptr && node["exec_mode"].as<std::string>() == "random") {
        bm_config.exec_mode_ = internal::Mode::Random;
      }
      // Assumption: incorrect parameter is not caught
    }
  } catch (const YAML::InvalidNode& e) {
    throw std::runtime_error("Exception during config parsing: " + e.msg);
  }
  return bm_config;
}
}  // namespace perma
