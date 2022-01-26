#include "benchmark.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <thread>
#include <utility>

#include "benchmark_config.hpp"
#include "fast_random.hpp"
#include "numa.hpp"

namespace {

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
  const double duration_in_s = total_duration / static_cast<double>(perma::NANOSECONDS_IN_SECONDS);
  const double data_in_gib = total_data_size / static_cast<double>(perma::BYTE_IN_GIGABYTE);
  return data_in_gib / duration_in_s;
}

}  // namespace

namespace perma {

const std::string& Benchmark::benchmark_name() const { return benchmark_name_; }

std::string Benchmark::benchmark_type_as_str() const {
  return utils::get_enum_as_string(BenchmarkEnums::str_to_benchmark_type, benchmark_type_);
}

BenchmarkType Benchmark::get_benchmark_type() const { return benchmark_type_; }

void Benchmark::single_set_up(const BenchmarkConfig& config, char* pmem_data, std::unique_ptr<BenchmarkResult>& result,
                              std::vector<std::thread>& pool, std::vector<ThreadRunConfig>& thread_config) {
  const size_t num_total_range_ops = config.total_memory_range / config.access_size;
  const size_t num_operations = (config.exec_mode == Mode::Random || config.exec_mode == Mode::Custom)
                                    ? config.number_operations
                                    : num_total_range_ops;
  const size_t num_ops_per_thread = num_operations / config.number_threads;

  pool.reserve(config.number_threads);
  thread_config.reserve(config.number_threads);
  result->latencies.resize(config.number_threads);

  if (config.exec_mode == Mode::Custom) {
    result->custom_operation_durations.resize(config.number_threads);
  }

  const uint16_t num_threads_per_partition = config.number_threads / config.number_partitions;
  const uint64_t partition_size = config.total_memory_range / config.number_partitions;

  for (uint16_t partition_num = 0; partition_num < config.number_partitions; partition_num++) {
    char* partition_start =
        (config.exec_mode == Mode::Sequential_Desc)
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
    return utils::map_file(data_file, !config.is_pmem, config.total_memory_range);
  }

  char* pmem_data = utils::create_file(data_file, !config.is_pmem, config.total_memory_range);
  if (config.contains_read_op()) {
    // If we read data in this benchmark, we need to generate it first.
    utils::generate_read_data(pmem_data, config.total_memory_range);
  }

  if (config.contains_write_op() && config.prefault_file) {
    const size_t page_size = config.is_pmem ? utils::PMEM_PAGE_SIZE : utils::DRAM_PAGE_SIZE;
    utils::prefault_file(pmem_data, config.total_memory_range, page_size);
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
        // Use Operation::Read here as default. This value is ignored for custom operations anyway.
        thread_config.latencies->emplace_back(static_cast<uint64_t>((op_end - op_start).count()), Operation::Read);
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
  if (config.numa_pattern == NumaPattern::Far) {
    set_to_far_cpus();
  }

  if (config.exec_mode == Mode::Custom) {
    return run_custom_ops_in_thread(thread_config, config);
  }

  const size_t ops_per_iteration = thread_config.num_threads_per_partition * config.access_size;
  const uint32_t num_accesses_in_range = thread_config.partition_size / config.access_size;
  const bool is_read_op = config.operation == Operation::Read;
  const size_t thread_partition_offset = thread_config.thread_num * config.access_size;

  std::random_device rnd_device;
  std::mt19937_64 rnd_generator{rnd_device()};
  std::uniform_int_distribution<int> access_distribution(0, num_accesses_in_range - 1);

  const size_t ops_per_chunk =
      config.access_size < config.min_io_chunk_size ? config.min_io_chunk_size / config.access_size : 1;
  const size_t num_chunks = thread_config.num_ops / ops_per_chunk;

  std::vector<char*> op_addresses{ops_per_chunk};
  const auto begin_ts = std::chrono::steady_clock::now();
  bool is_time_finished = false;

  while (!is_time_finished) {
    char* next_op_position = config.exec_mode == Mode::Sequential_Desc
                                 ? thread_config.partition_start_addr - thread_partition_offset
                                 : thread_config.partition_start_addr + thread_partition_offset;

    if (config.run_time == -1) {
      // Only do one for loop iteration
      is_time_finished = true;
    }

    for (uint32_t io_chunk = 0; io_chunk < num_chunks; ++io_chunk) {
      for (size_t io_op = 0; io_op < ops_per_chunk; ++io_op) {
        switch (config.exec_mode) {
          case Mode::Random: {
            uint64_t random_value;
            if (config.random_distribution == RandomDistribution::Uniform) {
              random_value = access_distribution(rnd_generator);
            } else {
              random_value = utils::zipf(config.zipf_alpha, num_accesses_in_range);
            }
            op_addresses[io_op] = thread_config.partition_start_addr + (random_value * config.access_size);
            break;
          }
          case Mode::Sequential: {
            op_addresses[io_op] = next_op_position;
            next_op_position += ops_per_iteration;
            break;
          }
          case Mode::Sequential_Desc: {
            op_addresses[io_op] = next_op_position;
            next_op_position -= ops_per_iteration;
            break;
          }
          default: {
            spdlog::error("Illegal state. Cannot be in `run_in_thread()` with different mode: {}.", config.exec_mode);
            utils::crash_exit();
          }
        }
      }

      IoOperation operation = is_read_op
                                  ? IoOperation::ReadOp(op_addresses, config.access_size)
                                  : IoOperation::WriteOp(op_addresses, config.access_size, config.persist_instruction);

      const auto start_ts = std::chrono::steady_clock::now();
      operation.run();
      const auto end_ts = std::chrono::steady_clock::now();

      Latency latency{static_cast<uint64_t>((end_ts - start_ts).count()), operation.op_type_};
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
  config["memory_type"] = utils::get_enum_as_string(ConfigEnums::str_to_mem_type, bm_config.is_pmem);
  config["total_memory_range"] = bm_config.total_memory_range;
  config["exec_mode"] = utils::get_enum_as_string(ConfigEnums::str_to_mode, bm_config.exec_mode);
  config["number_partitions"] = bm_config.number_partitions;
  config["number_threads"] = bm_config.number_threads;
  config["numa_pattern"] = utils::get_enum_as_string(ConfigEnums::str_to_numa_pattern, bm_config.numa_pattern);
  config["prefault_file"] = bm_config.prefault_file;

  if (bm_config.exec_mode != Mode::Custom) {
    config["access_size"] = bm_config.access_size;
    config["operation"] = utils::get_enum_as_string(ConfigEnums::str_to_operation, bm_config.operation);
    config["min_io_chunk_size"] = bm_config.min_io_chunk_size;

    if (bm_config.operation == Operation::Write) {
      config["persist_instruction"] =
          utils::get_enum_as_string(ConfigEnums::str_to_persist_instruction, bm_config.persist_instruction);
    }
  }

  if (bm_config.exec_mode == Mode::Random) {
    config["number_operations"] = bm_config.number_operations;
    config["random_distribution"] =
        utils::get_enum_as_string(ConfigEnums::str_to_random_distribution, bm_config.random_distribution);
    if (bm_config.random_distribution == RandomDistribution::Zipf) {
      config["zipf_alpha"] = bm_config.zipf_alpha;
    }
  }

  if (bm_config.exec_mode == Mode::Custom) {
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

const std::unordered_map<std::string, BenchmarkType> BenchmarkEnums::str_to_benchmark_type{
    {"single", BenchmarkType::Single}, {"parallel", BenchmarkType::Parallel}};

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
  if (config.exec_mode == Mode::Custom) {
    return get_custom_results_as_json();
  }

  if (latencies.empty()) {
    spdlog::info("No result data collected!");
    utils::crash_exit();
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
    const std::vector<Latency>& thread_latencies = latencies[thread_num];

    for (size_t io_chunk = 0; io_chunk < num_ops; ++io_chunk) {
      Latency latency = thread_latencies[io_chunk];
      const uint64_t duration = latency.duration;

      const Operation op_type = latency.op_type;
      if (op_type == Operation::Read) {
        total_read_size += chunk_size;
        total_read_duration += duration;
      } else if (op_type == Operation::Write) {
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
    bandwidth_results["read"] = bandwidth;
  }
  if (total_write_duration > 0) {
    // TODO(#165): Remove this at some point
    const uint64_t write_execution_time = total_write_duration / config.number_threads;
    const double bandwidth = get_bandwidth(total_write_size, write_execution_time);
    bandwidth_results["write"] = bandwidth;
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
  const double avg_duration = avg_duration_ns / NANOSECONDS_IN_SECONDS;
  const double ops_per_sec = config.number_operations / avg_duration;

  result["avg_duration_sec"] = avg_duration;
  result["ops_per_second"] = ops_per_sec;

  if (config.latency_sample_frequency != 0) {
    for (const std::vector<Latency>& thread_latencies : latencies) {
      for (const Latency& latency : thread_latencies) {
        hdr_record_value(latency_hdr, latency.duration);
      }
    }
    result["duration"] = hdr_histogram_to_json(latency_hdr);
  }

  return result;
}

}  // namespace perma
