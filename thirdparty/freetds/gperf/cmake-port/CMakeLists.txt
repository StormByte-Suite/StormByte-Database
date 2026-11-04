cmake_minimum_required(VERSION 3.28)
project(gperf VERSION 3.3 LANGUAGES C CXX)

include(CheckIncludeFile)
include(CheckSymbolExists)
include(GNUInstallDirs)

set(GPERF_SOURCE_DIR "${BUILDMASTER_BINDIR}/files/gperf/gperf-3.3")
if(NOT EXISTS "${GPERF_SOURCE_DIR}/src/main.cc")
	message(FATAL_ERROR "GNU gperf 3.3 source is missing under ${GPERF_SOURCE_DIR}")
endif()

check_include_file("sys/cdefs.h" HAVE_SYS_CDEFS_H)
check_include_file("stdbool.h" HAVE_STDBOOL_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_symbol_exists(memset_explicit "string.h" HAVE_MEMSET_EXPLICIT)
if(NOT HAVE_SYS_CDEFS_H)
	set(HAVE_SYS_CDEFS_H 0)
endif()

set(GNULIB_XALLOC_DIE 1)
set(HAVE_INTTYPES_H 1)
set(HAVE_STDINT_H 1)
set(HAVE_STDIO_H 1)
set(HAVE_STDLIB_H 1)
set(HAVE_STRING_H 1)
set(HAVE_SYS_TYPES_H 1)
set(PACKAGE_NAME "\"GNU gperf\"")
set(PACKAGE_VERSION "\"${PROJECT_VERSION}\"")
set(__GETOPT_PREFIX gperf_)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
file(READ "${GPERF_SOURCE_DIR}/lib/config.h.in" _config)
string(REGEX REPLACE "\n#undef ([A-Za-z_][A-Za-z0-9_]*)"
	"\n#cmakedefine \\1 @\\1@" _config "${_config}")
string(CONFIGURE "${_config}" _config @ONLY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
	"#ifndef GPERF_PORT_CONFIG_H\n#define GPERF_PORT_CONFIG_H\n${_config}\n")
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/include/config.h" [=[
#include <arg-nonnull.h>
#if !HAVE_MEMSET_EXPLICIT
# include <stddef.h>
# ifdef __cplusplus
extern "C" {
# endif
void *memset_explicit(void *, int, size_t);
# ifdef __cplusplus
}
# endif
#endif
#endif
]=])

# Use GNU's option parser on every platform, with private symbol names.
set(GUARD_PREFIX GPERF_PORT)
set(HAVE_GETOPT_H 0)
set(INCLUDE_NEXT include)
set(NEXT_GETOPT_H "<getopt.h>")
set(PRAGMA_SYSTEM_HEADER "")
set(PRAGMA_COLUMNS "")
configure_file("${GPERF_SOURCE_DIR}/lib/getopt.in.h"
	"${CMAKE_CURRENT_BINARY_DIR}/include/getopt.h" @ONLY)
configure_file("${GPERF_SOURCE_DIR}/lib/getopt-cdefs.in.h"
	"${CMAKE_CURRENT_BINARY_DIR}/include/getopt-cdefs.h" @ONLY)

set(_read_file "${GPERF_SOURCE_DIR}/lib/read-file.c")
if(WIN32)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/windows/read-file.c.in"
		"${CMAKE_CURRENT_BINARY_DIR}/read-file.c" @ONLY)
	set(_read_file "${CMAKE_CURRENT_BINARY_DIR}/read-file.c")
endif()

add_executable(gperf
	"${GPERF_SOURCE_DIR}/src/arraylist.cc"
	"${GPERF_SOURCE_DIR}/src/bool-array.cc"
	"${GPERF_SOURCE_DIR}/src/hash-table.cc"
	"${GPERF_SOURCE_DIR}/src/input.cc"
	"${GPERF_SOURCE_DIR}/src/keyword-list.cc"
	"${GPERF_SOURCE_DIR}/src/keyword.cc"
	"${GPERF_SOURCE_DIR}/src/main.cc"
	"${GPERF_SOURCE_DIR}/src/options.cc"
	"${GPERF_SOURCE_DIR}/src/output.cc"
	"${GPERF_SOURCE_DIR}/src/positions.cc"
	"${GPERF_SOURCE_DIR}/src/search.cc"
	"${GPERF_SOURCE_DIR}/src/version.cc"
	"${GPERF_SOURCE_DIR}/lib/getopt.c"
	"${GPERF_SOURCE_DIR}/lib/getopt1.c"
	"${GPERF_SOURCE_DIR}/lib/gl_hash_map.c"
	"${GPERF_SOURCE_DIR}/lib/gl_map.c"
	"${GPERF_SOURCE_DIR}/lib/gl_xmap.c"
	"${GPERF_SOURCE_DIR}/lib/hash.cc"
	"${GPERF_SOURCE_DIR}/lib/xsize.c"
	"${_read_file}"
	"${CMAKE_CURRENT_SOURCE_DIR}/xalloc.c"
)
if(NOT HAVE_MEMSET_EXPLICIT)
	target_sources(gperf PRIVATE "${GPERF_SOURCE_DIR}/lib/memset_explicit.c")
endif()
target_include_directories(gperf PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}/include"
	"${GPERF_SOURCE_DIR}/lib"
	"${GPERF_SOURCE_DIR}/src"
)
if(NOT HAVE_UNISTD_H)
	target_include_directories(gperf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/windows")
endif()
if(MSVC)
	target_compile_options(gperf PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")
endif()
set_target_properties(gperf PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

install(TARGETS gperf RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
