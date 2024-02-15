project(example1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(example1 example1.cpp)
target_link_libraries(example1 libtubul Threads::Threads)
