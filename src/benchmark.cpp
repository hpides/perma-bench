#include "benchmark.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <thread>
#include <utility>

#include "fast_random.hpp"
#include "numa.hpp"

namespace {

#define CHECK_ARGUMENT(exp, txt) \
  if (!(exp)) {                  \
    spdlog::critical(txt);       \
    perma::crash_exit();         \
  }

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
bool get_size_if_present(YAML::Node& data, const std::string& name, const std::unordered_map<char, uint64_t>& enum_map,
                         T* attribute) {
  YAML::Node entry = data[name];
  if (!entry) {
    return false;
  }
  ensure_unique_key(entry, name);

  const auto size_string = entry.as<std::string>();
  const char size_suffix = size_string.back();
  size_t size_end = size_string.length();
  uint64_t factor = 1;

  auto it = enum_map.find(size_suffix);
  if (it != enum_map.end()) {
    factor = it->second;
    size_end -= 1;
  } else if (isalpha(size_suffix)) {
    throw std::invalid_argument(std::string("Unknown size suffix: ") + size_suffix);
  }

  char* end;
  const std::string size_number = size_string.substr(0, size_end);
  const uint64_t size = std::strtoull(size_number.data(), &end, 10);
  *attribute = size * factor;

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

struct BenchmarkEnums {
  static const std::unordered_map<std::string, internal::BenchmarkType> str_to_benchmark_type;
};

const std::unordered_map<std::string, internal::BenchmarkType> BenchmarkEnums::str_to_benchmark_type{
    {"single", internal::BenchmarkType::Single}, {"parallel", internal::BenchmarkType::Parallel}};

struct ConfigEnums {
  static const std::unordered_map<std::string, bool> str_to_mem_type;
  static const std::unordered_map<std::string, internal::Mode> str_to_mode;
  static const std::unordered_map<std::string, internal::NumaPattern> str_to_numa_pattern;
  static const std::unordered_map<std::string, internal::PersistInstruction> str_to_persist_instruction;
  static const std::unordered_map<std::string, internal::RandomDistribution> str_to_random_distribution;

  // Map to convert a K/M/G suffix to the correct kibi, mebi-, gibibyte value.
  static const std::unordered_map<char, uint64_t> scale_suffix_to_factor;
};

const std::unordered_map<std::string, bool> ConfigEnums::str_to_mem_type{{"pmem", true}, {"dram", false}};

const std::unordered_map<std::string, internal::Mode> ConfigEnums::str_to_mode{
    {"sequential", internal::Mode::Sequential},
    {"sequential_asc", internal::Mode::Sequential},
    {"sequential_desc", internal::Mode::Sequential_Desc},
    {"random", internal::Mode::Random},
    {"custom", internal::Mode::Custom}};

const std::unordered_map<std::string, internal::NumaPattern> ConfigEnums::str_to_numa_pattern{
    {"near", internal::NumaPattern::Near}, {"far", internal::NumaPattern::Far}};

const std::unordered_map<std::string, internal::PersistInstruction> ConfigEnums::str_to_persist_instruction{
    {"nocache", internal::PersistInstruction::NoCache},
    {"cache", internal::PersistInstruction::Cache},
    {"cache_inv", internal::PersistInstruction::CacheInvalidate},
    {"none", internal::PersistInstruction::None}};

const std::unordered_map<std::string, internal::RandomDistribution> ConfigEnums::str_to_random_distribution{
    {"uniform", internal::RandomDistribution::Uniform}, {"zipf", internal::RandomDistribution::Zipf}};

const std::unordered_map<char, uint64_t> ConfigEnums::scale_suffix_to_factor{{'k', 1024},
                                                                             {'K', 1024},
                                                                             {'m', 1024 * 1024},
                                                                             {'M', 1024 * 1024},
                                                                             {'g', 1024 * 1024 * 1024},
                                                                             {'G', 1024 * 1024 * 1024}};

const std::string& Benchmark::benchmark_name() const { return benchmark_name_; }

std::string Benchmark::benchmark_type_as_str() const {
  return get_enum_as_string(BenchmarkEnums::str_to_benchmark_type, benchmark_type_);
}

internal::BenchmarkType Benchmark::get_benchmark_type() const { return benchmark_type_; }

void Benchmark::single_set_up(const BenchmarkConfig& config, char* pmem_data, std::unique_ptr<BenchmarkResult>& result,
                              std::vector<std::thread>& pool, std::vector<ThreadRunConfig>& thread_config) {
  const size_t num_total_range_ops = config.total_memory_range / config.access_size;
  const size_t num_operations =
      (config.exec_mode == internal::Mode::Random || config.exec_mode == internal::Mode::Custom)
          ? config.number_operations
          : num_total_range_ops;
  const size_t num_ops_per_thread = num_operations / config.number_threads;

  pool.reserve(config.number_threads);
  thread_config.reserve(config.number_threads);
  result->latencies.resize(config.number_threads);

  if (config.exec_mode == internal::Mode::Custom) {
    result->custom_operation_durations.resize(config.number_threads);
  }

  const uint16_t num_threads_per_partition = config.number_threads / config.number_partitions;
  const uint64_t partition_size = config.total_memory_range / config.number_partitions;

  for (uint16_t partition_num = 0; partition_num < config.number_partitions; partition_num++) {
    char* partition_start =
        (config.exec_mode == internal::Mode::Sequential_Desc)
            ? pmem_data + ((config.number_partitions - partition_num) * partition_size) - config.access_size
            : partition_start = pmem_data + (partition_num * partition_size);

    for (uint16_t thread_num = 0; thread_num < num_threads_per_partition; thread_num++) {
      const uint32_t index = thread_num + (partition_num * num_threads_per_partition);
      result->latencies[index].reserve(num_ops_per_thread);
      thread_config.emplace_back(partition_start, partition_size, num_threads_per_partition, thread_num,
                                 num_ops_per_thread, &result->latencies[index],
                                 &result->custom_operation_durations[index], config);
    }
  }
}

char* Benchmark::create_single_data_file(const BenchmarkConfig& config, std::filesystem::path& data_file) {
  if (std::filesystem::exists(data_file)) {
    // Data was already generated. Only re-map it.
    return map_file(data_file, !config.is_pmem, config.total_memory_range);
  }

  char* pmem_data = create_file(data_file, !config.is_pmem, config.total_memory_range);
  if (config.write_ratio < 1) {
    // If we read data in this benchmark, we need to generate it first.
    generate_read_data(pmem_data, config.total_memory_range);
  }
  if (config.write_ratio == 1 && config.prefault_file) {
    const size_t page_size = config.is_pmem ? internal::PMEM_PAGE_SIZE : internal::DRAM_PAGE_SIZE;
    prefault_file(pmem_data, config.total_memory_range, page_size);
  }
  return pmem_data;
}

void Benchmark::run_custom_ops_in_thread(const ThreadRunConfig& thread_config, const BenchmarkConfig& config) {
  const std::vector<CustomOp>& operations = config.custom_operations;
  const size_t num_ops = operations.size();

  std::vector<ChainedOperation> operation_chain;
  operation_chain.reserve(num_ops);

  // Determine maximum access size to ensure that operations don't write beyond the end of the range.
  size_t max_access_size = 0;
  for (const CustomOp& op : operations) {
    max_access_size = std::max(op.size, max_access_size);
  }
  const size_t aligned_range_size = thread_config.partition_size - max_access_size;

  for (size_t i = 0; i < num_ops; ++i) {
    const CustomOp& op = operations[i];
    operation_chain.emplace_back(op, thread_config.partition_start_addr, aligned_range_size);
    if (i > 0) {
      operation_chain[i - 1].set_next(&operation_chain[i]);
    }
  }

  const size_t seed = std::chrono::steady_clock::now().time_since_epoch().count() * (thread_config.thread_num + 1);
  lehmer64_seed(seed);
  char* start_addr = (char*)seed;

  ChainedOperation& start_op = operation_chain[0];
  auto start_ts = std::chrono::steady_clock::now();

  if (config.latency_sample_frequency == 0) {
    // We don't want the sampling code overhead if we don't want to sample the latency
    for (size_t iteration = 0; iteration < thread_config.num_ops; ++iteration) {
      start_op.run(start_addr, start_addr);
    }
  } else {
    // Latency sampling requested, measure the latency every x iterations.
    const uint64_t freq = config.latency_sample_frequency;
    // Start at 1 to avoid measuring latency of first request.
    for (size_t iteration = 1; iteration <= thread_config.num_ops; ++iteration) {
      if (iteration % freq == 0) {
        auto op_start = std::chrono::steady_clock::now();
        start_op.run(start_addr, start_addr);
        auto op_end = std::chrono::steady_clock::now();
        thread_config.latencies->emplace_back(static_cast<uint64_t>((op_end - op_start).count()),
                                              internal::OpType::Custom);
      } else {
        start_op.run(start_addr, start_addr);
      }
    }
  }

  auto end_ts = std::chrono::steady_clock::now();
  auto duration = (end_ts - start_ts).count();
  *thread_config.custom_op_duration = duration;
}

void Benchmark::run_in_thread(const ThreadRunConfig& thread_config, const BenchmarkConfig& config) {
  if (config.numa_pattern == internal::NumaPattern::Far) {
    set_to_far_cpus();
  }

  if (config.exec_mode == internal::Mode::Custom) {
    return run_custom_ops_in_thread(thread_config, config);
  }

  const size_t ops_per_iteration = thread_config.num_threads_per_partition * config.access_size;
  const uint32_t num_accesses_in_range = thread_config.partition_size / config.access_size;
  const bool is_read_only = config.write_ratio == 0;
  const bool is_write_only = config.write_ratio == 1;
  const bool has_pause = config.pause_frequency > 0;
  size_t current_pause_frequency_count = 0;
  bool is_read = is_read_only;
  assert(is_write_only || is_read_only || config.exec_mode == internal::Mode::Random ||
         config.exec_mode == internal::Mode::Custom);

  const size_t thread_partition_offset = thread_config.thread_num * config.access_size;

  std::random_device rnd_device;
  std::mt19937_64 rnd_generator{rnd_device()};
  std::bernoulli_distribution io_mode_distribution(1 - config.write_ratio);
  std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range - 1);

  const size_t ops_per_chunk =
      config.access_size < config.min_io_chunk_size ? config.min_io_chunk_size / config.access_size : 1;
  const size_t num_chunks = thread_config.num_ops / ops_per_chunk;

  std::vector<char*> op_addresses{ops_per_chunk};
  const auto begin_ts = std::chrono::steady_clock::now();
  bool is_time_finished = false;

  while (!is_time_finished) {
    char* next_op_position = config.exec_mode == internal::Mode::Sequential_Desc
                                 ? thread_config.partition_start_addr - thread_partition_offset
                                 : thread_config.partition_start_addr + thread_partition_offset;

    if (config.run_time == -1) {
      // Only do one for loop iteration
      is_time_finished = true;
    }

    for (uint32_t io_chunk = 0; io_chunk < num_chunks; ++io_chunk) {
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

      IoOperation operation = is_read
                                  ? IoOperation::ReadOp(op_addresses, config.access_size)
                                  : IoOperation::WriteOp(op_addresses, config.access_size, config.persist_instruction);

      const auto start_ts = std::chrono::steady_clock::now();
      operation.run();
      const auto end_ts = std::chrono::steady_clock::now();

      if (has_pause && ++current_pause_frequency_count >= config.pause_frequency &&
          io_chunk < thread_config.num_ops - 1) {
        IoOperation::PauseOp(config.pause_length_micros).run();
        current_pause_frequency_count = 0;
      }

      internal::Latency latency{static_cast<uint64_t>((end_ts - start_ts).count()), operation.op_type_};
      thread_config.latencies->emplace_back(latency);

      const auto run_time_in_sec = std::chrono::duration_cast<std::chrono::seconds>(end_ts - begin_ts).count();
      if (!is_time_finished && run_time_in_sec >= config.run_time) {
        is_time_finished = true;
        break;
      }
    }
  }
}

nlohmann::json Benchmark::get_benchmark_config_as_json(const BenchmarkConfig& bm_config) {
  nlohmann::json config;
  config["memory_type"] = get_enum_as_string(ConfigEnums::str_to_mem_type, bm_config.is_pmem);
  config["total_memory_range"] = bm_config.total_memory_range;
  config["exec_mode"] = get_enum_as_string(ConfigEnums::str_to_mode, bm_config.exec_mode);
  config["number_partitions"] = bm_config.number_partitions;
  config["number_threads"] = bm_config.number_threads;
  config["numa_pattern"] = get_enum_as_string(ConfigEnums::str_to_numa_pattern, bm_config.numa_pattern);
  config["prefault_file"] = bm_config.prefault_file;

  if (bm_config.exec_mode != internal::Mode::Custom) {
    config["access_size"] = bm_config.access_size;
    config["write_ratio"] = bm_config.write_ratio;
    config["pause_frequency"] = bm_config.pause_frequency;
    config["min_io_chunk_size"] = bm_config.min_io_chunk_size;

    if (bm_config.pause_frequency > 0) {
      config["pause_length_micros"] = bm_config.pause_length_micros;
    }

    if (bm_config.write_ratio > 0) {
      config["persist_instruction"] =
          get_enum_as_string(ConfigEnums::str_to_persist_instruction, bm_config.persist_instruction);
    }
  }

  if (bm_config.exec_mode == internal::Mode::Random) {
    config["number_operations"] = bm_config.number_operations;
    config["random_distribution"] =
        get_enum_as_string(ConfigEnums::str_to_random_distribution, bm_config.random_distribution);
    if (bm_config.random_distribution == internal::RandomDistribution::Zipf) {
      config["zipf_alpha"] = bm_config.zipf_alpha;
    }
  }

  if (bm_config.exec_mode == internal::Mode::Custom) {
    config["number_operations"] = bm_config.number_operations;
    config["custom_operations"] = CustomOp::all_to_string(bm_config.custom_operations);
  }

  if (bm_config.run_time != -1) {
    config["run_time"] = bm_config.run_time;
  }

  return config;
}

const std::vector<BenchmarkConfig>& Benchmark::get_benchmark_configs() const { return configs_; }

const std::vector<std::filesystem::path>& Benchmark::get_pmem_files() const { return pmem_files_; }

std::vector<char*> Benchmark::get_pmem_data() const { return pmem_data_; }

const std::vector<std::vector<ThreadRunConfig>>& Benchmark::get_thread_configs() const { return thread_configs_; }

const std::vector<std::unique_ptr<BenchmarkResult>>& Benchmark::get_benchmark_results() const { return results_; }

std::vector<bool> Benchmark::owns_pmem_files() const { return owns_pmem_files_; }
nlohmann::json Benchmark::get_json_config(uint8_t config_index) {
  return get_benchmark_config_as_json(configs_[config_index]);
}

BenchmarkResult::BenchmarkResult(BenchmarkConfig config) : config{std::move(config)}, latency_hdr{nullptr} {
  // Initialize HdrHistrogram
  // 100 seconds in nanoseconds as max value.
  hdr_init(1, 100000000000, 3, &latency_hdr);
}

BenchmarkResult::~BenchmarkResult() {
  if (latency_hdr != nullptr) {
    hdr_close(latency_hdr);
  }

  latencies.clear();
  latencies.shrink_to_fit();
}

nlohmann::json BenchmarkResult::get_result_as_json() const {
  if (config.exec_mode == internal::Mode::Custom) {
    return get_custom_results_as_json();
  }

  if (latencies.empty()) {
    spdlog::info("No result data collected!");
    crash_exit();
  }

  nlohmann::json result;
  nlohmann::json result_points = nlohmann::json::array();

  uint64_t total_read_size = 0;
  uint64_t total_write_size = 0;
  uint64_t total_read_duration = 0;
  uint64_t total_write_duration = 0;

  const size_t num_ops = latencies[0].size();
  const uint64_t data_size = config.access_size;
  const uint64_t num_ops_per_chunk = std::max(config.min_io_chunk_size / data_size, 1ul);
  const uint64_t chunk_size = std::max(config.min_io_chunk_size, data_size);

  for (uint16_t thread_num = 0; thread_num < config.number_threads; thread_num++) {
    const std::vector<internal::Latency>& thread_latencies = latencies[thread_num];

    for (size_t io_chunk = 0; io_chunk < num_ops; ++io_chunk) {
      internal::Latency latency = thread_latencies[io_chunk];
      const uint64_t duration = latency.duration;

      const internal::OpType op_type = latency.op_type;
      if (op_type == internal::OpType::Read) {
        total_read_size += chunk_size;
        total_read_duration += duration;
      } else if (op_type == internal::OpType::Write) {
        total_write_size += chunk_size;
        total_write_duration += duration;
      } else {
        throw std::runtime_error{"Unknown IO operation in results"};
      }
    }
  }

  nlohmann::json bandwidth_results;
  if (total_read_duration > 0) {
    // TODO(#165): Remove this at some point
    const uint64_t read_execution_time = total_read_duration / config.number_threads;
    const double bandwidth = get_bandwidth(total_read_size, read_execution_time);
    // We need to normalize the bandwidth here if we do not execute the same operation type 100% of the time.
    // Otherwise, we strongly overestimate the bandwidth, as the operations are running only X% of the time but we
    // logically scale it up to 100%. This will result in a slight underestimation, but not by much.
    // This is a no-op if we run one op-type only.
    const double normalized_bandwidth = bandwidth * (1 - config.write_ratio);
    bandwidth_results["read"] = normalized_bandwidth;
  }
  if (total_write_duration > 0) {
    // TODO(#165): Remove this at some point
    const uint64_t write_execution_time = total_write_duration / config.number_threads;
    const double bandwidth = get_bandwidth(total_write_size, write_execution_time);
    // We need to normalize the bandwidth here if we do not execute the same operation type 100% of the time.
    // Otherwise, we strongly overestimate the bandwidth, as the operations are running only X% of the time but we
    // logically scale it up to 100%. This will result in a slight underestimation, but not by much.
    // This is a no-op if we run one op-type only.
    const double normalized_bandwidth = bandwidth * config.write_ratio;
    bandwidth_results["write"] = normalized_bandwidth;
  }

  result["bandwidth"] = bandwidth_results;
  return result;
}

nlohmann::json BenchmarkResult::get_custom_results_as_json() const {
  nlohmann::json result;

  uint64_t total_duration = 0;
  for (uint64_t thread_id = 0; thread_id < config.number_threads; ++thread_id) {
    total_duration += custom_operation_durations[thread_id];
  }
  const double avg_duration_ns = (double)total_duration / config.number_threads;
  const double avg_duration = avg_duration_ns / internal::NANOSECONDS_IN_SECONDS;
  const double ops_per_sec = config.number_operations / avg_duration;

  result["avg_duration_sec"] = avg_duration;
  result["ops_per_second"] = ops_per_sec;

  if (config.latency_sample_frequency != 0) {
    for (const std::vector<internal::Latency>& thread_latencies : latencies) {
      for (const internal::Latency& latency : thread_latencies) {
        hdr_record_value(latency_hdr, latency.duration);
      }
    }
    result["duration"] = hdr_histogram_to_json(latency_hdr);
  }

  return result;
}

BenchmarkConfig BenchmarkConfig::decode(YAML::Node& node) {
  BenchmarkConfig bm_config{};
  size_t num_found = 0;
  try {
    num_found += get_size_if_present(node, "total_memory_range", ConfigEnums::scale_suffix_to_factor,
                                     &bm_config.total_memory_range);
    num_found += get_size_if_present(node, "access_size", ConfigEnums::scale_suffix_to_factor, &bm_config.access_size);

    num_found += get_if_present(node, "number_operations", &bm_config.number_operations);
    num_found += get_if_present(node, "run_time", &bm_config.run_time);
    num_found += get_if_present(node, "write_ratio", &bm_config.write_ratio);
    num_found += get_if_present(node, "pause_frequency", &bm_config.pause_frequency);
    num_found += get_if_present(node, "pause_length_micros", &bm_config.pause_length_micros);
    num_found += get_if_present(node, "number_partitions", &bm_config.number_partitions);
    num_found += get_if_present(node, "number_threads", &bm_config.number_threads);
    num_found += get_if_present(node, "zipf_alpha", &bm_config.zipf_alpha);
    num_found += get_if_present(node, "prefault_file", &bm_config.prefault_file);
    num_found += get_if_present(node, "min_io_chunk_size", &bm_config.min_io_chunk_size);
    num_found += get_if_present(node, "latency_sample_frequency", &bm_config.latency_sample_frequency);

    num_found += get_enum_if_present(node, "exec_mode", ConfigEnums::str_to_mode, &bm_config.exec_mode);
    num_found += get_enum_if_present(node, "numa_pattern", ConfigEnums::str_to_numa_pattern, &bm_config.numa_pattern);
    num_found += get_enum_if_present(node, "random_distribution", ConfigEnums::str_to_random_distribution,
                                     &bm_config.random_distribution);
    num_found += get_enum_if_present(node, "persist_instruction", ConfigEnums::str_to_persist_instruction,
                                     &bm_config.persist_instruction);

    std::string custom_ops;
    const bool has_custom_ops = get_if_present(node, "custom_operations", &custom_ops);
    if (has_custom_ops) {
      bm_config.custom_operations = CustomOp::all_from_string(custom_ops);
      num_found++;
    }

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
  CHECK_ARGUMENT(is_memory_range_multiple_of_access_size, "Total memory range must be a multiple of access size.");

  // Check if ratio is between one and zero
  const bool is_ratio_between_one_zero = 0 <= write_ratio && write_ratio <= 1;
  CHECK_ARGUMENT(is_ratio_between_one_zero, "Write ratio must be between 0 and 1");

  // Check if runtime is at least one second
  const bool is_at_least_one_second_or_default = run_time > 0 || run_time == -1;
  CHECK_ARGUMENT(is_at_least_one_second_or_default, "Run time be at least 1 second");

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

  // Assumption: total memory range must be evenly divisible into number of partitions
  const bool is_partitionable = ((total_memory_range / number_partitions) % access_size) == 0;
  CHECK_ARGUMENT(is_partitionable,
                 "Total memory range must be evenly divisible into number of partitions. "
                 "Most likely you can fix this by using 2^x partitions.");

  // Assumption: number_operations should only be set for random/custom access. It is ignored in sequential IO.
  const bool is_number_operations_set_random =
      number_operations == BenchmarkConfig{}.number_operations ||
      (exec_mode == internal::Mode::Random || exec_mode == internal::Mode::Custom);
  CHECK_ARGUMENT(is_number_operations_set_random, "Number of operations should only be set for random/custom access");

  // Assumption: sequential access does not make sense if we mix reads and writes
  const bool is_mixed_workload_random = (write_ratio == 1 || write_ratio == 0) || exec_mode == internal::Mode::Random;
  CHECK_ARGUMENT(is_mixed_workload_random, "Mixed read/write workloads only supported for random execution.");

  // Assumption: min_io_chunk size must be a power of two
  const bool is_valid_min_io_chunk_size = min_io_chunk_size >= 64 && (min_io_chunk_size & (min_io_chunk_size - 1)) == 0;
  CHECK_ARGUMENT(is_valid_min_io_chunk_size, "Minimum IO chunk must be >= 64 Byte and a power of two");

  // Assumption: total memory needs to fit into N chunks exactly
  const bool is_seq_total_memory_chunkable =
      (exec_mode == internal::Mode::Random || exec_mode == internal::Mode::Custom) ||
      (total_memory_range % min_io_chunk_size) == 0;
  CHECK_ARGUMENT(is_seq_total_memory_chunkable,
                 "Total file size needs to be multiple of " + std::to_string(min_io_chunk_size));

  // Assumption: we chunk operations and we need enough data to fill at least one chunk
  const bool is_total_memory_large_enough = (total_memory_range / number_threads) >= min_io_chunk_size;
  CHECK_ARGUMENT(is_total_memory_large_enough,
                 "Each thread needs at least " + std::to_string(min_io_chunk_size) + " memory.");

  const bool is_pause_freq_chunkable = pause_frequency == 0 || pause_frequency >= (min_io_chunk_size / access_size);
  CHECK_ARGUMENT(is_pause_freq_chunkable, "Cannot insert pauses with single chunk of " +
                                              std::to_string(min_io_chunk_size / access_size) + " ops (" +
                                              std::to_string(min_io_chunk_size) + " Byte / " +
                                              std::to_string(access_size) + " Byte) in this configuration.");

  const bool is_far_numa_node_available = numa_pattern == internal::NumaPattern::Near || has_far_numa_nodes();
  CHECK_ARGUMENT(is_far_numa_node_available, "Cannot run far NUMA node benchmark without far NUMA nodes.");

  const bool has_custom_ops = exec_mode != internal::Mode::Custom || !custom_operations.empty();
  CHECK_ARGUMENT(has_custom_ops, "Must specify custom_operations for custom execution.");

  const bool has_no_custom_ops = exec_mode == internal::Mode::Custom || custom_operations.empty();
  CHECK_ARGUMENT(has_no_custom_ops, "Cannot specify custom_operations for non-custom execution.");

  const bool latency_sample_is_custom = exec_mode == internal::Mode::Custom || latency_sample_frequency == 0;
  CHECK_ARGUMENT(latency_sample_is_custom, "Latency sampling can only be used with custom operations.");
}

}  // namespace perma
