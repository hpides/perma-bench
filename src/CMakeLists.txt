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

add_library(perma STATIC ${SOURCES})
target_link_libraries(perma yaml-cpp)

if (${BUILD_PMDK})
    add_dependencies(perma PMDK)
    target_link_libraries(perma pmem)
else ()
    target_link_libraries(perma ${PMEM_LIBRARIES})
endif ()

add_executable(perma-bench perma.cpp)
target_link_libraries(perma-bench perma)
