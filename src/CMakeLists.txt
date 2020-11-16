set(
        SOURCES

        benchmark.cpp
        benchmark.hpp
        benchmark_factory.cpp
        benchmark_factory.hpp
        benchmark_suite.cpp
        benchmark_suite.hpp
        io_operation.cpp
        io_operation.hpp
        utils.cpp
        utils.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")
add_executable(perma-bench perma.cpp ${SOURCES})

add_dependencies(perma-bench YAML)
target_link_libraries(perma-bench yaml-cpp)

if (${BUILD_PMDK})
    add_dependencies(perma-bench PMDK)
    target_link_libraries(perma-bench pmem)
else ()
    target_link_libraries(perma-bench ${PMEM_LIBRARIES})
endif ()
