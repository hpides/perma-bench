cmake_minimum_required(VERSION 3.16)
project(perma-bench)

include(ExternalProject)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(COMPILE_FLAGS "-march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message("-- Release mode, all optimizations enabled")
    set(COMPILE_FLAGS "${COMPILE_FLAGS} -O3 -DNDEBUG")
endif ()

##################### AVX ####################
# Use this short program to check if AVX-512 is supported. For now, we only use AVX-512 or not.
# This might need to be adapted in the future to support different SSE/AVX instructions.
include(CheckCSourceRuns)
set(avx512_prog "int main() { asm volatile(\"vmovdqu64 %zmm0, %zmm1\"); return 0; }")
set(CMAKE_REQUIRED_FLAGS "-mavx512f")
check_c_source_runs("${avx512_prog}" HAS_AVX)
if (${HAS_AVX})
    # Set HAS_AVX so we can use it to enable all explicit AVX instructions.
    add_definitions("-DHAS_AVX")
    message(STATUS "System supports AVX-512.")
else ()
    message(STATUS "System does not support AVX-512.")
endif ()

##################### PMDK ####################
option(BUILD_PMDK "Set true if PMDK should be downloaded and built from source.")
option(PMDK_INCLUDE_PATH "Path to custom PMDK include files" "")
option(PMDK_LIBRARY_PATH "Path to custom PMDK library" "")

if (${BUILD_PMDK})
    message(STATUS "PMDK not provided. Downloading and building from source.")
    set(PMDK_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/pmdk")
    ExternalProject_Add(
            PMDK

            GIT_REPOSITORY https://github.com/pmem/pmdk.git
            GIT_TAG 1.10
            BUILD_IN_SOURCE 1
            BUILD_COMMAND $(MAKE)
            PREFIX ${PMDK_PREFIX}
            CONFIGURE_COMMAND ""
            INSTALL_COMMAND ""
            LOG_DOWNLOAD ON
            LOG_CONFIGURE ON
            LOG_BUILD ON
            UPDATE_DISCONNECTED 1
    )

    include_directories(${PMDK_PREFIX}/src/PMDK/src/include)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        link_directories(${PMDK_PREFIX}/src/PMDK/src/debug)
    else ()
        link_directories(${PMDK_PREFIX}/src/PMDK/src/nondebug)
    endif ()
else ()
    set(CMAKE_INCLUDE_PATH ${PMDK_INCLUDE_PATH} /usr/local/include)
    set(PMDK_HINTS ${PMDK_LIBRARY_PATH} /usr/local/lib64 /usr/local/lib)

    find_path(PMEM_INCLUDE_DIRS libpmem.h REQUIRED)
    find_library(PMEM_LIBRARIES NAMES pmem libpmem REQUIRED HINTS ${PMDK_HINTS})

    if (NOT PMEM_INCLUDE_DIRS OR "${PMEM_INCLUDE_DIRS}" STREQUAL "")
        message(FATAL_ERROR "ERROR: libpmem include directory not found.")
    endif ()
    if (NOT PMEM_LIBRARIES OR "${PMEM_LIBRARIES}" STREQUAL "")
        message(FATAL_ERROR "ERROR: libpmem library not found.")
    endif ()

    include_directories(${LIBPMEM_INCLUDE_DIRS})
    message(STATUS "PMDK provided. Including ${PMEM_INCLUDE_DIRS} and linking ${PMEM_LIBRARIES}.")
endif ()

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
set(JSON_VERSION v3.9.1)
set(JSON_DOWNLOAD_PATH https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/json.hpp)
set(JSON_DIR ${CMAKE_CURRENT_BINARY_DIR}/json)
set(JSON_INCLUDE_PATH ${JSON_DIR}/json.hpp)
file(DOWNLOAD ${JSON_DOWNLOAD_PATH} ${JSON_INCLUDE_PATH})
include_directories(${JSON_DIR})

################### Hdr Histogram ####################
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
        GIT_TAG v1.8.1
)
FetchContent_MakeAvailable(spdlog)

################### CLI11 ####################
set(CLI11_VERSION v1.9.1)
set(CLI11_DOWNLOAD_PATH https://github.com/CLIUtils/CLI11/releases/download/${CLI11_VERSION}/CLI11.hpp)
set(CLI11_DIR ${CMAKE_CURRENT_BINARY_DIR}/cli11)
set(CLI11_INCLUDE_PATH ${CLI11_DIR}/CLI11.hpp)
file(DOWNLOAD ${CLI11_DOWNLOAD_PATH} ${CLI11_INCLUDE_PATH})
include_directories(${CLI11_DIR})

##################### PerMA ####################
add_custom_target(copy_configs
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/configs ${CMAKE_CURRENT_BINARY_DIR}/configs)

include_directories(src)
add_subdirectory(src)

##################### Test ####################
option(BUILD_TEST "Set true if tests should be built and run.")

if (${BUILD_TEST})
    enable_testing()
    add_subdirectory(test)
    message(STATUS "Test are included in this build.")
endif ()
