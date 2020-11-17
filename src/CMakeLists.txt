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

add_library(libperma STATIC ${SOURCES})
add_dependencies(libperma YAML)
target_link_libraries(libperma yaml-cpp)

add_executable(perma-bench perma.cpp ${SOURCES})

target_link_libraries(perma-bench libperma)

if (${BUILD_PMDK})
    add_dependencies(perma-bench PMDK)
    target_link_libraries(perma-bench pmem)
else ()
    target_link_libraries(perma-bench ${PMEM_LIBRARIES})
endif ()
