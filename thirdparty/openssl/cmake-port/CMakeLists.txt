cmake_minimum_required(VERSION 3.28)
project(StormByteBundledOpenSSL LANGUAGES C)

include(GNUInstallDirs)

set(OPENSSL_CONFIGURE_OPTIONS
	"--prefix=${CMAKE_INSTALL_PREFIX}"
	"--libdir=${OPENSSL_LIBDIR}"
	no-ssl3
	no-comp
)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../cmake" openssl-cmake)