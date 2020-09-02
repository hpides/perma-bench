#include "bm0.hpp"

#include <search.h>

#include <iostream>
namespace nvmbm {

bm0::bm0(const YAML::Node& init_data) {
  try {
    for (std::size_t i = 0; i < init_data.size(); i++) {
      if (init_data[i]["access_size"] != nullptr)
        access_size_ = init_data[i]["access_size"].as<unsigned int>();
      if (init_data[i]["target_size"] != nullptr)
        target_size_ = init_data[i]["target_size"].as<unsigned int>();
      if (init_data[i]["exec_mode"] != nullptr &&
          init_data[i]["exec_mode"].as<std::string>() == "random")
        exec_mode_ = internal::Random;
      if (init_data[i]["read_ratio"] != nullptr)
        read_ratio_ = init_data[i]["read_ratio"].as<float>();
      if (init_data[i]["write_ratio"] != nullptr)
        write_ratio_ = init_data[i]["write_ratio"].as<float>();
      if (init_data[i]["pause_frequency"] != nullptr)
        pause_frequency_ = init_data[i]["pause_frequency"].as<unsigned int>();
      if (init_data[i]["pause_length"] != nullptr)
        pause_length_ = init_data[i]["pause_length"].as<unsigned int>();
    }
  } catch (const YAML::InvalidNode& e) {
    std::cerr << "Exception during config parsing: " << e.msg << std::endl;
  }
  // Create io operations
  // When is map_pmem_file executed and where?
  // Same for create_pmem_file
  // build io operations in separate function?
}

void bm0::getResult() {}

void bm0::SetUp() {}

void bm0::TearDown() {}
}  // namespace nvmbm