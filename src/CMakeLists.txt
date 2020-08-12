set(
        SOURCES

        benchmark.cpp
        benchmark.hpp
        benchmark_factory.cpp
        benchmark_factory.hpp
        benchmark_suite.cpp
        benchmark_suite.hpp
        config_parser.cpp
        config_parser.hpp
        io_operation.cpp
        io_operation.hpp
)

add_executable(nvmbm ../nvmbm.cpp ${SOURCES})
add_dependencies(nvmbm PMDK)
target_link_libraries(nvmbm pmem)