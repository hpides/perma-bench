#include "utils.hpp"

#include <libpmem.h>

#include <iostream>

namespace nvmbm {

void* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length) {
  int is_pmem;
  void* pmem_addr =
      pmem_map_file(file.c_str(), 0, 0, 0, mapped_length, &is_pmem);
  if (pmem_addr == nullptr) {
    throw std::runtime_error{"Could not map file: " + file.string()};
  }

  if (!is_pmem) {
    std::cout << "File " + file.string() + " is not in persistent memory!"
              << std::endl;
  }

  return pmem_addr;
}

void* create_pmem_file(const std::filesystem::path& file, size_t length) {
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), length, PMEM_FILE_CREATE, 0644,
                                  &mapped_length, &is_pmem);
  if (pmem_addr == nullptr) {
    throw std::runtime_error{"Could not create file: " + file.string()};
  }

  if (!is_pmem) {
    std::cout << "File " + file.string() + " is not in persistent memory!"
              << std::endl;
  }

  if (length != mapped_length) {
    throw std::runtime_error{"Mapped size different than specified size!"};
  }

  return pmem_addr;
}

}  // namespace nvmbm
