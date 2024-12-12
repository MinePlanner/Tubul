project(libtubul)

#Grab all source code that must be compiled.
file(GLOB tubul_src "*.cpp")
add_library(libtubul STATIC ${tubul_src} )

#Tubul requires access to external libs so we add them. Also
#note that we add the include dirs as PUBLIC so they can be inherited
#by the user application.
foreach(dir ${TUBUL_EXTERNAL_LIBS_INCLUDES})
    message(STATUS "Adding include dir='${dir}'")
    target_include_directories(libtubul PUBLIC ${dir})
endforeach()

target_include_directories(libtubul PUBLIC ${CMAKE_SOURCE_DIR}/api)
target_compile_definitions(libtubul PUBLIC ${TUBUL_COMPILE_DEFS})
## MSVC complains about very base functions used by third parties, like fopen. This
## compiler definition lowers the rage for those functions
if (MSVC)
    target_compile_definitions(libtubul PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(libtubul PROPERTIES POSITION_INDEPENDENT_CODE ON)

##Adding some options to increase the level of warnings. It's similar to CMP but not so extensive.
if(MSVC)
  target_compile_options(libtubul PRIVATE /W4 /WX)
else()
  target_compile_options(libtubul PRIVATE -Wall -Wextra -Wpedantic -Werror -Wunused-but-set-variable -Wmismatched-tags)
endif()

