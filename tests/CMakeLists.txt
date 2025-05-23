
project(tubultest)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

file(GLOB tubul_tests_src "*.cpp")

add_executable(testtubul ${tubul_tests_src})
target_link_libraries(testtubul GTest::gtest_main libtubul)

gtest_discover_tests(testtubul)
