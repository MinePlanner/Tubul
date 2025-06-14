cmake_minimum_required(VERSION 3.22)

# ad-hoc setup for mansci's linux
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if("$ENV{HOSTNAME}" STREQUAL "mansci.public")
    SET(CMAKE_CXX_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/g++")
    SET(CMAKE_C_COMPILER "/opt/rh/devtoolset-11/root/usr/bin/gcc")
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#On Clang, the sized deallocation is not the default behavior!!!!
#You have to actively ask for it.
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")
endif()

project(tubul)

#Tubul-wide detection of target architecture.
if(APPLE)
  set(TUBUL_COMPILE_DEFS "TUBUL_MACOS")
elseif(WIN32)
  set(TUBUL_COMPILE_DEFS "TUBUL_WINDOWS")
elseif(UNIX) # We should move to CMake 3.22+ where we have LINUX direct check
  set(TUBUL_COMPILE_DEFS "TUBUL_LINUX")
endif()

# Ease of use: Check if we got a CMAKE_BUILD_TYPE, and if not, default to Release instead
# of panicking
if (DEFINED CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE)
    message(STATUS "Compiling in '${CMAKE_BUILD_TYPE}' mode!")
else()
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "Compiling in '${CMAKE_BUILD_TYPE}' mode! (default mode)")
endif()

# Setup location of the generated files, binaries and libraries.
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

# Colors in errors if possible
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag( -fcolor-diagnostics COLOR_DIAGNOSTICS_FLAG_SUPPORTED)
if (COLOR_DIAGNOSTICS_FLAG_SUPPORTED)
    add_compile_options (-fcolor-diagnostics)
else()
    check_cxx_compiler_flag( -fdiagnostics-color COLOR_DIAGNOSTICS_FLAG_SUPPORTED2)
    if (COLOR_DIAGNOSTICS_FLAG_SUPPORTED2)
        add_compile_options (-fdiagnostics-color)
    endif()
endif()

#Adding external dependencies.
add_subdirectory(External)


add_subdirectory(tubul)

#Check to add tests and apps directories only if we are building tubul
#as the main target. I.e, if someone fetches tubul through Fetch_Content
#only the tubul lib would be compiled, wich is most likely what a user
#would want.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_subdirectory(tests)
  add_subdirectory(apps)
endif()
