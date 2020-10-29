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
)

include_directories(${YAML_PREFIX}/src/YAML/include)
link_directories(${YAML_PREFIX}/src/YAML)

##################### JSON ####################
set(JSON_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/json")
ExternalProject_Add(
        JSON

        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.9.1
        BUILD_IN_SOURCE 1
        PREFIX ${JSON_PREFIX}
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_BUILD ON
)

include_directories(${JSON_PREFIX}/src/JSON/single_include/nlohmann)

add_subdirectory(src)
