cmake_minimum_required(VERSION 3.22)

# ad-hoc setup for mansci's linux
if("$ENV{HOSTNAME}" STREQUAL "mansci.public")
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

# Setup location of the generated files, binaries and libraries.
message(STATUS "Compiling in ${CMAKE_BUILD_TYPE} mode!")
string( TOLOWER "${CMAKE_BUILD_TYPE}" build_type )
if((build_type MATCHES "debug") OR (build_type MATCHES "release"))
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}") # For the executables and, in case of Windows, the DLLs
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}") # For the static libraries
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}") # For the dynamic libraries
    set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}") # For the symbols in Windows
else()
	message(STATUS "Unsupported Build Type")
endif()

# Check if ccache is installed
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Found ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

#Adding external dependencies.
add_subdirectory(External)

include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/tubul)

add_subdirectory(tubul)
add_subdirectory(tests)
add_subdirectory(apps)
