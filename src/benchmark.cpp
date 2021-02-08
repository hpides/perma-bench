#include "benchmark.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
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

template <typename T>
std::string get_enum_as_string(const std::unordered_map<std::string, T>& enum_map, T value) {
  for (auto it = enum_map.cbegin(); it != enum_map.cend(); ++it) {
    if (it->second == value) {
      return it->first;
    }
  }
  throw std::invalid_argument("Unknown enum value for " + std::string(typeid(T).name()));
}

nlohmann::json hdr_histogram_to_json(hdr_histogram* hdr) {
  nlohmann::json result;
  result["max"] = hdr_max(hdr);
  result["avg"] = hdr_mean(hdr);
  result["min"] = hdr_min(hdr);
  result["std_dev"] = hdr_stddev(hdr);
  result["median"] = hdr_value_at_percentile(hdr, 50.0);
  result["lower_quartile"] = hdr_value_at_percentile(hdr, 25.0);
  result["upper_quartile"] = hdr_value_at_percentile(hdr, 75.0);
  result["percentile_90"] = hdr_value_at_percentile(hdr, 90.0);
  result["percentile_95"] = hdr_value_at_percentile(hdr, 95.0);
  result["percentile_99"] = hdr_value_at_percentile(hdr, 99.0);
  result["percentile_999"] = hdr_value_at_percentile(hdr, 99.9);
  result["percentile_9999"] = hdr_value_at_percentile(hdr, 99.99);
  return result;
}

inline double get_bandwidth(const uint64_t total_data_size, const uint64_t total_duration) {
  const double duration_in_s = total_duration / static_cast<double>(perma::internal::NANOSECONDS_IN_SECONDS);
  const double data_in_gib = total_data_size / static_cast<double>(perma::internal::BYTE_IN_GIGABYTE);
  return data_in_gib / duration_in_s;
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
    {"nocache", internal::PersistInstruction::NoCache},
    {"cache", internal::PersistInstruction::Cache},
    {"none", internal::PersistInstruction::None}};

const std::unordered_map<std::string, internal::RandomDistribution> ConfigEnums::str_to_random_distribution{
    {"uniform", internal::RandomDistribution::Uniform}, {"zipf", internal::RandomDistribution::Zipf}};

const std::string& Benchmark::benchmark_name() const { return benchmark_name_; }

void UnaryBenchmark::run() {
  for (size_t thread_index = 0; thread_index < config_.number_threads; thread_index++) {
    pool_.emplace_back(&run_in_thread, std::ref(thread_configs_[thread_index]), std::ref(config_));
  }

  // wait for all threads
  for (std::thread& thread : pool_) {
    thread.join();
  }
}

void UnaryBenchmark::create_data_file() { create_single_data_file(config_, pmem_data_, pmem_file_); }

void UnaryBenchmark::set_up() { single_set_up(config_, pmem_data_, result_, pool_, thread_configs_); }

void UnaryBenchmark::tear_down(const bool force) {
  if (pmem_data_ != nullptr) {
    pmem_unmap(pmem_data_, config_.total_memory_range);
    pmem_data_ = nullptr;
  }
  if (owns_pmem_file_ || force) {
    std::filesystem::remove(pmem_file_);
  }
}

nlohmann::json UnaryBenchmark::get_result_as_json() {
  nlohmann::json result;
  result["config"] = get_json_config();
  result.update(result_->get_result_as_json());
  return result;
}

nlohmann::json UnaryBenchmark::get_json_config() { return get_benchmark_config_as_json(config_); }

UnaryBenchmark::UnaryBenchmark(std::string benchmark_name, const BenchmarkConfig& config)
    : Benchmark(std::move(benchmark_name)),
      pmem_file_{generate_random_file_name(config.pmem_directory)},
      owns_pmem_file_{true},
      config_{config},
      result_{std::make_unique<BenchmarkResult>(config)} {}

UnaryBenchmark::UnaryBenchmark(std::string benchmark_name, const BenchmarkConfig& config,
                               std::filesystem::path pmem_file)
    : Benchmark(std::move(benchmark_name)),
      pmem_file_{std::move(pmem_file)},
      owns_pmem_file_{false},
      config_{config},
      result_{std::make_unique<BenchmarkResult>(config)} {}

const BenchmarkConfig& UnaryBenchmark::get_benchmark_config() const { return config_; }

const std::filesystem::path& UnaryBenchmark::get_pmem_file() const { return pmem_file_; }

bool UnaryBenchmark::owns_pmem_file() const { return owns_pmem_file_; }

const char* UnaryBenchmark::get_pmem_data() const { return pmem_data_; }

const std::vector<ThreadRunConfig>& UnaryBenchmark::get_thread_configs() const { return thread_configs_; }

const BenchmarkResult& UnaryBenchmark::get_benchmark_result() const { return *result_; }

BenchmarkResult::BenchmarkResult(const BenchmarkConfig& config) : config{config}, latency_hdr{nullptr} {
  // Initialize HdrHistrogram
  // 100 seconds in nanoseconds as max value.
  hdr_init(1, 100000000000, 3, &latency_hdr);
}

BenchmarkResult::~BenchmarkResult() {
  if (latency_hdr != nullptr) {
    hdr_close(latency_hdr);
  }

  raw_measurements.clear();
  raw_measurements.shrink_to_fit();
  latencies.clear();
  latencies.shrink_to_fit();
}

nlohmann::json BenchmarkResult::get_result_as_json() const {
  nlohmann::json result;
  nlohmann::json result_points = nlohmann::json::array();

  uint64_t total_read_size = 0;
  uint64_t total_write_size = 0;
  uint64_t total_read_duration = 0;
  uint64_t total_write_duration = 0;
  const std::vector<internal::Measurement> dummy_measurements{};
  const std::vector<internal::Latency> dummy_latencies{};

  assert(!raw_measurements.empty() || !latencies.empty());
  const bool is_raw = config.raw_results;
  const size_t num_ops = is_raw ? raw_measurements[0].size() : latencies[0].size();
  assert(num_ops > 0);
  const uint64_t data_size = config.access_size;
  const uint64_t num_ops_per_chunk = std::max(internal::MIN_IO_CHUNK_SIZE / data_size, 1ul);
  const uint64_t chunk_size = std::max(internal::MIN_IO_CHUNK_SIZE, data_size);

  for (uint16_t thread_num = 0; thread_num < config.number_threads; thread_num++) {
    const std::vector<internal::Measurement>& thread_measurements =
        is_raw ? raw_measurements[thread_num] : dummy_measurements;
    const std::vector<internal::Latency>& thread_latencies = is_raw ? dummy_latencies : latencies[thread_num];

    for (size_t io_chunk = 0; io_chunk < num_ops; ++io_chunk) {
      internal::Latency latency = is_raw ? thread_measurements[io_chunk].latency : thread_latencies[io_chunk];
      const uint64_t duration = latency.duration;
      const uint64_t avg_duration = duration / num_ops_per_chunk;

      const internal::OpType op_type = latency.op_type;
      if (op_type == internal::Pause && is_raw) {
        result_points += {{"type", "pause"}, {"length", duration}, {"thread_id", thread_num}};
        continue;
      }

      hdr_record_value(latency_hdr, avg_duration);

      if (op_type == internal::Read) {
        total_read_size += chunk_size;
        total_read_duration += duration;
      } else if (op_type == internal::Write) {
        total_write_size += chunk_size;
        total_write_duration += duration;
      } else {
        throw std::runtime_error{"Unknown IO operation in results"};
      }

      if (is_raw) {
        const internal::Measurement measurement = thread_measurements[io_chunk];
        const uint64_t start_ts = duration_to_nanoseconds(measurement.start_ts.time_since_epoch());
        const uint64_t end_ts = start_ts + duration;
        const uint64_t bandwidth = chunk_size / (duration / static_cast<double>(internal::NANOSECONDS_IN_SECONDS));
        const double bandwidth_gib = bandwidth / static_cast<double>(internal::BYTE_IN_GIGABYTE);

        result_points += {{"type", op_type == internal::Write ? "write" : "read"},
                          {"latency", duration},
                          {"bandwidth", bandwidth_gib},
                          {"data_size", chunk_size},
                          {"start_timestamp", start_ts},
                          {"end_timestamp", end_ts},
                          {"thread_id", thread_num}};
      }
    }
  }

  if (is_raw) {
    result["raw_results"] = result_points;
  }

  nlohmann::json bandwidth_results;
  if (total_read_duration > 0) {
    const uint64_t read_execution_time = total_read_duration / config.number_threads;
    bandwidth_results["read"] = get_bandwidth(total_read_size, read_execution_time);
  }
  if (total_write_duration > 0) {
    const uint64_t write_execution_time = total_write_duration / config.number_threads;
    bandwidth_results["write"] = get_bandwidth(total_write_size, write_execution_time);
  }

  result["bandwidth"] = bandwidth_results;
  result["duration"] = hdr_histogram_to_json(latency_hdr);

  return result;
}

BenchmarkConfig BenchmarkConfig::decode(YAML::Node& node) {
  BenchmarkConfig bm_config{};
  try {
    size_t num_found = 0;
    num_found += get_if_present(node, "total_memory_range", &bm_config.total_memory_range);
    num_found += get_if_present(node, "access_size", &bm_config.access_size);
    num_found += get_if_present(node, "number_operations", &bm_config.number_operations);
    num_found += get_if_present(node, "write_ratio", &bm_config.write_ratio);
    num_found += get_if_present(node, "pause_frequency", &bm_config.pause_frequency);
    num_found += get_if_present(node, "pause_length_micros", &bm_config.pause_length_micros);
    num_found += get_if_present(node, "number_partitions", &bm_config.number_partitions);
    num_found += get_if_present(node, "number_threads", &bm_config.number_threads);
    num_found += get_if_present(node, "zipf_alpha", &bm_config.zipf_alpha);
    num_found += get_if_present(node, "raw_results", &bm_config.raw_results);
    num_found += get_if_present(node, "prefault_file", &bm_config.prefault_file);
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
  CHECK_ARGUMENT(is_memory_range_multiple_of_access_size, "Total memory range must be a multiple access size.");

  // Check if ratio is between one and zero
  const bool is_ratio_between_one_zero = 0 <= write_ratio && write_ratio <= 1;
  CHECK_ARGUMENT(is_ratio_between_one_zero, "Write ratio must be between 0 and 1");

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

  // Assumption: sequential access does not make sense if we mix reads and writes
  const bool is_mixed_workload_random = (write_ratio == 1 || write_ratio == 0) || exec_mode == internal::Random;
  CHECK_ARGUMENT(is_mixed_workload_random, "Mixed read/write workloads only supported for random execution.");

  // Assumption: total memory needs to fit into N chunks exactly
  const bool is_seq_total_memory_chunkable =
      exec_mode == internal::Random || (total_memory_range % internal::MIN_IO_CHUNK_SIZE) == 0;
  CHECK_ARGUMENT(is_seq_total_memory_chunkable,
                 "Total file size needs to be multiple of " + std::to_string(internal::MIN_IO_CHUNK_SIZE));

  // Assumption: we chunk operations and we need enough data to fill at least one chunk
  const bool is_total_memory_large_enough = (total_memory_range / number_threads) >= internal::MIN_IO_CHUNK_SIZE;
  CHECK_ARGUMENT(is_total_memory_large_enough,
                 "Each thread needs at least " + std::to_string(internal::MIN_IO_CHUNK_SIZE) + " memory.");

  const bool is_pause_freq_chunkable =
      pause_frequency == 0 || pause_frequency >= (internal::MIN_IO_CHUNK_SIZE / access_size);
  CHECK_ARGUMENT(is_pause_freq_chunkable, "Cannot insert pauses with single chunk of " +
                                              std::to_string(internal::MIN_IO_CHUNK_SIZE / access_size) + " ops (" +
                                              std::to_string(internal::MIN_IO_CHUNK_SIZE) + " Byte / " +
                                              std::to_string(access_size) + " Byte) in this configuration.");
}

BinaryBenchmark::BinaryBenchmark(std::string benchmark_name, std::string first_benchmark_name,
                                 std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                 const BenchmarkConfig& second_config)
    : Benchmark(std::move(benchmark_name)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)},
      pmem_file_one_{generate_random_file_name(first_config.pmem_directory)},
      pmem_file_two_{generate_random_file_name(second_config.pmem_directory)},
      owns_pmem_file_one_{true},
      owns_pmem_file_two_{true},
      config_one_{first_config},
      config_two_{second_config},
      result_one_{std::make_unique<BenchmarkResult>(first_config)},
      result_two_{std::make_unique<BenchmarkResult>(second_config)} {}

BinaryBenchmark::BinaryBenchmark(std::string benchmark_name, std::string first_benchmark_name,
                                 std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                 const BenchmarkConfig& second_config, std::filesystem::path pmem_file_first)
    : Benchmark(std::move(benchmark_name)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)},
      pmem_file_one_{std::move(pmem_file_first)},
      pmem_file_two_{generate_random_file_name(second_config.pmem_directory)},
      owns_pmem_file_one_{false},
      owns_pmem_file_two_{true},
      config_one_{first_config},
      config_two_{second_config},
      result_one_{std::make_unique<BenchmarkResult>(first_config)},
      result_two_{std::make_unique<BenchmarkResult>(second_config)} {}

BinaryBenchmark::BinaryBenchmark(std::string benchmark_name, std::string first_benchmark_name,
                                 std::string second_benchmark_name, const BenchmarkConfig& first_config,
                                 const BenchmarkConfig& second_config, std::filesystem::path pmem_file_first,
                                 std::filesystem::path pmem_file_second)
    : Benchmark(std::move(benchmark_name)),
      benchmark_name_one_{std::move(first_benchmark_name)},
      benchmark_name_two_{std::move(second_benchmark_name)},
      pmem_file_one_{std::move(pmem_file_first)},
      pmem_file_two_{std::move(pmem_file_second)},
      owns_pmem_file_one_{false},
      owns_pmem_file_two_{false},
      config_one_{first_config},
      config_two_{second_config},
      result_one_{std::make_unique<BenchmarkResult>(first_config)},
      result_two_{std::make_unique<BenchmarkResult>(second_config)} {}

void BinaryBenchmark::run() {
  for (size_t thread_index = 0; thread_index < config_one_.number_threads; thread_index++) {
    pool_one_.emplace_back(&run_in_thread, std::ref(thread_configs_one_[thread_index]), std::ref(config_one_));
  }
  for (size_t thread_index = 0; thread_index < config_two_.number_threads; thread_index++) {
    pool_two_.emplace_back(&run_in_thread, std::ref(thread_configs_two_[thread_index]), std::ref(config_two_));
  }

  // wait for all threads
  for (std::thread& thread : pool_one_) {
    thread.join();
  }
  for (std::thread& thread : pool_two_) {
    thread.join();
  }
}

void BinaryBenchmark::create_data_file() {
  create_single_data_file(config_one_, pmem_data_one_, pmem_file_one_);
  create_single_data_file(config_two_, pmem_data_two_, pmem_file_two_);
}

void BinaryBenchmark::set_up() {
  single_set_up(config_one_, pmem_data_one_, result_one_, pool_one_, thread_configs_one_);
  single_set_up(config_two_, pmem_data_two_, result_two_, pool_two_, thread_configs_two_);
}

void BinaryBenchmark::tear_down(bool force) {
  if (pmem_data_one_ != nullptr) {
    pmem_unmap(pmem_data_one_, config_one_.total_memory_range);
    pmem_data_one_ = nullptr;
  }
  if (pmem_data_two_ != nullptr) {
    pmem_unmap(pmem_data_two_, config_two_.total_memory_range);
    pmem_data_two_ = nullptr;
  }

  if (owns_pmem_file_one_ || force) {
    std::filesystem::remove(pmem_file_one_);
  }
  if (owns_pmem_file_two_ || force) {
    std::filesystem::remove(pmem_file_two_);
  }
}

nlohmann::json BinaryBenchmark::get_result_as_json() {
  nlohmann::json result;
  result["config_one"] = get_json_config_one();
  result["name_one"] = get_benchmark_name_one();
  result["config_two"] = get_json_config_two();
  result["name_two"] = get_benchmark_name_two();
  result["result_one"].update(result_one_->get_result_as_json());
  result["result_two"].update(result_two_->get_result_as_json());
  return result;
}

const BenchmarkConfig& BinaryBenchmark::get_benchmark_config_one() const { return config_one_; }

const BenchmarkConfig& BinaryBenchmark::get_benchmark_config_two() const { return config_two_; }

nlohmann::json BinaryBenchmark::get_json_config_one() { return get_benchmark_config_as_json(config_one_); }

nlohmann::json BinaryBenchmark::get_json_config_two() { return get_benchmark_config_as_json(config_two_); }

const std::string& BinaryBenchmark::get_benchmark_name_one() const { return benchmark_name_one_; }

const std::string& BinaryBenchmark::get_benchmark_name_two() const { return benchmark_name_two_; }

inline void single_set_up(const BenchmarkConfig& config, char* pmem_data, std::unique_ptr<BenchmarkResult>& result,
                          std::vector<std::thread>& pool, std::vector<ThreadRunConfig>& thread_config) {
  const size_t num_total_range_ops = config.total_memory_range / config.access_size;
  const size_t num_operations = (config.exec_mode == internal::Random) ? config.number_operations : num_total_range_ops;
  const size_t num_ops_per_thread = num_operations / config.number_threads;

  pool.reserve(config.number_threads);
  thread_config.reserve(config.number_threads);

  if (config.raw_results) {
    result->raw_measurements.resize(config.number_threads);
  } else {
    result->latencies.resize(config.number_threads);
  }

  const uint16_t num_threads_per_partition = config.number_threads / config.number_partitions;
  const uint64_t partition_size = config.total_memory_range / config.number_partitions;

  for (uint16_t partition_num = 0; partition_num < config.number_partitions; partition_num++) {
    char* partition_start =
        (config.exec_mode == internal::Sequential_Desc)
            ? pmem_data + ((config.number_partitions - partition_num) * partition_size) - config.access_size
            : partition_start = pmem_data + (partition_num * partition_size);

    for (uint16_t thread_num = 0; thread_num < num_threads_per_partition; thread_num++) {
      const uint32_t index = thread_num + (partition_num * num_threads_per_partition);
      if (config.raw_results) {
        result->raw_measurements[index].reserve(num_ops_per_thread);
      } else {
        result->latencies[index].reserve(num_ops_per_thread);
      }
      thread_config.emplace_back(partition_start, partition_size, num_threads_per_partition, thread_num,
                                 num_ops_per_thread, &result->raw_measurements[index], &result->latencies[index],
                                 config);
    }
  }
}

inline void create_single_data_file(const BenchmarkConfig& config, char*& pmem_data, std::filesystem::path& pmem_file) {
  if (std::filesystem::exists(pmem_file)) {
    // Data was already generated. Only re-map it.
    pmem_data = map_pmem_file(pmem_file, config.total_memory_range);
    return;
  }

  pmem_data = create_pmem_file(pmem_file, config.total_memory_range);
  if (config.write_ratio < 1) {
    // If we read data in this benchmark, we need to generate it first.
    generate_read_data(pmem_data, config.total_memory_range);
  }
  if (config.write_ratio == 1 && config.prefault_file) {
    prefault_file(pmem_data, config.total_memory_range);
  }
}

inline void run_in_thread(const ThreadRunConfig& thread_config, const BenchmarkConfig& config) {
  const size_t ops_per_iteration = thread_config.num_threads_per_partition * config.access_size;
  const uint32_t num_accesses_in_range = thread_config.partition_size / config.access_size;
  const bool is_read_only = config.write_ratio == 0;
  const bool is_write_only = config.write_ratio == 1;
  const bool has_pause = config.pause_frequency > 0;
  size_t current_pause_frequency_count = 0;
  bool is_read = is_read_only;
  assert(is_write_only || is_read_only || config.exec_mode == internal::Random);

  const size_t thread_partition_offset = thread_config.thread_num * config.access_size;
  char* next_op_position = config.exec_mode == internal::Sequential_Desc
                               ? thread_config.partition_start_addr - thread_partition_offset
                               : thread_config.partition_start_addr + thread_partition_offset;

  std::random_device rnd_device;
  std::mt19937_64 rnd_generator{rnd_device()};
  std::bernoulli_distribution io_mode_distribution(1 - config.write_ratio);
  std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range - 1);

  const size_t ops_per_chunk =
      config.access_size < internal::MIN_IO_CHUNK_SIZE ? internal::MIN_IO_CHUNK_SIZE / config.access_size : 1;
  const size_t num_chunks = thread_config.num_ops / ops_per_chunk;

  for (uint32_t io_chunk = 0; io_chunk < num_chunks; ++io_chunk) {
    std::vector<char*> op_addresses{ops_per_chunk};

    for (size_t io_op = 0; io_op < ops_per_chunk; ++io_op) {
      switch (config.exec_mode) {
        case internal::Mode::Random: {
          uint64_t random_value;
          if (config.random_distribution == internal::RandomDistribution::Uniform) {
            random_value = access_distribution(rnd_generator);
          } else {
            random_value = zipf(config.zipf_alpha, num_accesses_in_range);
          }
          op_addresses[io_op] = thread_config.partition_start_addr + (random_value * config.access_size);
          is_read = !is_write_only && io_mode_distribution(rnd_generator);
          break;
        }
        case internal::Mode::Sequential: {
          op_addresses[io_op] = next_op_position;
          next_op_position += ops_per_iteration;
          break;
        }
        case internal::Mode::Sequential_Desc: {
          op_addresses[io_op] = next_op_position;
          next_op_position -= ops_per_iteration;
          break;
        }
      }
    }

    IoOperation operation =
        is_read ? IoOperation::ReadOp(std::move(op_addresses), config.access_size, config.data_instruction)
                : IoOperation::WriteOp(std::move(op_addresses), config.access_size, config.data_instruction,
                                       config.persist_instruction);

    const auto start_ts = std::chrono::high_resolution_clock::now();
    operation.run();
    const auto end_ts = std::chrono::high_resolution_clock::now();

    if (has_pause && ++current_pause_frequency_count >= config.pause_frequency &&
        io_chunk < thread_config.num_ops - 1) {
      IoOperation::PauseOp(config.pause_length_micros).run();
      current_pause_frequency_count = 0;
    }

    internal::Latency latency{static_cast<uint64_t>((end_ts - start_ts).count()), operation.op_type_};
    if (config.raw_results) {
      thread_config.raw_measurements->emplace_back(start_ts, latency);
    } else {
      thread_config.latencies->emplace_back(latency);
    }
  }
}

nlohmann::json get_benchmark_config_as_json(const BenchmarkConfig& bm_config) {
  nlohmann::json config;
  config["total_memory_range"] = bm_config.total_memory_range;
  config["access_size"] = bm_config.access_size;
  config["exec_mode"] = get_enum_as_string(ConfigEnums::str_to_mode, bm_config.exec_mode);
  config["write_ratio"] = bm_config.write_ratio;
  config["pause_frequency"] = bm_config.pause_frequency;
  config["number_partitions"] = bm_config.number_partitions;
  config["number_threads"] = bm_config.number_threads;
  config["data_instruction"] = get_enum_as_string(ConfigEnums::str_to_data_instruction, bm_config.data_instruction);

  if (bm_config.pause_frequency > 0) {
    config["pause_length_micros"] = bm_config.pause_length_micros;
  }

  if (bm_config.write_ratio > 0) {
    config["persist_instruction"] =
        get_enum_as_string(ConfigEnums::str_to_persist_instruction, bm_config.persist_instruction);
  }

  if (bm_config.exec_mode == internal::Mode::Random) {
    config["number_operations"] = bm_config.number_operations;
    config["random_distribution"] =
        get_enum_as_string(ConfigEnums::str_to_random_distribution, bm_config.random_distribution);
    if (bm_config.random_distribution == internal::Zipf) {
      config["zipf_alpha"] = bm_config.zipf_alpha;
    }
  }

  config["prefault_file"] = bm_config.prefault_file;

  return config;
}
}  // namespace perma
