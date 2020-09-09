#include "read_benchmark.hpp"

#include <nvm_management.h>

#include <iostream>

#include "../utils.hpp"

namespace nvmbm {

void ReadBenchmark::getResult() {}

void ReadBenchmark::SetUp() {
  // Map read file
  size_t mapped_size;
  char* read_addr = map_pmem_file("/mnt/nvram-nvmbm/test.file", &mapped_size);

  // Create IOReadOperations
  for (uint32_t i = 0; i < config_.number_operations_;
       i = i + internal::number_ios) {  // Assumption: num_ops is multiple of
                                        // internal::number_ios(1000)
    if (i > 0 && i % config_.pause_frequency_ ==
                     0) {  // Assumption: pause_frequency is multiple of
                           // internal:: number_ios (1000)
      auto pause_io = std::make_unique<Pause>(config_.pause_length_);
      io_operations_.push_back(std::move(pause_io));
    }
    
    auto end_addr = mapped_size > config_.target_size_
                        ? read_addr + config_.target_size_
                        : read_addr + mapped_size;
    auto read_io = std::make_unique<Read>(
        read_addr, end_addr, internal::number_ios, config_.access_size_,
        (config_.exec_mode_ == internal::Mode::Random));
    io_operations_.push_back(std::move(read_io));
  }
}

void ReadBenchmark::TearDown() {}

ReadBenchmarkConfig ReadBenchmarkConfig::decode(const YAML::Node& init_data) {
  ReadBenchmarkConfig read_bm_config{};
  try {
    for (const YAML::Node& node : init_data) {
      getIfPresent("access_size", node, read_bm_config.access_size_);
      getIfPresent("target_size", node, read_bm_config.target_size_);
      getIfPresent("number_operations", node,
                   read_bm_config.number_operations_);
      getIfPresent("pause_frequency", node, read_bm_config.pause_frequency_);
      getIfPresent("pause_length", node, read_bm_config.pause_length_);
      if (node["exec_mode"] != nullptr &&
          node["exec_mode"].as<std::string>() == "random") {
        read_bm_config.exec_mode_ = internal::Mode::Random;
      }
    }
  } catch (const YAML::InvalidNode& e) {
    std::cerr << "Exception during config parsing: " << e.msg << std::endl;
  }
  return read_bm_config;
}
}  // namespace nvmbm
