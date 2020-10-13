#include "write_benchmark.hpp"

namespace perma {

void WriteBenchmark::set_up() {
  char* end_addr = pmem_file_ + get_length();
  io_operations_.reserve(config_.number_operations_ / internal::NUMBER_IO_OPERATIONS);
  // Create IOReadOperations
  for (uint32_t i = 1; i <= config_.number_operations_; i += internal::NUMBER_IO_OPERATIONS) {
    io_operations_.push_back(std::make_unique<Write>(pmem_file_, end_addr, internal::NUMBER_IO_OPERATIONS,
                                                     config_.access_size_, config_.exec_mode_));
    // Assumption: num_ops is multiple of internal::number_ios(1000)
    if (i % config_.pause_frequency_ == 0) {
      // Assumption: pause_frequency is multiple of internal:: number_ios (1000)
      auto pause_io = std::make_unique<Pause>(config_.pause_length_);
      io_operations_.push_back(std::move(pause_io));
    }
  }
}

nlohmann::json WriteBenchmark::get_config() {
  return {{"number_operations", config_.number_operations_},
          {"access_size", config_.access_size_},
          {"target_size", config_.target_size_},
          {"pause_length", config_.pause_length_},
          {"pause_frequency", config_.pause_frequency_},
          {"exec_mode", config_.exec_mode_}};
}

size_t WriteBenchmark::get_length() { return config_.target_size_ * internal::BYTE_IN_MEBIBYTE; }

WriteBenchmarkConfig WriteBenchmarkConfig::decode(const YAML::Node& raw_config_data) {
  WriteBenchmarkConfig write_bm_config{};
  try {
    for (const YAML::Node& node : raw_config_data) {
      internal::get_if_present(node, "access_size", &write_bm_config.access_size_);
      internal::get_if_present(node, "target_size", &write_bm_config.target_size_);
      internal::get_if_present(node, "number_operations", &write_bm_config.number_operations_);
      internal::get_if_present(node, "pause_frequency", &write_bm_config.pause_frequency_);
      internal::get_if_present(node, "pause_length", &write_bm_config.pause_length_);
      if (node["exec_mode"] != nullptr && node["exec_mode"].as<std::string>() == "random") {
        write_bm_config.exec_mode_ = internal::Mode::Random;
      }
      // Assumption: incorrect parameter is not caught
    }
  } catch (const YAML::InvalidNode& e) {
    throw std::runtime_error("Exception during config parsing: " + e.msg);
  }
  return write_bm_config;
}
}  // namespace perma
