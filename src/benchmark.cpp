#include "benchmark.hpp"

#include <memory>
#include <random>
#include <thread>

#include "read_write_ops.hpp"
#include "utils.hpp"

namespace {

#define CHECK_ARGUMENT(exp, txt) \
  if (!exp) throw std::invalid_argument(txt)

constexpr auto VISITED_TAG = "visited";

void ensure_unique_key(const YAML::Node& entry, const std::string& name) {
  if (entry.Tag() == VISITED_TAG) {
    const YAML::Mark& mark = entry.Mark();
    throw std::invalid_argument("Duplicate entry: '" + name + "' (in line: " + std::to_string(mark.line) + ")");
  }
}

template <typename T>
bool get_if_present(YAML::Node& data, const std::string& name, T* attribute) {
  YAML::Node entry = data[name];
  if (!entry) {
    return false;
  }
  ensure_unique_key(entry, name);

  *attribute = entry.as<T>();
  entry.SetTag(VISITED_TAG);
  return true;
}

template <typename T>
bool get_enum_if_present(YAML::Node& data, const std::string& name, const std::unordered_map<std::string, T>& enum_map,
                         T* attribute) {
  YAML::Node entry = data[name];
  if (!entry) {
    return false;
  }
  ensure_unique_key(entry, name);

  const auto enum_key = entry.as<std::string>();
  auto it = enum_map.find(enum_key);
  if (it == enum_map.end()) {
    throw std::invalid_argument("Unknown '" + name + "': " + enum_key);
  }

  *attribute = it->second;
  entry.SetTag(VISITED_TAG);
  return true;
}

nlohmann::json hdr_histogram_to_json(hdr_histogram* hdr, const bool include_percentiles = false) {
  nlohmann::json result;
  result["max"] = hdr_max(hdr);
  result["avg"] = hdr_mean(hdr);
  result["min"] = hdr_min(hdr);
  result["std"] = hdr_stddev(hdr);
  result["median"] = hdr_value_at_percentile(hdr, 50.0);
  result["lower_quartile"] = hdr_value_at_percentile(hdr, 25.0);
  result["upper_quartile"] = hdr_value_at_percentile(hdr, 75.0);
  if (include_percentiles) {
    result["percentile_90"] = hdr_value_at_percentile(hdr, 90.0);
    result["percentile_95"] = hdr_value_at_percentile(hdr, 95.0);
    result["percentile_99"] = hdr_value_at_percentile(hdr, 99.0);
    result["percentile_999"] = hdr_value_at_percentile(hdr, 99.9);
    result["percentile_9999"] = hdr_value_at_percentile(hdr, 99.99);
  }
  return result;
}

}  // namespace

namespace perma {

struct ConfigEnums {
  static const std::unordered_map<std::string, internal::Mode> str_to_mode;
  static const std::unordered_map<std::string, internal::DataInstruction> str_to_data_instruction;
  static const std::unordered_map<std::string, internal::PersistInstruction> str_to_persist_instruction;
  static const std::unordered_map<std::string, internal::RandomDistribution> str_to_random_distribution;
};

const std::unordered_map<std::string, internal::Mode> ConfigEnums::str_to_mode{
    {"sequential", internal::Mode::Sequential},
    {"sequential_asc", internal::Mode::Sequential},
    {"sequential_desc", internal::Mode::Sequential_Desc},
    {"random", internal::Mode::Random}};

const std::unordered_map<std::string, internal::DataInstruction> ConfigEnums::str_to_data_instruction{
    {"simd", internal::DataInstruction::SIMD}, {"mov", internal::DataInstruction::MOV}};

const std::unordered_map<std::string, internal::PersistInstruction> ConfigEnums::str_to_persist_instruction{
    {"ntstore", internal::PersistInstruction::NTSTORE},
    {"clwb", internal::PersistInstruction::CLWB},
    {"clflush", internal::PersistInstruction::CLFLUSH}};

const std::unordered_map<std::string, internal::RandomDistribution> ConfigEnums::str_to_random_distribution{
    {"uniform", internal::RandomDistribution::Uniform}, {"zipf", internal::RandomDistribution::Zipf}};

void Benchmark::run_in_thread(const uint16_t thread_id) {
  for (const std::unique_ptr<IoOperation>& io_op : io_operations_[thread_id]) {
    const auto start_ts = std::chrono::high_resolution_clock::now();
    io_op->run();
    const auto end_ts = std::chrono::high_resolution_clock::now();
    measurements_[thread_id].emplace_back(start_ts, end_ts);
  }
}

void Benchmark::run() {
  for (size_t thread_index = 1; thread_index < config_.number_threads; thread_index++) {
    pool_.emplace_back(&Benchmark::run_in_thread, this, thread_index);
  }

  run_in_thread(0);

  // wait for all threads
  for (std::thread& thread : pool_) {
    thread.join();
  }
}

void Benchmark::create_data_file() {
  if (std::filesystem::exists(pmem_file_)) {
    // Data was already generated. Only re-map it.
    pmem_data_ = map_pmem_file(pmem_file_, config_.total_memory_range);
    return;
  }

  pmem_data_ = create_pmem_file(pmem_file_, config_.total_memory_range);
  if (config_.read_ratio > 0) {
    // If we read data in this benchmark, we need to generate it first.
    rw_ops::write_data(pmem_data_, pmem_data_ + config_.total_memory_range);
  }
}

nlohmann::json Benchmark::get_result() {
  nlohmann::json result;
  result["benchmark_name"] = benchmark_name_;
  result["config"] = get_json_config();
  nlohmann::json result_points = nlohmann::json::array();
  for (uint16_t thread_num = 0; thread_num < config_.number_threads; thread_num++) {
    const std::vector<std::unique_ptr<IoOperation>>& thread_io_ops = io_operations_[thread_num];
    const std::vector<internal::Measurement>& thread_measurements = measurements_[thread_num];

    for (size_t io_op_num = 0; io_op_num < thread_io_ops.size(); io_op_num++) {
      const internal::Measurement measurement = thread_measurements[io_op_num];
      const IoOperation* io_op = thread_io_ops[io_op_num].get();

      if (io_op->is_active()) {
        const auto* active_io_op = dynamic_cast<const ActiveIoOperation*>(io_op);
        const uint64_t latency = duration_to_nanoseconds(measurement.end_ts_ - measurement.start_ts_);
        const uint64_t start_ts = duration_to_nanoseconds(measurement.start_ts_.time_since_epoch());
        const uint64_t end_ts = duration_to_nanoseconds(measurement.end_ts_.time_since_epoch());
        const uint64_t data_size = active_io_op->get_io_size();
        const uint64_t bandwidth = data_size / (latency / static_cast<double>(internal::NANOSECONDS_IN_SECONDS));
        const double data_size_gib = data_size / static_cast<double>(internal::BYTE_IN_GIGABYTE);
        const double bandwidth_gib = bandwidth / static_cast<double>(internal::BYTE_IN_GIGABYTE);

        // Increase number of entries in histograms to get more accurate results for smaller benchmarks.
        const uint32_t num_ops = active_io_op->num_ops_;
        const uint64_t avg_latency = latency / num_ops;
        hdr_record_values(latency_hdr_, avg_latency, num_ops);
        hdr_record_value(bandwidth_hdr_, bandwidth);

        std::string type;
        if (typeid(*io_op) == typeid(Read)) {
          type = "read";
        } else if (typeid(*io_op) == typeid(Write)) {
          type = "write";
        } else {
          throw std::runtime_error{"Unknown IO operation in results"};
        }

        result_points += {{"type", type},
                          {"latency", latency},
                          {"bandwidth", bandwidth_gib},
                          {"data_size", data_size_gib},
                          {"start_timestamp", start_ts},
                          {"end_timestamp", end_ts},
                          {"thread_id", thread_num}};
      } else {
        result_points +=
            {{"type", "pause"}, {"length", dynamic_cast<const Pause*>(io_op)->get_length()}, {"thread_id", thread_num}};
      }
    }
  }
  if (config_.raw_results) {
    result["raw_results"] = result_points;
  }
  result["bandwidth"] = hdr_histogram_to_json(bandwidth_hdr_);
  result["latency"] = hdr_histogram_to_json(latency_hdr_, true);
  return result;
}

void Benchmark::set_up() {
  const size_t num_total_range_ops = config_.total_memory_range / config_.access_size;
  const size_t num_operations =
      (config_.exec_mode == internal::Random) ? config_.number_operations : num_total_range_ops;
  const size_t num_ops_per_chunk = std::ceil((double)internal::MIN_IO_OP_SIZE / config_.access_size);
  const size_t num_ops_per_thread = num_operations / config_.number_threads;
  const size_t num_io_chunks_per_thread = num_ops_per_thread / num_ops_per_chunk;

  io_operations_.reserve(config_.number_threads);
  pool_.reserve(config_.number_threads - 1);
  measurements_.resize(config_.number_threads);

  const uint16_t num_threads_per_partition = config_.number_threads / config_.number_partitions;
  const uint64_t partition_size = config_.total_memory_range / config_.number_partitions;

  std::random_device rnd_device;
  std::mt19937_64 rnd_generator{rnd_device()};

  for (uint16_t partition_num = 0; partition_num < config_.number_partitions; partition_num++) {
    char* partition_start;
    const char* partition_end;
    if (config_.exec_mode == internal::Sequential_Desc) {
      partition_start =
          pmem_data_ + ((config_.number_partitions - partition_num) * partition_size) - config_.access_size;
    } else {
      partition_start = pmem_data_ + (partition_num * partition_size);
    }

    for (uint16_t thread_num = 0; thread_num < num_threads_per_partition; thread_num++) {
      const uint32_t index = thread_num + (partition_num * num_threads_per_partition);
      measurements_[index].reserve(num_io_chunks_per_thread);

      std::vector<std::unique_ptr<IoOperation>> io_ops{};
      const uint64_t num_pauses = config_.pause_frequency == 0 ? 0 : num_ops_per_thread / config_.pause_frequency - 1;
      size_t current_pause_frequency_count = 0;
      io_ops.reserve(num_io_chunks_per_thread + num_pauses);

      // Create IOOperations
      char* next_op_position = partition_start;
      std::bernoulli_distribution io_mode_distribution(config_.read_ratio);

      for (uint32_t io_op = 1; io_op <= num_io_chunks_per_thread; ++io_op) {
        const bool is_read = io_mode_distribution(rnd_generator);
        if (is_read) {
          io_ops.push_back(std::make_unique<Read>(config_.access_size, num_ops_per_chunk, config_.data_instruction,
                                                  config_.persist_instruction));
        } else {
          io_ops.push_back(std::make_unique<Write>(config_.access_size, num_ops_per_chunk, config_.data_instruction,
                                                   config_.persist_instruction));
        }

        std::vector<char*>& op_addresses = dynamic_cast<ActiveIoOperation*>(io_ops.back().get())->op_addresses_;

        switch (config_.exec_mode) {
          case internal::Mode::Random: {
            const uint32_t num_accesses_in_range = partition_size / config_.access_size;

            std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range - 1);

            for (uint32_t op = 0; op < num_ops_per_chunk; ++op) {
              uint64_t random_value;
              if (config_.random_distribution == internal::RandomDistribution::Uniform) {
                random_value = access_distribution(rnd_generator);
              } else {
                random_value = zipf(config_.zipf_alpha, num_accesses_in_range);
              }
              op_addresses[op] = partition_start + (random_value * config_.access_size);
            }
            break;
          }
          case internal::Mode::Sequential: {
            for (uint32_t op = 0; op < num_ops_per_chunk; ++op) {
              op_addresses[op] = next_op_position + (op * config_.access_size);
            }
            next_op_position += num_ops_per_chunk * config_.access_size;
            break;
          }
          case internal::Mode::Sequential_Desc: {
            for (uint32_t op = 0; op < num_ops_per_chunk; ++op) {
              op_addresses[op] = next_op_position - (op * config_.access_size);
            }
            next_op_position -= num_ops_per_chunk * config_.access_size;
            break;
          }
        }

        current_pause_frequency_count += num_ops_per_chunk;
        if (num_pauses > 0 && current_pause_frequency_count >= config_.pause_frequency &&
            io_op < num_io_chunks_per_thread) {
          io_ops.push_back(std::make_unique<Pause>(config_.pause_length_micros));
          current_pause_frequency_count = 0;
        }
      }
      io_operations_.push_back(std::move(io_ops));
    }
  }

  // Initialize HdrHistrograms
  // 200 GiB in Byte as max value.
  hdr_init(1, 26843545600, 3, &bandwidth_hdr_);
  // 100 seconds in nanoseconds as max value.
  hdr_init(1, 100000000000, 3, &latency_hdr_);
}

void Benchmark::tear_down(const bool force) {
  if (pmem_data_ != nullptr) {
    pmem_unmap(pmem_data_, config_.total_memory_range);
    pmem_data_ = nullptr;
  }
  if (owns_pmem_file_ || force) {
    std::filesystem::remove(pmem_file_);
  }

  if (bandwidth_hdr_ != nullptr) {
    hdr_close(bandwidth_hdr_);
    bandwidth_hdr_ = nullptr;
  }
  if (latency_hdr_ != nullptr) {
    hdr_close(latency_hdr_);
    latency_hdr_ = nullptr;
  }
}

nlohmann::json Benchmark::get_json_config() {
  nlohmann::json config;
  config["total_memory_range"] = config_.total_memory_range;
  config["access_size"] = config_.access_size;
  config["exec_mode"] = config_.exec_mode;
  config["write_ratio"] = config_.write_ratio;
  config["read_ratio"] = config_.read_ratio;
  config["pause_frequency"] = config_.pause_frequency;
  config["number_partitions"] = config_.number_partitions;
  config["number_threads"] = config_.number_threads;

  if (config_.pause_frequency > 0) {
    config["pause_length_micros"] = config_.pause_length_micros;
  }

  if (config_.exec_mode == internal::Mode::Random) {
    config["number_operations"] = config_.number_operations;
    config["random_distribution"] = config_.random_distribution;
    if (config_.random_distribution == internal::Zipf) {
      config["zipf_alpha"] = config_.zipf_alpha;
    }
  }

  return config;
}

const std::string& Benchmark::benchmark_name() const { return benchmark_name_; }

const BenchmarkConfig& Benchmark::get_benchmark_config() const { return config_; }

BenchmarkConfig BenchmarkConfig::decode(YAML::Node& node) {
  BenchmarkConfig bm_config{};
  try {
    size_t num_found = 0;
    num_found += get_if_present(node, "total_memory_range", &bm_config.total_memory_range);
    num_found += get_if_present(node, "access_size", &bm_config.access_size);
    num_found += get_if_present(node, "number_operations", &bm_config.number_operations);
    num_found += get_if_present(node, "write_ratio", &bm_config.write_ratio);
    num_found += get_if_present(node, "read_ratio", &bm_config.read_ratio);
    num_found += get_if_present(node, "pause_frequency", &bm_config.pause_frequency);
    num_found += get_if_present(node, "pause_length_micros", &bm_config.pause_length_micros);
    num_found += get_if_present(node, "number_partitions", &bm_config.number_partitions);
    num_found += get_if_present(node, "number_threads", &bm_config.number_threads);
    num_found += get_if_present(node, "zipf_alpha", &bm_config.zipf_alpha);
    num_found += get_if_present(node, "raw_results", &bm_config.raw_results);
    num_found += get_enum_if_present(node, "exec_mode", ConfigEnums::str_to_mode, &bm_config.exec_mode);
    num_found += get_enum_if_present(node, "random_distribution", ConfigEnums::str_to_random_distribution,
                                     &bm_config.random_distribution);
    num_found += get_enum_if_present(node, "data_instruction", ConfigEnums::str_to_data_instruction,
                                     &bm_config.data_instruction);
    num_found += get_enum_if_present(node, "persist_instruction", ConfigEnums::str_to_persist_instruction,
                                     &bm_config.persist_instruction);

    if (num_found != node.size()) {
      for (YAML::const_iterator entry = node.begin(); entry != node.end(); ++entry) {
        if (entry->second.Tag() != VISITED_TAG) {
          throw std::invalid_argument("Unknown config entry '" + entry->first.as<std::string>() +
                                      "' in line: " + std::to_string(entry->second.Mark().line));
        }
      }
    }
  } catch (const YAML::InvalidNode& e) {
    throw std::invalid_argument("Exception during config parsing: " + e.msg);
  }

  bm_config.validate();
  return bm_config;
}

void BenchmarkConfig::validate() const {
  // Check if access size is at least 512-bit, i.e., 64byte (cache line)
  const bool is_access_size_greater_64_byte = access_size >= 64;
  CHECK_ARGUMENT(is_access_size_greater_64_byte, "Access size must be at least 64-byte, i.e., a cache line");

  // Check if access size is a power of two
  const bool is_access_size_power_of_two = (access_size & (access_size - 1)) == 0;
  CHECK_ARGUMENT(is_access_size_power_of_two, "Access size must be a power of 2");

  // Check if memory range is multiple of access size
  const bool is_memory_range_multiple_of_access_size = (total_memory_range % access_size) == 0;
  CHECK_ARGUMENT(is_memory_range_multiple_of_access_size, "Total memory range must be a multiple of twos");

  // Check if ratio is equal to one
  const bool is_ratio_equal_one = (read_ratio + write_ratio) == 1.0;
  CHECK_ARGUMENT(is_ratio_equal_one, "Read and write ratio must add up to 1");

  // Check if at least one thread
  const bool is_at_least_one_thread = number_threads > 0;
  CHECK_ARGUMENT(is_at_least_one_thread, "Number threads must be at least 1");

  // Check if at least one partition
  const bool is_at_least_one_partition = number_partitions > 0;
  CHECK_ARGUMENT(is_at_least_one_partition, "Number partitions must be at least 1");

  // Assumption: number_threads is multiple of number_partitions
  const bool is_number_threads_multiple_of_number_partitions = (number_threads % number_partitions) == 0;
  CHECK_ARGUMENT(is_number_threads_multiple_of_number_partitions,
                 "Number threads must be a multiple of number partitions");

  // Assumption: number_operations should only be set for random access. It is ignored in sequential IO.
  const bool is_number_operations_set_random =
      number_operations == BenchmarkConfig{}.number_operations || exec_mode == internal::Random;
  CHECK_ARGUMENT(is_number_operations_set_random, "Number of operations should only be set for random access");

  // Assumption: cannot insert a pause within a chunk. the frequency match be at least one chunks.
  const bool is_pause_frequency_chunkable =
      pause_frequency == 0 || (pause_frequency * access_size) >= internal::MIN_IO_OP_SIZE;
  CHECK_ARGUMENT(is_pause_frequency_chunkable, "Cannot insert pause in <" +
                                                   std::to_string(internal::MIN_IO_OP_SIZE / access_size) +
                                                   " frequency for " + std::to_string(access_size) + " byte access.");
}
}  // namespace perma
