#pragma once

#include <filesystem>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, size_t expected_length);
char* create_pmem_file(const std::filesystem::path& file, size_t length);

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir);

uint64_t duration_to_nanoseconds(std::chrono::high_resolution_clock::duration duration);

// Returns a Zipf random variable
uint64_t zipf(double alpha, uint64_t n);
double rand_val();

void init_numa(const std::filesystem::path& pmem_dir);

}  // namespace perma
