#pragma once

#include <filesystem>
#include <vector>

namespace perma {

namespace internal {

static constexpr size_t NUM_UTIL_THREADS = 4;                  // Should be a power of two
static constexpr size_t PMEM_PAGE_SIZE = 2 * (1024ul * 1024);  // 2MiB persistent memory page size
static constexpr size_t ONE_GB = 1024ul * 1024 * 1024;

}  // namespace internal

char* map_pmem_file(const std::filesystem::path& file, size_t expected_length);
char* create_pmem_file(const std::filesystem::path& file, size_t length);

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir);

void generate_read_data(char* addr, uint64_t total_memory_range);

void prefault_file(char* addr, uint64_t total_memory_range);

uint64_t duration_to_nanoseconds(std::chrono::high_resolution_clock::duration duration);

// Returns a Zipf random variable
uint64_t zipf(double alpha, uint64_t n);
double rand_val();

void crash_exit();

void init_numa(const std::filesystem::path& pmem_dir, const std::vector<uint64_t>& arg_nodes);
void set_to_far_cpus();
bool has_far_numa_nodes();

}  // namespace perma
