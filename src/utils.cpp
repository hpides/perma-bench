#include "utils.hpp"

#include <libpmem.h>

#include <algorithm>
#include <iostream>
#include <random>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, size_t* mapped_length) {
  int is_pmem;
  void* pmem_addr = pmem_map_file(file.c_str(), 0, 0, 0, mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not map file: " + file.string()};
  }

  if (!is_pmem) {
    std::cout << "File " + file.string() + " is not in persistent memory!" << std::endl;
  }

  return static_cast<char*>(pmem_addr);
}

char* create_pmem_file(const std::filesystem::path& file, size_t length) {
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), length, PMEM_FILE_CREATE, 0644, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not create file: " + file.string()};
  }

  if (!is_pmem) {
    std::cout << "File " + file.string() + " is not in persistent memory!" << std::endl;
  }

  if (length != mapped_length) {
    throw std::runtime_error{"Mapped size different than specified size!"};
  }

  return static_cast<char*>(pmem_addr);
}

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir) {
  if (!std::filesystem::exists(base_dir)) {
    if (!std::filesystem::create_directories(base_dir)) {
      throw std::runtime_error{"Could not create dir: " + base_dir.string()};
    }
  }
  std::string str("abcdefghijklmnopqrstuvwxyz");
  std::random_device rd;
  std::mt19937 generator(rd());
  std::shuffle(str.begin(), str.end(), generator);
  const std::string file_name = str + ".file";
  const std::filesystem::path file{file_name};
  return base_dir / file;
}

uint64_t duration_to_nanoseconds(const std::chrono::high_resolution_clock::duration duration) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

}  // namespace perma
