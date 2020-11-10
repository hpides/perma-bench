cmake_minimum_required(VERSION 3.16)
project(perma-bench)

include(ExternalProject)
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
if (HAS_AVX)
    # Set HAS_AVX so we can use it to enable all explicit AVX instructions.
    add_definitions("-DHAS_AVX")
    message(STATUS "System supports AVX-512.")
else ()
    message(STATUS "System does not support AVX-512.")
endif ()

##################### PMDK ####################
set(PMDK_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/pmdk")
ExternalProject_Add(
        PMDK

        GIT_REPOSITORY https://github.com/pmem/pmdk.git
        GIT_TAG 1.9
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

##################### YAML ####################
set(YAML_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/yaml")
ExternalProject_Add(
        YAML

        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.6.3
        BUILD_IN_SOURCE 1
        PREFIX ${YAML_PREFIX}
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_BUILD ON
        UPDATE_DISCONNECTED 1
)

include_directories(${YAML_PREFIX}/src/YAML/include)
link_directories(${YAML_PREFIX}/src/YAML)

##################### JSON ####################
set(JSON_VERSION v3.9.1)
set(JSON_DOWNLOAD_PATH https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/json.hpp)
set(JSON_DIR ${CMAKE_CURRENT_BINARY_DIR}/json)
set(JSON_INCLUDE_PATH ${JSON_DIR}/json.hpp)
file(DOWNLOAD ${JSON_DOWNLOAD_PATH} ${JSON_INCLUDE_PATH})
include_directories(${JSON_DIR})


##################### PerMA ####################
file(COPY configs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(src)
