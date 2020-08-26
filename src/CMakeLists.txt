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
        utils.cpp
        utils.hpp
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")
add_executable(nvmbm nvmbm.cpp ${SOURCES})
add_dependencies(nvmbm PMDK yaml-cpp)
target_link_libraries(nvmbm pmem yaml-cpp)
