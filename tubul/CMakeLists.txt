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

#Check the flag for disabling tubul's custom memory tracking.
if (DEFINED TUBUL_NO_MALLOC)
    message(WARNING "Not using tubul's memory tracking")
else()
    target_compile_definitions(libtubul PUBLIC TUBUL_OPERATOR_NEW_OVERRIDE)
endif()

#Setting compile difinitions and include folders for users of the api so
#just adding the link declaration is enough to propagate required flags.
target_compile_definitions(libtubul PUBLIC ${TUBUL_COMPILE_DEFS})
target_include_directories(libtubul INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../api )
target_include_directories(libtubul PUBLIC ${CMAKE_CURRENT_LIST_DIR} )

#Always use PIC so we can create shared libs with tubul.
set_target_properties(libtubul PROPERTIES POSITION_INDEPENDENT_CODE ON)

## Export some easy to use macros to identify release or debug builds
string( TOLOWER "${CMAKE_BUILD_TYPE}" build_type )
if((build_type MATCHES "debug") )
    target_compile_definitions(libtubul PUBLIC "TUBUL_DEBUG")
endif()

if((build_type MATCHES "release") )
    target_compile_definitions(libtubul PUBLIC "TUBUL_RELEASE")
endif()

## MSVC complains about very base functions used by third parties, like fopen. This
## compiler definition lowers the rage for those functions
if (MSVC)
    target_compile_definitions(libtubul PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

##Adding some options to increase the level of warnings. It's similar to CMP but not so extensive.
if(MSVC)
  target_compile_options(libtubul PRIVATE /W4 /WX)
else()
  target_compile_options(libtubul PRIVATE -Wall -Wextra -Wpedantic -Werror -Wunused-but-set-variable -Wmismatched-tags)
endif()

