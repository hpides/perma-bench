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
static constexpr size_t BYTES_IN_MEGABYTE = 1024u * 1024;
static constexpr size_t BYTES_IN_GIGABYTE = 1024u * BYTES_IN_MEGABYTE;
static constexpr size_t NANOSECONDS_IN_SECONDS = 1e9;

/**
 * This represents a custom operation to be specified by the user. Its string representation, is:
 *
 * For reads: r(<location>)_<size>
 *
 * with:
 * 'r' for read,
 * (optional) <location> is 'd' or 'p' for DRAM/PMem (with p as default is nothing is specified),
 * <size> is the size of the access (must be power of 2).
 *
 * For writes: w(<location>)_<size>_<persist_instruction>(_<offset>)
 *
 * with:
 * 'w' for write,
 * (optional) <location> is 'd' or 'p' for DRAM/PMem (with p as default is nothing is specified),
 * <size> is the size of the access (must be power of 2),
 * <persist_instruction> is the instruction to use after the write (none, cache, cacheinv, noache),
 * (optional) <offset> is the offset to the previously accessed address (can be negative, default is 0)
 *
 * */
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

  static bool validate(const std::vector<CustomOp>& operations);

  friend std::ostream& operator<<(std::ostream& os, const CustomOp& op);
  bool operator==(const CustomOp& rhs) const;
  bool operator!=(const CustomOp& rhs) const;
};

/**
 * The values shown here define the benchmark and represent user-facing configuration options.
 */
struct BenchmarkConfig {
  /** Represents the size of an individual memory access in Byte. Must be a power of two. */
  uint32_t access_size = 256;

  /** Represents the total PMem memory range to use for the benchmark. Must be a multiple of `access_size`.  */
  uint64_t memory_range = 10 * BYTES_IN_GIGABYTE;  // 10 GiB

  /** Represents the total DRAM memory range to use for the benchmark. Must be a multiple of `access_size`.  */
  uint64_t dram_memory_range = 0;

  /** Represents the ratio of DRAM IOOperations to PMem IOOperations. Must only contain one digit after decimal point,
   * i.e., 0.1 or 0.2. */
  double dram_operation_ratio = 0.0;

  /** Represents the number of random access / custom operations to perform. Can *not* be set for sequential access. */
  uint64_t number_operations = 100'000'000;

  /** Number of threads to run the benchmark with. Must be a power of two. */
  uint16_t number_threads = 1;

  /** Alternative measure to end a benchmark by letting is run for `run_time` seconds. */
  uint64_t run_time = 0;

  /** Type of memory access operation to perform, i.e., read or write. */
  Operation operation = Operation::Read;

  /** Mode of execution, i.e., sequential, random, or custom. See `Mode` for all options. */
  Mode exec_mode = Mode::Sequential;

  /** Persist instruction to use after write operations. Only works with `Operation::Write`. See
   * `PersistInstruction` for more details on available options. */
  PersistInstruction persist_instruction = PersistInstruction::NoCache;

  /** Number of disjoint memory regions to partition the `memory_range` into. Must be 0 or a divisor of
   * `number_threads` i.e., one or more threads map to one partition. When set to 0, it is equal to the number of
   * threads, i.e., each thread has its own partition. Default is set to 0, as it is more common for each thread to have
   * its own region in sequential access and the impact of shared/disjoint regions on random access is negligible when
   * the ranges are large enough. */
  uint16_t number_partitions = 0;

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

  /** Whether or not to use transparent huge pages in DRAM, i.e., 2 MiB instead of regular 4 KiB pages. */
  bool dram_huge_pages = true;

  /** Represents the minimum size of an atomic work package. A chunk contains chunk_size / access_size number of
   * operations. Assuming the lowest bandwidth of 1 GiB/s operations per thread, 64 MiB is a ~60 ms execution unit. */
  uint64_t min_io_chunk_size = 64 * BYTES_IN_MEGABYTE;

  /** These fields are set internally and do not represent user-facing options. */
  /** This field is required and has no default value, i.e., it must be set as a command line argument. */
  std::string pmem_directory{};
  std::vector<std::string> matrix_args{};
  bool is_pmem = true;
  bool is_hybrid = false;

  static BenchmarkConfig decode(YAML::Node& raw_config_data);
  void validate() const;
  bool contains_read_op() const;
  bool contains_write_op() const;
  bool contains_dram_op() const;
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
