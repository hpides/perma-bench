#pragma once

#include <yaml-cpp/yaml.h>

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace perma {

enum class Mode : uint8_t { Sequential, Sequential_Desc, Random, Custom };

enum class RandomDistribution : uint8_t { Uniform, Zipf };

enum class PersistInstruction : uint8_t { Cache, CacheInvalidate, NoCache, None };

enum class Operation : uint8_t { Read, Write };

enum class NumaPattern : uint8_t { Near, Far };

// We assume 2^30 for GB and not 10^9
static constexpr size_t BYTE_IN_MEGABYTE = 1024u * 1024;
static constexpr size_t BYTE_IN_GIGABYTE = 1024u * BYTE_IN_MEGABYTE;
static constexpr size_t NANOSECONDS_IN_SECONDS = 1e9;

struct CustomOp {
  Operation type;
  bool is_pmem = true;
  uint64_t size;
  PersistInstruction persist = PersistInstruction::None;
  // This can be signed, e.g., to represent the case when the previous cache line should be written to.
  int64_t offset = 0;

  static CustomOp from_string(const std::string& str);
  static std::vector<CustomOp> all_from_string(const std::string& str);
  static std::string all_to_string(const std::vector<CustomOp>& ops);
  static std::string to_string(const CustomOp& op);
  std::string to_string() const;

  friend std::ostream& operator<<(std::ostream& os, const CustomOp& op);
  bool operator==(const CustomOp& rhs) const;
  bool operator!=(const CustomOp& rhs) const;
};

/**
 * The values shown here define the benchmark and represent user-facing configuration options.
 */
struct BenchmarkConfig {
  /** Represents the total PMem memory range to use for the benchmark. Must be a multiple of `access_size`.  */
  uint64_t total_memory_range = 10'737'418'240;  // 10 GiB

  /** Represents the size of an individual memory access. Must tbe a power of two. */
  uint32_t access_size = 256;

  /** Represents the number of random access operations to perform. Can *not* be set for sequential access. */
  uint64_t number_operations = 10'000'000;

  /** Number of threads to run the benchmark with. Must be a power of two. */
  uint16_t number_threads = 1;

  /** Alternative measure to end a benchmark by letting is run for `run_time` seconds. */
  uint64_t run_time = UINT64_MAX;

  /** Type of memory access operation to perform, i.e., read or write. */
  Operation operation = Operation::Read;

  /** Mode of execution, i.e., sequential, random, or custom. See `Mode` for all options. */
  Mode exec_mode = Mode::Sequential;

  /** Persist instruction to use after write operations. Only works with `Operation::Write`. See
   * `PersistInstruction` for more details on available options. */
  PersistInstruction persist_instruction = PersistInstruction::NoCache;

  /** Number of disjoint memory regions to partition the `total_memory_range` into. Must be a multiple of
   * `number_threads`. */
  uint16_t number_partitions = 1;

  /** Define whether the memory access should be NUMA-local (`near`) or -remote (`far`). */
  NumaPattern numa_pattern = NumaPattern::Near;

  /** Distribution to use for `Mode::Random`, i.e., uniform of zipfian. */
  RandomDistribution random_distribution = RandomDistribution::Uniform;

  /** Zipf skew factor for `Mode::Random` and `RandomDistribution::Zipf`. */
  double zipf_alpha = 0.9;

  /** List of custom operations to use in `Mode::Custom`. See `CustomOp` for more details on string representation.  */
  std::vector<CustomOp> custom_operations;

  /** Frequency in which to sample latency of custom operations. Only works in combination with `Mode::Custom`. */
  uint64_t latency_sample_frequency = 0;

  /** Whether or not to prefault the memory region before writing to it. If set to false, the benchmark will include the
   * time caused by page faults on first access to the allocated memory region. */
  bool prefault_file = true;

  // TODO: remove this logic to avoid overhead of chunking
  uint64_t min_io_chunk_size = 128 * 1024u;  // 128 KiB

  /** This field is required and has no default value, i.e., it must be set as a command line argument. */
  std::string pmem_directory{};

  /** These fields are set internally and do not represent user-facing options. */
  std::vector<std::string> matrix_args{};
  bool is_pmem = true;

  static BenchmarkConfig decode(YAML::Node& raw_config_data);
  void validate() const;
  bool contains_read_op() const;
  bool contains_write_op() const;
};

struct ConfigEnums {
  // <read or write, is_pmem>
  using OpLocation = std::pair<perma::Operation, bool>;

  static const std::unordered_map<std::string, bool> str_to_mem_type;
  static const std::unordered_map<std::string, Mode> str_to_mode;
  static const std::unordered_map<std::string, Operation> str_to_operation;
  static const std::unordered_map<std::string, OpLocation> str_to_op_location;
  static const std::unordered_map<std::string, NumaPattern> str_to_numa_pattern;
  static const std::unordered_map<std::string, PersistInstruction> str_to_persist_instruction;
  static const std::unordered_map<std::string, RandomDistribution> str_to_random_distribution;

  // Map to convert a K/M/G suffix to the correct kibi, mebi-, gibibyte value.
  static const std::unordered_map<char, uint64_t> scale_suffix_to_factor;
};

}  // namespace perma
