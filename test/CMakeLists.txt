include(FetchContent)

##################### GoogleTest ####################

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(PERMA_TEST_SOURCES file_test.cpp)

add_executable(perma-test perma_test.cpp ${PERMA_TEST_SOURCES})

target_link_libraries(perma-test gtest gtest_main libperma)

add_test(perma-test perma-test)
