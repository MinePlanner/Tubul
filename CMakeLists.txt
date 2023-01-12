cmake_minimum_required(VERSION 3.22)
project(tubul)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/tubul)

add_subdirectory(tubul)
add_subdirectory(tests)
add_subdirectory(apps)


