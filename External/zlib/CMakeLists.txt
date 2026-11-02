project(TUBUL_ZLIB C)

message( "Preparing ${PROJECT_NAME}")

## zlib 1.3.2 (zlib license, see LICENSE), from
## https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
## (sha256 bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16).
## Only its .c and .h files, LICENSE and README are checked in; this file
## replaces upstream's build. Tubul uses it to read gzip-compressed files.
add_library(tubul_zlib STATIC
        adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c
        infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c
)

target_include_directories(tubul_zlib PUBLIC ${PROJECT_SOURCE_DIR})

## Z_PREFIX renames every zlib symbol to z_..., so this copy can't clash with
## another zlib linked into the same program. PUBLIC, so Tubul's code that
## includes zlib.h uses the same names.
target_compile_definitions(tubul_zlib PUBLIC Z_PREFIX)

## As upstream's CMakeLists.txt: zlib needs <unistd.h> where it exists.
include(CheckIncludeFile)
check_include_file(unistd.h TUBUL_ZLIB_HAVE_UNISTD_H)
if (TUBUL_ZLIB_HAVE_UNISTD_H)
    target_compile_definitions(tubul_zlib PUBLIC HAVE_UNISTD_H)
endif()

## Third-party code: no warnings, so Tubul's warning flags can't fail the build.
if (MSVC)
    target_compile_definitions(tubul_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
    target_compile_options(tubul_zlib PRIVATE /w)
else()
    target_compile_options(tubul_zlib PRIVATE -w)
endif()

set_target_properties(tubul_zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
