#pragma once

#include <filesystem>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length);
char* create_pmem_file(const std::filesystem::path& file, size_t length);

}  // namespace perma
