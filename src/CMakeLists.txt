set(
        SOURCES

        benchmark.cpp
        benchmark.hpp
        benchmark_factory.cpp
        benchmark_factory.hpp
        benchmark_suite.cpp
        benchmark_suite.hpp
        io_operation.hpp
        numa.cpp
        numa.hpp
        single_benchmark.cpp
        single_benchmark.hpp
        parallel_benchmark.cpp
        parallel_benchmark.hpp
        utils.cpp
        utils.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

add_library(perma STATIC ${SOURCES})
add_dependencies(perma copy_configs)
target_link_libraries(perma yaml-cpp hdr_histogram_static spdlog)

if (${NUMA_FOUND})
    target_link_libraries(perma ${NUMA_LIBRARIES})
endif ()

add_executable(perma-bench perma.cpp)
target_link_libraries(perma-bench perma)
set_target_properties(perma-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
