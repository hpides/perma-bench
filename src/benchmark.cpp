#include "benchmark.hpp"

#include <spdlog/spdlog.h>

#include <condition_variable>
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

inline double get_bandwidth(const uint64_t total_data_size, const std::chrono::steady_clock::duration total_duration) {
  const double duration_in_s = static_cast<double>(total_duration.count()) / perma::NANOSECONDS_IN_SECONDS;
  const double data_in_gib = static_cast<double>(total_data_size) / perma::BYTES_IN_GIGABYTE;
  return data_in_gib / duration_in_s;
}

}  // namespace

namespace perma {

const std::string& Benchmark::benchmark_name() const { return benchmark_name_; }

std::string Benchmark::benchmark_type_as_str() const {
  return utils::get_enum_as_string(BenchmarkEnums::str_to_benchmark_type, benchmark_type_);
}

BenchmarkType Benchmark::get_benchmark_type() const { return benchmark_type_; }

void Benchmark::single_set_up(const BenchmarkConfig& config, char* pmem_data, char* dram_data,
                              BenchmarkExecution* execution, BenchmarkResult* result, std::vector<std::thread>* pool,
                              std::vector<ThreadRunConfig>* thread_config) {
  const size_t num_total_range_ops = config.memory_range / config.access_size;
  const bool is_custom_execution = config.exec_mode == Mode::Custom;
  const size_t num_operations =
      (config.exec_mode == Mode::Random || is_custom_execution) ? config.number_operations : num_total_range_ops;
  const size_t num_ops_per_thread = num_operations / config.number_threads;

  pool->reserve(config.number_threads);
  thread_config->reserve(config.number_threads);
  result->total_operation_durations.resize(config.number_threads);
  result->total_operation_sizes.resize(config.number_threads, 0);

  uint64_t estimate_num_latency_measurements = 0;
  if (is_custom_execution) {
    result->custom_operation_latencies.resize(config.number_threads);
    result->custom_operation_durations.resize(config.number_threads, 0);

    if (config.latency_sample_frequency > 0) {
      estimate_num_latency_measurements = (num_ops_per_thread / config.latency_sample_frequency) * 2;
    }
  }

  // If number_partitions is 0, each thread gets its own partition.
  const uint16_t num_partitions = config.number_partitions == 0 ? config.number_threads : config.number_partitions;

  const uint16_t num_threads_per_partition = config.number_threads / num_partitions;
  const uint64_t partition_size = config.memory_range / num_partitions;
  const uint64_t dram_partition_size = config.dram_memory_range / num_partitions;

  // Set up thread synchronization and execution parameters
  const size_t ops_per_chunk =
      config.access_size < config.min_io_chunk_size ? config.min_io_chunk_size / config.access_size : 1;
  // Add one chunk for non-divisible numbers so that we perform at least number_operations ops and not fewer.
  const size_t num_chunks = (num_operations / ops_per_chunk) + (num_operations % ops_per_chunk != 0);

  execution->threads_remaining = config.number_threads;
  execution->io_position = 0;
  execution->io_operations.resize(num_chunks);
  execution->num_custom_chunks_remaining = num_chunks;

  for (uint16_t partition_num = 0; partition_num < num_partitions; partition_num++) {
    char* partition_start = (config.exec_mode == Mode::Sequential_Desc)
                                ? pmem_data + ((num_partitions - partition_num) * partition_size) - config.access_size
                                : pmem_data + (partition_num * partition_size);

    // Only possible in random or custom mode
    char* dram_partition_start = dram_data + (partition_num * partition_size);

    for (uint16_t partition_thread_num = 0; partition_thread_num < num_threads_per_partition; partition_thread_num++) {
      const uint32_t thread_idx = (partition_num * num_threads_per_partition) + partition_thread_num;

      // Reserve space for custom operation latency measurements to avoid resizing during benchmark execution.
      if (is_custom_execution) {
        result->custom_operation_latencies[thread_idx].reserve(estimate_num_latency_measurements);
      }

      ExecutionDuration* total_op_duration = &result->total_operation_durations[thread_idx];
      uint64_t* total_op_size = &result->total_operation_sizes[thread_idx];
      uint64_t* custom_op_duration = is_custom_execution ? &result->custom_operation_durations[thread_idx] : nullptr;
      std::vector<uint64_t>* custom_op_latencies =
          is_custom_execution ? &result->custom_operation_latencies[thread_idx] : nullptr;

      thread_config->emplace_back(partition_start, dram_partition_start, partition_size, dram_partition_size,
                                  num_threads_per_partition, thread_idx, ops_per_chunk, num_chunks, config, execution,
                                  total_op_duration, total_op_size, custom_op_duration, custom_op_latencies);
    }
  }
}

char* Benchmark::create_pmem_data_file(const BenchmarkConfig& config, const MemoryRegion& memory_region) {
  if (std::filesystem::exists(memory_region.pmem_file)) {
    // Data was already generated. Only re-map it.
    return utils::map_pmem(memory_region.pmem_file, config.memory_range);
  }

  char* file_data = utils::create_pmem_file(memory_region.pmem_file, config.memory_range);
  prepare_data_file(file_data, config, config.memory_range, utils::PMEM_PAGE_SIZE);
  return file_data;
}

char* Benchmark::create_dram_data(const BenchmarkConfig& config) {
  char* dram_data = utils::map_dram(config.dram_memory_range, config.dram_huge_pages);
  prepare_data_file(dram_data, config, config.dram_memory_range, utils::DRAM_PAGE_SIZE);
  return dram_data;
}

void Benchmark::prepare_data_file(char* file_data, const BenchmarkConfig& config, const uint64_t memory_range,
                                  const uint64_t page_size) {
  if (config.contains_read_op()) {
    // If we read data in this benchmark, we need to generate it first.
    utils::generate_read_data(file_data, memory_range);
  }

  if (config.contains_write_op() && config.prefault_file) {
    utils::prefault_file(file_data, memory_range, page_size);
  }
}

void Benchmark::run_custom_ops_in_thread(ThreadRunConfig* thread_config, const BenchmarkConfig& config) {
  throw PermaException();
  const std::vector<CustomOp>& operations = config.custom_operations;
  const size_t num_ops = operations.size();

  std::vector<ChainedOperation> operation_chain;
  operation_chain.reserve(num_ops);

  // Determine maximum access size to ensure that operations don't write beyond the end of the range.
  size_t max_access_size = 0;
  for (const CustomOp& op : operations) {
    max_access_size = std::max(op.size, max_access_size);
  }

  const size_t aligned_range_size = thread_config->partition_size - max_access_size;
  const size_t aligned_dram_range_size = thread_config->dram_partition_size - max_access_size;

  for (size_t i = 0; i < num_ops; ++i) {
    const CustomOp& op = operations[i];

    if (op.is_pmem) {
      operation_chain.emplace_back(op, thread_config->partition_start_addr, aligned_range_size);
    } else {
      operation_chain.emplace_back(op, thread_config->dram_partition_start_addr, aligned_dram_range_size);
    }

    if (i > 0) {
      operation_chain[i - 1].set_next(&operation_chain[i]);
    }
  }

  const size_t seed = std::chrono::steady_clock::now().time_since_epoch().count() * (thread_config->thread_num + 1);
  lehmer64_seed(seed);
  char* start_addr = (char*)seed;

  ChainedOperation& start_op = operation_chain[0];
  auto start_ts = std::chrono::steady_clock::now();

  if (config.latency_sample_frequency == 0) {
    // We don't want the sampling code overhead if we don't want to sample the latency.
    while (true) {
      for (size_t iteration = 0; iteration < thread_config->num_ops_per_chunk; ++iteration) {
        start_op.run(start_addr, start_addr);
      }
      if (thread_config->execution->num_custom_chunks_remaining.fetch_sub(1)) {
        // TODO
      }
    }
  } else {
    // Latency sampling requested, measure the latency every x iterations.
    const uint64_t freq = config.latency_sample_frequency;
    // Start at 1 to avoid measuring latency of first request.
    for (size_t iteration = 1; iteration <= thread_config->num_ops_per_chunk; ++iteration) {
      if (iteration % freq == 0) {
        auto op_start = std::chrono::steady_clock::now();
        start_op.run(start_addr, start_addr);
        auto op_end = std::chrono::steady_clock::now();
        thread_config->custom_op_latencies->emplace_back((op_end - op_start).count());
      } else {
        start_op.run(start_addr, start_addr);
      }
    }
  }

  auto end_ts = std::chrono::steady_clock::now();
  auto duration = (end_ts - start_ts).count();
  *(thread_config->custom_op_duration) = duration;
}

void Benchmark::run_in_thread(ThreadRunConfig* thread_config, const BenchmarkConfig& config) {
  if (config.numa_pattern == NumaPattern::Far) {
    set_to_far_cpus();
  }

  if (config.exec_mode == Mode::Custom) {
    return run_custom_ops_in_thread(thread_config, config);
  }

  const size_t ops_per_iteration = thread_config->num_threads_per_partition * config.access_size;
  const uint32_t num_accesses_in_range = thread_config->partition_size / config.access_size;
  const uint32_t num_dram_accesses_in_range = thread_config->dram_partition_size / config.access_size;
  const bool is_read_op = config.operation == Operation::Read;
  const size_t thread_num_in_partition = thread_config->thread_num % thread_config->num_threads_per_partition;
  const size_t thread_partition_offset = thread_num_in_partition * config.access_size;
  const auto dram_target_ratio = static_cast<uint64_t>(config.dram_operation_ratio * 100);

  const size_t seed = std::chrono::steady_clock::now().time_since_epoch().count() * (thread_config->thread_num + 1);
  lehmer64_seed(seed);

  auto dram_target_distribution = [&]() { return (lehmer64() % 100) < dram_target_ratio; };
  auto access_distribution = [&]() { return lehmer64() % num_accesses_in_range; };
  auto dram_access_distribution = [&]() { return lehmer64() % num_dram_accesses_in_range; };

  spdlog::debug("Thread #{}: Starting address generation", thread_config->thread_num);
  const auto generation_begin_ts = std::chrono::steady_clock::now();

  char* next_op_position = config.exec_mode == Mode::Sequential_Desc
                               ? thread_config->partition_start_addr - thread_partition_offset
                               : thread_config->partition_start_addr + thread_partition_offset;

  // Create all chunks before executing.
  size_t num_chunks_per_thread = thread_config->num_chunks / config.number_threads;
  const size_t remaining_chunks = thread_config->num_chunks % config.number_threads;
  if (remaining_chunks > 0 && thread_config->thread_num < remaining_chunks) {
    // This thread needs to create an extra chunk for an uneven number.
    num_chunks_per_thread++;
  }

  for (size_t chunk_num = 0; chunk_num < num_chunks_per_thread; ++chunk_num) {
    std::vector<char*> op_addresses(thread_config->num_ops_per_chunk);

    for (size_t io_op = 0; io_op < thread_config->num_ops_per_chunk; ++io_op) {
      switch (config.exec_mode) {
        case Mode::Random: {
          char* partition_start;
          uint32_t num_target_accesses_in_range;
          std::function<uint64_t()> random_distribution;
          // Get memory target
          if (config.is_hybrid && dram_target_distribution()) {
            // DRAM target
            partition_start = thread_config->dram_partition_start_addr;
            num_target_accesses_in_range = num_dram_accesses_in_range;
            random_distribution = dram_access_distribution;
          } else {
            // PMEM target
            partition_start = thread_config->partition_start_addr;
            num_target_accesses_in_range = num_accesses_in_range;
            random_distribution = access_distribution;
          }

          uint64_t random_value;
          if (config.random_distribution == RandomDistribution::Uniform) {
            random_value = random_distribution();
          } else {
            random_value = utils::zipf(config.zipf_alpha, num_target_accesses_in_range);
          }
          op_addresses[io_op] = partition_start + (random_value * config.access_size);
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
          spdlog::error("Illegal state. Cannot be in `run_in_thread()` with different mode.");
          utils::crash_exit();
        }
      }
    }

    // We can always pass the persist_instruction as is. It is ignored for read access.
    Operation op = is_read_op ? Operation::Read : Operation::Write;
    const size_t insert_pos = (chunk_num * config.number_threads) + thread_config->thread_num;

    IoOperation& current_io = thread_config->execution->io_operations[insert_pos];
    current_io.op_addresses_ = std::move(op_addresses);
    current_io.access_size_ = config.access_size;
    current_io.op_type_ = op;
    current_io.persist_instruction_ = config.persist_instruction;
  }

  const auto generation_end_ts = std::chrono::steady_clock::now();
  const uint64_t generation_duration_us =
      std::chrono::duration_cast<std::chrono::milliseconds>(generation_end_ts - generation_begin_ts).count();
  spdlog::debug("Thread #{}: Finished address generation in {} ms", thread_config->thread_num, generation_duration_us);

  bool is_last;
  uint16_t& threads_remaining = thread_config->execution->threads_remaining;
  {
    std::lock_guard<std::mutex> gen_lock{thread_config->execution->generation_lock};
    threads_remaining -= 1;
    is_last = threads_remaining == 0;
  }

  if (is_last) {
    thread_config->execution->generation_done.notify_all();
  } else {
    std::unique_lock<std::mutex> gen_lock{thread_config->execution->generation_lock};
    thread_config->execution->generation_done.wait(gen_lock, [&] { return threads_remaining == 0; });
  }

  // Generation is done in all threads, start execution
  const auto execution_begin_ts = std::chrono::steady_clock::now();
  std::atomic<uint64_t>* io_position = &thread_config->execution->io_position;

  uint64_t num_executed_operations;
  if (config.run_time == 0) {
    num_executed_operations = run_fixed_sized_benchmark(&thread_config->execution->io_operations, io_position);
  } else {
    const auto execution_end = execution_begin_ts + std::chrono::seconds{config.run_time};
    num_executed_operations =
        run_duration_based_benchmark(&thread_config->execution->io_operations, io_position, execution_end);
  }

  const auto execution_end_ts = std::chrono::steady_clock::now();
  const auto execution_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(execution_end_ts - execution_begin_ts);
  spdlog::debug("Thread #{}: Finished execution in {} ms", thread_config->thread_num, execution_duration.count());

  const uint64_t chunk_size = config.access_size * thread_config->num_ops_per_chunk;
  *(thread_config->total_operation_size) = num_executed_operations * chunk_size;
  *(thread_config->total_operation_duration) = ExecutionDuration{execution_begin_ts, execution_end_ts};
}

uint64_t Benchmark::run_fixed_sized_benchmark(std::vector<IoOperation>* io_operations,
                                              std::atomic<uint64_t>* io_position) {
  const uint64_t total_num_operations = io_operations->size();
  uint64_t num_executed_operations = 0;

  while (true) {
    const uint64_t op_pos = io_position->fetch_add(1);
    if (op_pos >= total_num_operations) {
      break;
    }

    (*io_operations)[op_pos].run();
    num_executed_operations++;
  }

  return num_executed_operations;
}

uint64_t Benchmark::run_duration_based_benchmark(std::vector<IoOperation>* io_operations,
                                                 std::atomic<uint64_t>* io_position,
                                                 std::chrono::steady_clock::time_point execution_end) {
  const uint64_t total_num_operations = io_operations->size();
  uint64_t num_executed_operations = 0;

  while (true) {
    const uint64_t work_package = io_position->fetch_add(1) % total_num_operations;

    (*io_operations)[work_package].run();
    num_executed_operations++;

    const auto current_time = std::chrono::steady_clock::now();
    if (current_time > execution_end) {
      break;
    }
  }

  return num_executed_operations;
}

nlohmann::json Benchmark::get_benchmark_config_as_json(const BenchmarkConfig& bm_config) {
  nlohmann::json config;
  config["memory_type"] = utils::get_enum_as_string(ConfigEnums::str_to_mem_type, bm_config.is_pmem);
  config["memory_range"] = bm_config.memory_range;
  config["exec_mode"] = utils::get_enum_as_string(ConfigEnums::str_to_mode, bm_config.exec_mode);
  config["number_partitions"] = bm_config.number_partitions;
  config["number_threads"] = bm_config.number_threads;
  config["numa_pattern"] = utils::get_enum_as_string(ConfigEnums::str_to_numa_pattern, bm_config.numa_pattern);
  config["prefault_file"] = bm_config.prefault_file;
  config["min_io_chunk_size"] = bm_config.min_io_chunk_size;

  if (bm_config.is_hybrid) {
    config["dram_memory_range"] = bm_config.dram_memory_range;
    config["dram_operation_ratio"] = bm_config.dram_operation_ratio;
    config["dram_huge_pages"] = bm_config.dram_huge_pages;
  }

  if (bm_config.exec_mode != Mode::Custom) {
    config["access_size"] = bm_config.access_size;
    config["operation"] = utils::get_enum_as_string(ConfigEnums::str_to_operation, bm_config.operation);

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

  if (bm_config.run_time > 0) {
    config["run_time"] = bm_config.run_time;
  }

  return config;
}

const std::vector<BenchmarkConfig>& Benchmark::get_benchmark_configs() const { return configs_; }

const std::filesystem::path& Benchmark::get_pmem_file(const uint8_t index) const {
  return memory_regions_[index].pmem_file;
}

bool Benchmark::owns_pmem_file(const uint8_t index) const { return memory_regions_[index].owns_pmem_file; }

const std::vector<char*>& Benchmark::get_pmem_data() const { return pmem_data_; }

const std::vector<char*>& Benchmark::get_dram_data() const { return dram_data_; }

const std::vector<std::vector<ThreadRunConfig>>& Benchmark::get_thread_configs() const { return thread_configs_; }
const std::vector<std::unique_ptr<BenchmarkResult>>& Benchmark::get_benchmark_results() const { return results_; }
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

  custom_operation_latencies.clear();
  custom_operation_latencies.shrink_to_fit();
}

nlohmann::json BenchmarkResult::get_result_as_json() const {
  if (config.exec_mode == Mode::Custom) {
    return get_custom_results_as_json();
  }

  uint64_t total_size = 0;
  std::vector<double> per_thread_bandwidth(config.number_threads);

  if (total_operation_durations.size() != config.number_threads) {
    spdlog::critical("Invalid state! Need n result durations for n threads. Got: {} but expected: {}",
                     total_operation_durations.size(), config.number_threads);
    utils::crash_exit();
  }

  if (total_operation_sizes.size() != config.number_threads) {
    spdlog::critical("Invalid state! Need n result sizes for n threads. Got: {} but expected: {}",
                     total_operation_sizes.size(), config.number_threads);
    utils::crash_exit();
  }

  std::chrono::steady_clock::time_point earliest_begin = total_operation_durations[0].begin;
  std::chrono::steady_clock::time_point latest_end = total_operation_durations[0].end;

  nlohmann::json per_thread_results = nlohmann::json::array();

  for (uint16_t thread_num = 0; thread_num < config.number_threads; thread_num++) {
    const ExecutionDuration& thread_timestamps = total_operation_durations[thread_num];
    const std::chrono::steady_clock::duration thread_duration = thread_timestamps.end - thread_timestamps.begin;
    const uint64_t thread_op_size = total_operation_sizes[thread_num];

    const double thread_bandwidth = get_bandwidth(thread_op_size, thread_duration);
    per_thread_bandwidth[thread_num] = thread_bandwidth;

    spdlog::debug("Thread #{}: Per-Thread Information", thread_num);
    spdlog::debug(" ├─ Bandwidth (GiB/s): {:.5f}", thread_bandwidth);
    spdlog::debug(" ├─ Total Access Size (MiB): {}", thread_op_size / BYTES_IN_MEGABYTE);
    spdlog::debug(" └─ Duration (s): {:.5f}", static_cast<double>(thread_duration.count()) / NANOSECONDS_IN_SECONDS);

    total_size += thread_op_size;
    earliest_begin = std::min(earliest_begin, thread_timestamps.begin);
    latest_end = std::max(latest_end, thread_timestamps.end);

    nlohmann::json thread_results;
    thread_results["bandwidth"] = thread_bandwidth;
    thread_results["execution_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(thread_duration).count();
    thread_results["accessed_bytes"] = thread_op_size;
    per_thread_results.emplace_back(std::move(thread_results));
  }

  // TODO: update!
  // Calculate final bandwidth. We base this on the "slowest" thread, i.e., the maximum duration. This slightly
  // underestimates the actual bandwidth utilization but avoids weird settings where threads have largely varying
  // runtimes, e.g., through hyper-threading, which overestimates the per-thread bandwidth and shows numbers that are
  // beyond the physical limit of the hardware. We are rather too conservative than claim numbers that are not true.
  const auto execution_time = latest_end - earliest_begin;
  const double total_bandwidth = get_bandwidth(total_size, execution_time);

  // Add information about per-thread avg and standard deviation.
  const double avg_bandwidth = total_bandwidth / config.number_threads;

  std::vector<double> thread_diff_to_avg(config.number_threads);
  std::transform(per_thread_bandwidth.begin(), per_thread_bandwidth.end(), thread_diff_to_avg.begin(),
                 [&](double x) { return x - avg_bandwidth; });
  const double sq_sum =
      std::inner_product(thread_diff_to_avg.begin(), thread_diff_to_avg.end(), thread_diff_to_avg.begin(), 0.0);
  // Use N - 1 for sample variance
  const double bandwidth_stddev = std::sqrt(sq_sum / std::max(1, config.number_threads - 1));

  spdlog::debug("Per-Thread Average Bandwidth: {}", avg_bandwidth);
  spdlog::debug("Per-Thread Standard Deviation: {}", bandwidth_stddev);
  spdlog::debug("Total Bandwidth: {}", total_bandwidth);

  nlohmann::json result;
  nlohmann::json bandwidth_results;

  std::chrono::duration<float> execution_time_s = execution_time;

  bandwidth_results["bandwidth"] = total_bandwidth;
  //  bandwidth_results["duration"] = std::chrono::duration_cast<std::chrono::milliseconds>(execution_time).count();
  bandwidth_results["execution_time"] = execution_time_s.count();
  bandwidth_results["accessed_bytes"] = total_size;
  bandwidth_results["thread_bandwidth_avg"] = avg_bandwidth;
  bandwidth_results["thread_bandwidth_std_dev"] = bandwidth_stddev;
  bandwidth_results["threads"] = per_thread_results;

  result["results"] = bandwidth_results;

  if (execution_time < std::chrono::seconds{1}) {
    spdlog::warn(
        "Benchmark ran less then 1 second ({} ms). The results may be inaccurate due to the short execution time.",
        std::chrono::duration_cast<std::chrono::milliseconds>(execution_time).count());
  }

  return result;
}

nlohmann::json BenchmarkResult::get_custom_results_as_json() const {
  nlohmann::json result;

  uint64_t total_duration = 0;
  for (uint64_t thread_id = 0; thread_id < config.number_threads; ++thread_id) {
    total_duration += custom_operation_durations[thread_id];
  }
  const double avg_duration_ns = static_cast<double>(total_duration) / config.number_threads;
  const double avg_duration = avg_duration_ns / NANOSECONDS_IN_SECONDS;
  const double ops_per_sec = config.number_operations / avg_duration;

  // TODO(#169): Add information about per-thread avg and standard deviation.

  result["avg_duration_sec"] = avg_duration;
  result["ops_per_second"] = ops_per_sec;

  if (config.latency_sample_frequency > 0) {
    for (const std::vector<uint64_t>& thread_latencies : custom_operation_latencies) {
      for (const uint64_t latency : thread_latencies) {
        hdr_record_value(latency_hdr, latency);
      }
    }
    result["latency"] = hdr_histogram_to_json(latency_hdr);
  }

  return result;
}

}  // namespace perma
