project(libtubul)

file(GLOB tubul_src "*.cpp")
add_library(libtubul STATIC ${tubul_src})

#Tubul requires the following dependencies.
target_include_directories(libtubul PRIVATE ${ARGPARSE_INCLUDE_DIRS})
