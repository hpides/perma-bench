cmake_minimum_required(VERSION 3.16)
project(nvmbm)

include(ExternalProject)
set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message("-- Release mode, all optimizations enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Ofast")
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

add_subdirectory(src)