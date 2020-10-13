#pragma once

#include <filesystem>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length);
char* create_pmem_file(const std::filesystem::path& file, size_t length);

uint64_t duration_to_nanoseconds(std::chrono::high_resolution_clock::duration duration);

}  // namespace perma
