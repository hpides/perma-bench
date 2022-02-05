cmake_minimum_required(VERSION 3.16)
project(perma-bench)

include(ExternalProject)
include(FetchContent)
include(CheckCXXSourceCompiles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -pthread")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message("-- Release mode, all optimizations enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -g")
endif ()

set(INCLUDE_HINTS ${CMAKE_INCLUDE_PATH} "/usr/include" "/usr/local/include")
set(LIB_HINTS "/usr/local/lib64" "/usr/local/lib" "/usr/lib" "/usr/lib64")

# CI
option(IS_CI_BUILD "Set true if this is a CI build.")

##################### AVX ####################
# Use this short program to check if AVX-512 is supported. For now, we only use AVX-512 or not.
# This might need to be adapted in the future to support different SSE/AVX instructions.
include(CheckCSourceRuns)
set(avx512_prog "int main() { asm volatile(\"vmovdqu64 %zmm0, %zmm1\"); return 0; }")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -mavx512f")
check_c_source_runs("${avx512_prog}" HAS_AVX)
if (${HAS_AVX})
    # Set HAS_AVX so we can use it to enable all explicit AVX instructions.
    add_definitions("-DHAS_AVX")
    message(STATUS "System supports AVX-512.")
else ()
    if (${IS_CI_BUILD})
        message(STATUS "System does not supports AVX-512. This is okay only in a CI build.")
    else ()
        message(FATAL_ERROR "System does not support AVX-512.")
    endif ()
endif ()

# Backup of actual required flags
set(BASE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

##################### CLWB ####################
set(clwb_prog "#include <immintrin.h>\n int main() { _mm_clwb((void*)0); return 0; }")
set(CMAKE_REQUIRED_FLAGS "${BASE_CMAKE_REQUIRED_FLAGS} -mclwb")
check_cxx_source_compiles("${clwb_prog}" HAS_CLWB)
if (${HAS_CLWB})
    # Set HAS_CLWB so we can use it to enable all explicit CLWB instructions.
    add_definitions("-DHAS_CLWB")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mclwb")
    message(STATUS "System supports CLWB.")
else ()
    message(STATUS "System does not support CLWB.")
endif ()

##################### CLFLUSHOPT ####################
set(clflush_prog "#include <immintrin.h>\n int main() { _mm_clflushopt((void*)0); return 0; }")
set(CMAKE_REQUIRED_FLAGS "${BASE_CMAKE_REQUIRED_FLAGS} -mclflushopt")
check_cxx_source_compiles("${clflush_prog}" HAS_CLFLUSHOPT)
if (${HAS_CLFLUSHOPT})
    # Set HAS_CLFLUSHOPT so we can use it to enable all explicit CLFLUSHOPT instructions.
    add_definitions("-DHAS_CLFLUSHOPT")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mclflushopt")
    message(STATUS "System supports CLFLUSHOPT.")
else ()
    message(STATUS "System does not support CLFLUSHOPT.")
endif ()

################### NUMA ####################
set(NUMA_INCLUDE_PATH "" CACHE STRING "Path to custom NUMA include files")
set(NUMA_LIBRARY_PATH "" CACHE STRING "Path to custom NUMA library")

set(NUMA_INCLUDE_HINTS ${NUMA_INCLUDE_PATH} ${INCLUDE_HINTS})
set(NUMA_LIB_HINTS ${NUMA_LIBRARY_PATH})

find_path(NUMA_INCLUDE_DIRS numa.h HINTS ${NUMA_INCLUDE_HINTS})
find_library(NUMA_LIBRARIES NAMES numa libnuma HINTS ${NUMA_LIB_HINTS})

set(NUMA_FOUND true)
if (NOT NUMA_INCLUDE_DIRS OR "${NUMA_INCLUDE_DIRS}" STREQUAL "")
    message(STATUS "WARNING: numa include directory not found. Not using NUMA in this build.")
    set(NUMA_FOUND false)
endif ()
if (NOT NUMA_LIBRARIES OR "${NUMA_LIBRARIES}" STREQUAL "")
    message(STATUS "WARNING: libnuma not found. Not using NUMA in this build.")
    set(NUMA_FOUND false)
endif ()

if (${NUMA_FOUND})
    include_directories(${NUMA_INCLUDE_DIRS})
    message(STATUS "NUMA provided. Including ${NUMA_INCLUDE_DIRS} and linking ${NUMA_LIBRARIES}.")
    add_definitions("-DHAS_NUMA")
    message(STATUS "Using libnuma in this build.")
endif()

##################### YAML ####################
set(YAML_CPP_BUILD_TESTS OFF CACHE INTERNAL "")
set(YAML_CPP_INSTALL OFF CACHE INTERNAL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "")

FetchContent_Declare(
        yaml_cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.6.3
)
FetchContent_MakeAvailable(yaml_cpp)

##################### JSON ####################
set(JSON_VERSION v3.10.5)
set(JSON_DOWNLOAD_PATH https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/json.hpp)
set(JSON_DIR ${CMAKE_CURRENT_BINARY_DIR}/json)
set(JSON_INCLUDE_PATH ${JSON_DIR}/json.hpp)
file(DOWNLOAD ${JSON_DOWNLOAD_PATH} ${JSON_INCLUDE_PATH})
include_directories(${JSON_DIR})

################### Hdr Histogram ####################
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE INTERNAL "")
FetchContent_Declare(
        hdr_histogram
        GIT_REPOSITORY https://github.com/HdrHistogram/HdrHistogram_c.git
        GIT_TAG 0.11.2
)
FetchContent_MakeAvailable(hdr_histogram)

################### spdlog ####################
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.9.2
)
FetchContent_MakeAvailable(spdlog)

################### CLI11 ####################
set(CLI11_VERSION v2.1.2)
set(CLI11_DOWNLOAD_PATH https://github.com/CLIUtils/CLI11/releases/download/${CLI11_VERSION}/CLI11.hpp)
set(CLI11_DIR ${CMAKE_CURRENT_BINARY_DIR}/cli11)
set(CLI11_INCLUDE_PATH ${CLI11_DIR}/CLI11.hpp)
file(DOWNLOAD ${CLI11_DOWNLOAD_PATH} ${CLI11_INCLUDE_PATH})
include_directories(${CLI11_DIR})

##################### PerMA ####################
add_custom_target(copy_configs
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/workloads ${CMAKE_CURRENT_BINARY_DIR}/workloads)

include_directories(src)
add_subdirectory(src)

##################### Test ####################
option(BUILD_TEST "Set true if tests should be built and run.")

if (${BUILD_TEST})
    message(STATUS "Tests are included in this build.")
    enable_testing()
    add_subdirectory(test)
endif ()
