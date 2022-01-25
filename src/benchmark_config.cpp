#include "benchmark_config.hpp"

#include <spdlog/spdlog.h>

#include "numa.hpp"
#include "utils.hpp"

namespace {

#define CHECK_ARGUMENT(exp, txt) \
  if (!(exp)) {                  \
    spdlog::critical(txt);       \
    perma::utils::crash_exit();  \
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

}  // namespace

namespace perma {

BenchmarkConfig BenchmarkConfig::decode(YAML::Node& node) {
  BenchmarkConfig bm_config{};
  size_t num_found = 0;
  try {
    num_found +=
        get_size_if_present(node, "memory_range", ConfigEnums::scale_suffix_to_factor, &bm_config.memory_range);
    num_found += get_size_if_present(node, "dram_memory_range", ConfigEnums::scale_suffix_to_factor,
                                     &bm_config.dram_memory_range);
    num_found += get_size_if_present(node, "access_size", ConfigEnums::scale_suffix_to_factor, &bm_config.access_size);

    num_found += get_if_present(node, "number_operations", &bm_config.number_operations);
    num_found += get_if_present(node, "run_time", &bm_config.run_time);
    num_found += get_if_present(node, "number_partitions", &bm_config.number_partitions);
    num_found += get_if_present(node, "number_threads", &bm_config.number_threads);
    num_found += get_if_present(node, "zipf_alpha", &bm_config.zipf_alpha);
    num_found += get_if_present(node, "prefault_file", &bm_config.prefault_file);
    num_found += get_if_present(node, "min_io_chunk_size", &bm_config.min_io_chunk_size);
    num_found += get_if_present(node, "latency_sample_frequency", &bm_config.latency_sample_frequency);

    num_found += get_enum_if_present(node, "exec_mode", ConfigEnums::str_to_mode, &bm_config.exec_mode);
    num_found += get_enum_if_present(node, "operation", ConfigEnums::str_to_operation, &bm_config.operation);
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

  // Check if PMem memory range is multiple of access size
  const bool is_memory_range_multiple_of_access_size = (memory_range % access_size) == 0;
  CHECK_ARGUMENT(is_memory_range_multiple_of_access_size, "PMem memory range must be a multiple of access size.");

  // Check if DRAM memory range is multiple of access size
  const bool is_dram_memory_range_multiple_of_access_size =
      dram_memory_range == 0 || (dram_memory_range % access_size) == 0;
  CHECK_ARGUMENT(is_memory_range_multiple_of_access_size, "DRAM memory range must be a multiple of access size or 0.");

  // Check if runtime is at least one second
  const bool is_at_least_one_second_or_default = run_time > 0 || run_time == -1;
  CHECK_ARGUMENT(is_at_least_one_second_or_default, "Run time be at least 1 second");

  // Check if at least one thread
  const bool is_at_least_one_thread = number_threads > 0;
  CHECK_ARGUMENT(is_at_least_one_thread, "Number threads must be at least 1.");

  // Check if at least one partition
  const bool is_at_least_one_partition = number_partitions > 0;
  CHECK_ARGUMENT(is_at_least_one_partition, "Number partitions must be at least 1.");

  // Assumption: number_threads is multiple of number_partitions
  const bool is_number_threads_multiple_of_number_partitions = (number_threads % number_partitions) == 0;
  CHECK_ARGUMENT(is_number_threads_multiple_of_number_partitions,
                 "Number threads must be a multiple of number partitions.");

  // Assumption: total memory range must be evenly divisible into number of partitions
  const bool is_partitionable = ((memory_range / number_partitions) % access_size) == 0;
  CHECK_ARGUMENT(is_partitionable,
                 "Total memory range must be evenly divisible into number of partitions. "
                 "Most likely you can fix this by using 2^x partitions.");

  // Assumption: number_operations should only be set for random/custom access. It is ignored in sequential IO.
  const bool is_number_operations_set_random = number_operations == BenchmarkConfig{}.number_operations ||
                                               (exec_mode == Mode::Random || exec_mode == Mode::Custom);
  CHECK_ARGUMENT(is_number_operations_set_random, "Number of operations should only be set for random/custom access.");

  // Assumption: min_io_chunk size must be a power of two
  const bool is_valid_min_io_chunk_size = min_io_chunk_size >= 64 && (min_io_chunk_size & (min_io_chunk_size - 1)) == 0;
  CHECK_ARGUMENT(is_valid_min_io_chunk_size, "Minimum IO chunk must be >= 64 Byte and a power of two.");

  // Assumption: total memory needs to fit into N chunks exactly
  const bool is_seq_total_memory_chunkable =
      (exec_mode == Mode::Random || exec_mode == Mode::Custom) || (memory_range % min_io_chunk_size) == 0;
  CHECK_ARGUMENT(is_seq_total_memory_chunkable,
                 "Total file size needs to be multiple of chunk size " + std::to_string(min_io_chunk_size));

  // Assumption: we chunk operations and we need enough data to fill at least one chunk
  const bool is_total_memory_large_enough = (memory_range / number_threads) >= min_io_chunk_size;
  CHECK_ARGUMENT(is_total_memory_large_enough,
                 "Each thread needs at least " + std::to_string(min_io_chunk_size) + " memory.");

  const bool is_far_numa_node_available = numa_pattern == NumaPattern::Near || has_far_numa_nodes();
  CHECK_ARGUMENT(is_far_numa_node_available, "Cannot run far NUMA node benchmark without far NUMA nodes.");

  const bool has_custom_ops = exec_mode != Mode::Custom || !custom_operations.empty();
  CHECK_ARGUMENT(has_custom_ops, "Must specify custom_operations for custom execution.");

  const bool has_no_custom_ops = exec_mode == Mode::Custom || custom_operations.empty();
  CHECK_ARGUMENT(has_no_custom_ops, "Cannot specify custom_operations for non-custom execution.");

  const bool latency_sample_is_custom = exec_mode == Mode::Custom || latency_sample_frequency == 0;
  CHECK_ARGUMENT(latency_sample_is_custom, "Latency sampling can only be used with custom operations.");
}

bool BenchmarkConfig::contains_read_op() const { return operation == Operation::Read || exec_mode == Mode::Custom; }

bool BenchmarkConfig::contains_write_op() const {
  auto find_custom_write_op = [](const CustomOp& op) { return op.type == Operation::Write; };
  return operation == Operation::Write ||
         std::any_of(custom_operations.begin(), custom_operations.end(), find_custom_write_op);
}

const std::unordered_map<std::string, bool> ConfigEnums::str_to_mem_type{{"pmem", true}, {"dram", false}};

const std::unordered_map<std::string, Mode> ConfigEnums::str_to_mode{{"sequential", Mode::Sequential},
                                                                     {"sequential_asc", Mode::Sequential},
                                                                     {"sequential_desc", Mode::Sequential_Desc},
                                                                     {"random", Mode::Random},
                                                                     {"custom", Mode::Custom}};

const std::unordered_map<std::string, Operation> ConfigEnums::str_to_operation{{"read", Operation::Read},
                                                                               {"write", Operation::Write}};

const std::unordered_map<std::string, NumaPattern> ConfigEnums::str_to_numa_pattern{{"near", NumaPattern::Near},
                                                                                    {"far", NumaPattern::Far}};

const std::unordered_map<std::string, PersistInstruction> ConfigEnums::str_to_persist_instruction{
    {"nocache", PersistInstruction::NoCache}, {"cache", PersistInstruction::Cache}, {"none", PersistInstruction::None}};

const std::unordered_map<std::string, RandomDistribution> ConfigEnums::str_to_random_distribution{
    {"uniform", RandomDistribution::Uniform}, {"zipf", RandomDistribution::Zipf}};

const std::unordered_map<char, uint64_t> ConfigEnums::scale_suffix_to_factor{{'k', 1024},
                                                                             {'K', 1024},
                                                                             {'m', 1024 * 1024},
                                                                             {'M', 1024 * 1024},
                                                                             {'g', 1024 * 1024 * 1024},
                                                                             {'G', 1024 * 1024 * 1024}};

}  // namespace perma
