include(FetchContent)

add_custom_target(copy_test_resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/test/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)

##################### GoogleTest ####################
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(PERMA_TEST_SOURCES
        benchmark_test.cpp
        config_test.cpp
        test_utils.cpp
        test_utils.hpp
        utils_test.cpp)

add_executable(perma-test perma_test.cpp ${PERMA_TEST_SOURCES})
add_dependencies(perma-test copy_test_resources)
target_link_libraries(perma-test gtest perma)
add_test(perma-test perma-test)
