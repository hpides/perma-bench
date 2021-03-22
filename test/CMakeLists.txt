include(FetchContent)

add_custom_target(copy_test_resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/test/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)

##################### DRAM/PMEM ####################

set(pmem_prog "
#include <asm-generic/mman-common.h>
#include <asm-generic/mman.h>
#include <fcntl.h>

#include <fstream>
int main() {
  const std::filesystem::path test_file = std::filesystem::temp_directory_path() / \"pmem_test.file\";
  std::ofstream temp_stream{test_file};
  temp_stream.close();
  std::filesystem::resize_file(test_file, 1024);
  int fd = open(test_file.c_str(), O_RDWR | O_DIRECT, 0644);
  void* addr = mmap(nullptr, 1024, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE | MAP_SYNC, fd, 0);
  close(fd);
  return addr != MAP_FAILED }")
check_c_source_runs("${pmem_prog}" HAS_PMEM_TEST)

if (${HAS_PMEM_TEST})
#Set HAS_PMEM_TEST so we can use it to enable the tests on pmem.
    add_definitions("-DHAS_PMEM_TEST")
    message(STATUS "Tests are run on persistent memory.")
else ()
    message(STATUS "Tests are run on DRAM.")
endif ()

##################### GoogleTest ####################
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(PERMA_TEST_SOURCES
        benchmark_test.cpp
        config_test.cpp
        test_utils.cpp
        test_utils.hpp
        utils_test.cpp
        read_write_test.cpp)

add_executable(perma-test perma_test.cpp ${PERMA_TEST_SOURCES})
add_dependencies(perma-test copy_test_resources)
target_link_libraries(perma-test gtest perma)
add_test(perma-test perma-test)
