#pragma once
#include <libpmem.h>
#include <search.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <iostream>
#include <json.hpp>
#include <map>
#include <vector>

#include "io_operation.hpp"

namespace perma {

namespace internal {

enum BenchmarkOptions { invalidBenchmark, readBenchmark, writeBenchmark };

static const std::map<std::string, BenchmarkOptions> optionStrings{
    {"read_benchmark", BenchmarkOptions::readBenchmark}, {"write_benchmark", BenchmarkOptions::writeBenchmark}};

BenchmarkOptions resolve_benchmark_option(const std::string& benchmark_option);

struct Measurement {
  const std::chrono::high_resolution_clock::time_point start_ts;
  const std::chrono::high_resolution_clock::time_point end_ts;
};

template <typename T>
static void get_if_present(const YAML::Node& data, const std::string& name, T* attribute) {
  if (data[name] != nullptr) {
    *attribute = data[name].as<T>();
  }
};

}  // namespace internal

class Benchmark {
 public:
  void run();
  void generate_data();
  virtual void get_result() {
    nlohmann::json result;
    // TODO: add benchmark name
    result["config"] = get_config();
    nlohmann::json measurements = nlohmann::json::array();
    for (int i = 0; i < io_operations_.size(); i++) {
      if (io_operations_.at(i)->is_active()) {
        uint64_t latency = duration_to_nanoseconds_in_long(measurements_.at(i).end_ts - measurements_.at(i).start_ts);
        uint64_t start_ts = duration_to_nanoseconds_in_long(measurements_.at(i).start_ts.time_since_epoch());
        uint64_t end_ts = duration_to_nanoseconds_in_long(measurements_.at(i).end_ts.time_since_epoch());
        uint64_t data_size = dynamic_cast<ActiveIoOperation*>(io_operations_.at(i).get())->get_io_size();
        double bandwidth = data_size / (latency / 1e9);

        std::string type;
        if (typeid(*io_operations_.at(i).get()) == typeid(Read)) {
          type = "read";
        } else if (typeid(*io_operations_.at(i).get()) == typeid(Write)) {
          type = "write";
        }

        measurements += {{"type", type},           {"latency", latency},          {"bandwidth", bandwidth},
                         {"data_size", data_size}, {"start_timestamp", start_ts}, {"end_timestamp", end_ts}};
      } else {
        measurements += {{"type", "pause"}, {"length", dynamic_cast<Pause*>(io_operations_.at(i).get())->get_length()}};
      }
    }
    result["results"] = measurements;
    std::cout << result.dump() << std::endl;
  };
  virtual void set_up() = 0;
  virtual void tear_down() {
    if (pmem_file_ != nullptr) {
      pmem_unmap(pmem_file_, get_length());
    }
    std::filesystem::remove("/mnt/nvram-nvmbm/read_benchmark.file");
  }

 protected:
  virtual size_t get_length() = 0;
  virtual nlohmann::json get_config() = 0;
  char* pmem_file_{nullptr};
  std::vector<std::unique_ptr<IoOperation>> io_operations_;
  std::vector<internal::Measurement> measurements_;

 private:
  static unsigned long duration_to_nanoseconds_in_long(const std::chrono::high_resolution_clock::duration& duration) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  }
};

}  // namespace perma
