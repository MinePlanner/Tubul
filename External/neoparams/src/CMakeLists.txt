add_library(neoparamslib OBJECT neoparams.cpp neoparams_types.cpp)
target_include_directories(neoparamslib PRIVATE ${NEOPARAMS_INCLUDE_DIRS})

