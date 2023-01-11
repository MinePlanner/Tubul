project(libtubul)

file(GLOB tubul_src "*.cpp")
add_library(libtubul STATIC ${tubul_src})
