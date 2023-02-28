cmake_minimum_required(VERSION 3.22)

# ad-hoc setup for mansci's linux
if(UNIX AND NOT APPLE)
    SET(CMAKE_CXX_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/g++")
    SET(CMAKE_C_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/gcc")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(tubul)

#Tubul-wide detection of target architecture.
if(APPLE)
    add_compile_definitions(TUBUL_MACOS)
elseif(WIN32)
    add_compile_definitions(TUBUL_WINDOWS)
elseif(UNIX) # We should move to CMake 3.22+ where we have LINUX direct check
    add_compile_definitions(TUBUL_LINUX)
endif()

#Adding external dependencies.
add_subdirectory(External)

include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/tubul)

add_subdirectory(tubul)
add_subdirectory(tests)
add_subdirectory(apps)
