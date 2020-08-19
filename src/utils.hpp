#pragma once

#include <filesystem>

namespace nvmbm {

void* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length);
void* create_pmem_file(const std::filesystem::path& file, size_t length);

}  // namespace nvmbm