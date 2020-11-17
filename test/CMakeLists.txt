include(FetchContent)

##################### GoogleTest ####################

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(TESTS)

add_executable(perma-test perma_test.cpp)

target_link_libraries(perma-test gtest gtest_main)

add_test(perma-test perma-test)
