project(libtubul)

file(GLOB tubul_src "*.cpp")
add_library(libtubul STATIC ${tubul_src} $<TARGET_OBJECTS:neoparamslib> )

#Tubul requires the following dependencies.
target_include_directories(libtubul PRIVATE ${ARGPARSE_INCLUDE_DIRS})
target_include_directories(libtubul PRIVATE ${FAST_FLOAT_INCLUDE_DIRS})
target_include_directories(libtubul PRIVATE ${RAPIDCSV_INCLUDE_DIRS})
target_include_directories(libtubul PRIVATE ${NEOPARAMS_INCLUDE_DIRS})



target_compile_definitions(libtubul PUBLIC ${TUBUL_COMPILE_DEFS})
