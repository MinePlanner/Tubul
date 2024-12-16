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

target_compile_definitions(libtubul PUBLIC ${TUBUL_COMPILE_DEFS})
set_target_properties(libtubul PROPERTIES POSITION_INDEPENDENT_CODE ON)