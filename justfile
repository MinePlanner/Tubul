num_cpus := if os() == "windows" {
    env_var("NUMBER_OF_PROCESSORS")
} else if os() == "macos" {
    `sysctl -n hw.ncpu`
} else {
    `nproc`
}

# Use the Ninja generator when it is available, otherwise fall back to the default.
generator := if `command -v ninja > /dev/null 2>&1 && echo true || echo false` == "true" { "-GNinja" } else { "" }

alias b := build

[private]
list:
	@just --list

# Build tubul. config is Debug or Release; target defaults to everything; extra args are passed to cmake configure.
build config="Release" target="ALL" *extra_args="":
	cmake {{generator}} -DCMAKE_BUILD_TYPE={{config}} {{extra_args}} -S . -B build/{{config}}
	cmake --build build/{{config}} --config {{config}} {{ if target == "ALL" { "" } else { "--target " + target } }} -j {{num_cpus}}
	cmake -E copy_if_different build/{{config}}/compile_commands.json compile_commands.json

# Build in Debug
debug: (build "Debug")

# Build in Release
release: (build "Release")

# Run tubul tests (built in Debug)
test: (build "Debug")
	./bin/Debug/testtubul

# Clean build files
clean:
	@rm -rf bin build lib compile_commands.json
