include("${CMAKE_SOURCE_DIR}/cmake/stormbyte.cmake")

set(STORMBYTE_BASE_VERSION "2.1.0")
set(STORMBYTE_LOGGER_VERSION "2.0.2")

if(WIN32)
	set(WITH_STORMBYTE "BUNDLED" CACHE STRING "Use StormByte components (SYSTEM/BUNDLED)" FORCE)
else()
	set(WITH_STORMBYTE "BUNDLED" CACHE STRING "Use StormByte components (SYSTEM/BUNDLED)")
endif()

if(WITH_STORMBYTE STREQUAL "SYSTEM")
	find_package(StormByte REQUIRED Base Logger)
else()
	stormbyte_component_url(STORMBYTE_BASE_URL "${STORMBYTE_BASE_VERSION}")
	stormbyte_component_url(STORMBYTE_LOGGER_URL "${STORMBYTE_LOGGER_VERSION}" "StormByte-Logger")

	if(BUILD_SHARED_LIBS)
		set(STORMBYTE_BASE_MODE shared)
		set(STORMBYTE_LOGGER_MODE shared)
	else()
		set(STORMBYTE_BASE_MODE static)
		set(STORMBYTE_LOGGER_MODE static)
	endif()

	buildmaster_component(
		StormByte-Base
		"StormByte Suite - Base"
		"${STORMBYTE_BASE_URL}"
		""
		"${STORMBYTE_BASE_MODE}"
		"StormByte"
		"BACKEND=binary;ALIAS=StormByte::Base"
	)
	buildmaster_component(
		StormByte-Logger
		"StormByte Suite - Logger"
		"${STORMBYTE_LOGGER_URL}"
		""
		"${STORMBYTE_LOGGER_MODE}"
		"StormByte-Logger"
		"BACKEND=binary;ALIAS=StormByte::Logger"
	)
	buildmaster_link(StormByte-Logger StormByte-Base)

	if(WIN32 AND BUILD_SHARED_LIBS)
		target_compile_definitions(StormByte-Base INTERFACE STORMBYTE_SHARED)
		target_compile_definitions(StormByte-Logger INTERFACE STORMBYTE_LOGGER_SHARED)
	endif()
endif()

buildmaster_link(StormByte-Database StormByte-Logger)
