if(WITH_MSSQL STREQUAL "SYSTEM")
	find_path(FREETDS_INCLUDE_DIR NAMES sybdb.h PATH_SUFFIXES freetds REQUIRED)
	find_library(FREETDS_DBLIB_LIBRARY NAMES sybdb REQUIRED)
	add_library(FreeTDS-DBLib INTERFACE IMPORTED GLOBAL)
	target_include_directories(FreeTDS-DBLib SYSTEM INTERFACE "${FREETDS_INCLUDE_DIR}")
	target_link_libraries(FreeTDS-DBLib INTERFACE "${FREETDS_DBLIB_LIBRARY}")
	buildmaster_link(StormByte-Database FreeTDS-DBLib)
elseif(WITH_MSSQL STREQUAL "BUNDLED")
	set(FREETDS_OPTIONS
		WITH_OPENSSL=ON
		ENABLE_KRB5=OFF
		ENABLE_EXTRA_CHECKS=OFF
		ENABLE_MSDBLIB=ON
		FREETDS_BUILD_SHARED=OFF
		FREETDS_BUILD_CTLIB=OFF
		FREETDS_BUILD_ODBC=OFF
		FREETDS_BUILD_TOOLS=OFF
		FREETDS_BUILD_SERVER=OFF
		FREETDS_BUILD_POOL=OFF
		FREETDS_BUILD_TESTS=OFF
		${OPENSSL_BACKEND_OPTIONS}
	)
	find_program(GPERF_SYSTEM_EXECUTABLE NAMES gperf)
	if(GPERF_SYSTEM_EXECUTABLE)
		message(STATUS "Using system GNU gperf: ${GPERF_SYSTEM_EXECUTABLE}")
		set(_build_gperf OFF)
	else()
		message(STATUS "System GNU gperf not found; building the pinned gperf submodule")
		add_subdirectory(gperf)
		set(_build_gperf ON)
	endif()
	if(WIN32)
		set(FREETDS_LINK "LINK={tds;replacements;tdsutils;ws2_32;crypt32}")
	elseif(APPLE)
		set(FREETDS_LINK "LINK={tds;replacements;tdsutils;iconv}")
	else()
		set(FREETDS_LINK "LINK={tds;replacements;tdsutils}")
	endif()

	buildmaster_component(
		FreeTDS-DBLib
		"FreeTDS DB-Library"
		"${CMAKE_CURRENT_LIST_DIR}/src"
		"${FREETDS_OPTIONS}"
		static
		"db-lib"
		"ALIAS=FreeTDS::DBLib;${FREETDS_LINK};GIT={RESET;PATCH={0001-Put-options-to-only-consume-client-library.patch;0002-Add-static-linkage.patch}}"
	)
	if(_build_gperf)
		buildmaster_depend(FreeTDS-DBLib gperf)
	endif()
	buildmaster_link(FreeTDS-DBLib OpenSSL)
	buildmaster_link(StormByte-Database FreeTDS-DBLib)
elseif(NOT WITH_MSSQL STREQUAL "OFF")
	message(FATAL_ERROR "WITH_MSSQL must be OFF, SYSTEM, or BUNDLED")
endif()