#include "read_benchmark.hpp"

#include <iostream>

#include "../utils.hpp"

namespace perma {

void ReadBenchmark::get_result() {}

void ReadBenchmark::set_up() {
  char* end_addr = pmem_file_ + get_length();
  // Create IOReadOperations
  for (uint32_t i = 1; i <= config_.number_operations_; i += internal::NUMBER_IO_OPERATIONS) {
    io_operations_.push_back(std::make_unique<Read>(pmem_file_, end_addr, internal::NUMBER_IO_OPERATIONS,
                                                    config_.access_size_, config_.exec_mode_));
    // Assumption: num_ops is multiple of internal::number_ios(1000)
    if (i % config_.pause_frequency_ == 0) {
      // Assumption: pause_frequency is multiple of internal:: number_ios (1000)
      auto pause_io = std::make_unique<Pause>(config_.pause_length_);
      io_operations_.push_back(std::move(pause_io));
    }
  }
}

size_t ReadBenchmark::get_length() { return config_.target_size_ * config_.number_operations_; }

ReadBenchmarkConfig ReadBenchmarkConfig::decode(const YAML::Node& raw_config_data) {
  ReadBenchmarkConfig read_bm_config{};
  try {
    for (const YAML::Node& node : raw_config_data) {
      get_if_present(node, "access_size", &read_bm_config.access_size_);
      get_if_present(node, "target_size", &read_bm_config.target_size_);
      get_if_present(node, "number_operations", &read_bm_config.number_operations_);
      get_if_present(node, "pause_frequency", &read_bm_config.pause_frequency_);
      get_if_present(node, "pause_length", &read_bm_config.pause_length_);
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
