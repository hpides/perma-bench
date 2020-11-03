#include "read_benchmark.hpp"

namespace perma {

void ReadBenchmark::set_up() {
  char* end_addr = pmem_file_ + get_length();
  uint64_t number_ios = config_.number_operations_ / internal::NUMBER_IO_OPERATIONS;
  io_operations_.reserve(config_.number_threads_);
  measurements_.resize(config_.number_threads_);
  pool_.resize(config_.number_threads_ - 1);
  for (uint16_t i = 0; i < config_.number_threads_; i++) {
    measurements_[i].reserve(number_ios);
    std::vector<std::unique_ptr<IoOperation>> io_ops{};
    io_ops.reserve(number_ios);

    // Create IOReadOperations
    for (uint32_t j = 1; j <= config_.number_operations_; j += internal::NUMBER_IO_OPERATIONS) {
      io_ops.push_back(std::make_unique<Read>(pmem_file_, end_addr, internal::NUMBER_IO_OPERATIONS,
                                              config_.access_size_, config_.exec_mode_));
      // Assumption: num_ops is multiple of internal::number_ios(1000)
      if (j % config_.pause_frequency_ == 0) {
        // Assumption: pause_frequency is multiple of internal:: number_ios (1000)
        io_ops.push_back(std::make_unique<Pause>(config_.pause_length_));
      }
    }
    io_operations_.push_back(std::move(io_ops));
  }
}

nlohmann::json ReadBenchmark::get_config() {
  return {{"number_operations", config_.number_operations_},
          {"access_size", config_.access_size_},
          {"target_size", config_.target_size_},
          {"pause_length", config_.pause_length_},
          {"pause_frequency", config_.pause_frequency_},
          {"exec_mode", config_.exec_mode_},
          {"number_threads", config_.number_threads_}};
}

size_t ReadBenchmark::get_length() { return config_.target_size_ * internal::BYTE_IN_MEBIBYTE; }

uint16_t ReadBenchmark::get_number_threads() { return config_.number_threads_; }

ReadBenchmarkConfig ReadBenchmarkConfig::decode(const YAML::Node& raw_config_data) {
  ReadBenchmarkConfig read_bm_config{};
  try {
    for (const YAML::Node& node : raw_config_data) {
      internal::get_if_present(node, "access_size", &read_bm_config.access_size_);
      internal::get_if_present(node, "target_size", &read_bm_config.target_size_);
      internal::get_if_present(node, "number_operations", &read_bm_config.number_operations_);
      internal::get_if_present(node, "pause_frequency", &read_bm_config.pause_frequency_);
      internal::get_if_present(node, "pause_length", &read_bm_config.pause_length_);
      internal::get_if_present(node, "number_threads", &read_bm_config.number_threads_);
      if (node["exec_mode"] != nullptr && node["exec_mode"].as<std::string>() == "random") {
        read_bm_config.exec_mode_ = internal::Mode::Random;
      }
      // Assumption: incorrect parameter is not caught
    }
  } catch (const YAML::InvalidNode& e) {
    throw std::runtime_error("Exception during config parsing: " + e.msg);
  }
  return read_bm_config;
}
}  // namespace perma
