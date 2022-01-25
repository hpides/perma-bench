#pragma once

#include <asm-generic/mman-common.h>
#include <asm-generic/mman.h>
#include <sys/mman.h>

#include <filesystem>
#include <vector>

#include "json.hpp"

namespace perma::utils {

static constexpr size_t NUM_UTIL_THREADS = 4;                  // Should be a power of two
static constexpr size_t PMEM_PAGE_SIZE = 2 * (1024ul * 1024);  // 2 MiB persistent memory page size
static constexpr size_t DRAM_PAGE_SIZE = 4 * 1024ul;           // 4 KiB DRAM page size
static constexpr size_t ONE_GB = 1024ul * 1024 * 1024;

static int PMEM_MAP_FLAGS = MAP_SHARED_VALIDATE | MAP_SYNC;
static int DRAM_MAP_FLAGS = MAP_SHARED | MAP_ANONYMOUS;

void setPMEM_MAP_FLAGS(int flags);

class PermaException : public std::exception {
 public:
  const char* what() const noexcept override { return "Execution failed. Check logs for more details."; }
};

char* map_file(const std::filesystem::path& file, bool is_dram, size_t expected_length);
char* create_file(const std::filesystem::path& file, bool is_dram, size_t length);

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir);

void generate_read_data(char* addr, uint64_t total_memory_range);

void prefault_file(char* addr, uint64_t total_memory_range, uint64_t page_size);

uint64_t duration_to_nanoseconds(std::chrono::steady_clock::duration duration);

// Returns a Zipf random variable
uint64_t zipf(double alpha, uint64_t n);
double rand_val();

void crash_exit();
void print_segfault_error();

std::string get_time_string();
std::filesystem::path create_result_file(const std::filesystem::path& result_dir,
                                         const std::filesystem::path& config_path);
void write_benchmark_results(const std::filesystem::path& result_path, const nlohmann::json& results);

}  // namespace perma::utils
