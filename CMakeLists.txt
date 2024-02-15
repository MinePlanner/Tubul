cmake_minimum_required(VERSION 3.22)

# ad-hoc setup for mansci's linux
if("$ENV{HOSTNAME}" STREQUAL "mansci.public")
    SET(CMAKE_CXX_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/g++")
    SET(CMAKE_C_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/gcc")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#On Clang, the sized deallocation is not the default behavior!!!!
#You have to actively ask for it.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")

project(tubul)

#Tubul-wide detection of target architecture.
if(APPLE)
  set(TUBUL_COMPILE_DEFS "TUBUL_MACOS")
elseif(WIN32)
  set(TUBUL_COMPILE_DEFS "TUBUL_WINDOWS")
elseif(UNIX) # We should move to CMake 3.22+ where we have LINUX direct check
  set(TUBUL_COMPILE_DEFS "TUBUL_LINUX")
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

# colors for ninja
if (APPLE)
    add_compile_options (-fcolor-diagnostics)
endif ()

#Adding external dependencies.
add_subdirectory(External)

include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/tubul)

add_subdirectory(tubul)

#Check to add tests and apps directories only if we are building tubul
#as the main target. I.e, if someone fetches tubul through Fetch_Content
#only the tubul lib would be compiled, wich is most likely what a user
#would want.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
add_subdirectory(tests)
add_subdirectory(apps)
endif()
