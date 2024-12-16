message(STATUS "Scanning external dependencies")

# Here we should simply add the folders containing external dependencies.
add_subdirectory( argparse )
add_subdirectory( fast_float )
add_subdirectory( text_format_utils )
add_subdirectory( better_enums )

#Creating a single list to contain all libraries that are required from External
#so it's easier to use them later.
set(TUBUL_EXTERNAL_LIBS_INCLUDES "${ARGPARSE_INCLUDE_DIRS}" "${FAST_FLOAT_INCLUDE_DIRS}" "${TEXT_FORMAT_UTILS_INCLUDE_DIRS}" "${BETTER_ENUMS_INCLUDE_DIRS}" CACHE INTERNAL "Set external includes" FORCE )
