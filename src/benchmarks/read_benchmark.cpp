#include "read_benchmark.hpp"

#include <iostream>

#include "../utils.hpp"

namespace perma {

void ReadBenchmark::getResult() {}

void ReadBenchmark::SetUp() {
  // Map read file
  size_t mapped_size;
  char* read_addr = map_pmem_file("/mnt/nvram-nvmbm/test.file", &mapped_size);

  // Create IOReadOperations
  for (uint32_t i = 0; i < config_.number_operations_; i += internal::NUMBER_IO_OPERATIONS) {
    // Assumption: num_ops is multiple of internal::number_ios(1000)
    if (i > 0 && i % config_.pause_frequency_ == 0) {
      // Assumption: pause_frequency is multiple of internal:: number_ios (1000)
      auto pause_io = std::make_unique<Pause>(config_.pause_length_);
      io_operations_.push_back(std::move(pause_io));
    }

    char* end_addr = mapped_size > config_.target_size_ ? read_addr + config_.target_size_ : read_addr + mapped_size;
    io_operations_.push_back(std::make_unique<Read>(read_addr, end_addr, internal::NUMBER_IO_OPERATIONS,
                                                    config_.access_size_, config_.exec_mode_));
  }
}

void ReadBenchmark::TearDown() {}

ReadBenchmarkConfig ReadBenchmarkConfig::decode(const YAML::Node& raw_config_data) {
  ReadBenchmarkConfig read_bm_config{};
  try {
    for (const YAML::Node& node : raw_config_data) {
      getIfPresent(node, "access_size", &read_bm_config.access_size_);
      getIfPresent(node, "target_size", &read_bm_config.target_size_);
      getIfPresent(node, "number_operations", &read_bm_config.number_operations_);
      getIfPresent(node, "pause_frequency", &read_bm_config.pause_frequency_);
      getIfPresent(node, "pause_length", &read_bm_config.pause_length_);
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
