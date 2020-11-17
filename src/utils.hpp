#pragma once

#include <filesystem>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length);
char* create_pmem_file(const std::filesystem::path& file, size_t length);

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir);

uint64_t duration_to_nanoseconds(std::chrono::high_resolution_clock::duration duration);

//----- Function prototypes -------------------------------------------------
uint64_t zipf(double alpha, uint64_t n);  // Returns a Zipf random variable
double rand_val();                        // Jain's RNG
}  // namespace perma
