
project(tubultest)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY git@github.com:google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(testtubul test_setup.cpp)
target_link_libraries(testtubul GTest::gtest_main)
target_link_libraries(testtubul libtubul)
target_include_directories(testtubul PRIVATE libtubul)
gtest_discover_tests(testtubul)
