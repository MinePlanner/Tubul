message(STATUS "Scanning external dependencies")

# Here we should simply add the folders containing external dependencies.
add_subdirectory( argparse )
add_subdirectory( fast_float )
add_subdirectory( rapidcsv )
add_subdirectory( neoparams )
