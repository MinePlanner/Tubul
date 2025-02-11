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


#Tubul uses threads, so we declare the dependency for platforms where
#we need special extra stuff (linux mostly).
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(libtubul PUBLIC Threads::Threads)

if (DEFINED TUBUL_NO_MALLOC)
    message(WARNING "Not using tubul's memory tracking")
else()
    target_compile_definitions(libtubul PUBLIC TUBUL_OPERATOR_NEW_OVERRIDE)
endif()


target_compile_definitions(libtubul PUBLIC ${TUBUL_COMPILE_DEFS})
target_include_directories(libtubul INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../api )
target_include_directories(libtubul PUBLIC ${CMAKE_CURRENT_LIST_DIR} )
set_target_properties(libtubul PROPERTIES POSITION_INDEPENDENT_CODE ON)
