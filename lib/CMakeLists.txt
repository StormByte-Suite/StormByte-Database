include(GNUInstallDirs)

foreach(_backend IN ITEMS POSTGRES MARIADB SQLITE MSSQL)
	if(WITH_${_backend} STREQUAL "OFF")
		set(STORMBYTE_DATABASE_HAS_${_backend} false)
	else()
		set(STORMBYTE_DATABASE_HAS_${_backend} true)
	endif()
endforeach()
configure_file(
	"${CMAKE_CURRENT_LIST_DIR}/public/StormByte/database/backend.hxx.in"
	"${CMAKE_CURRENT_BINARY_DIR}/generated/StormByte/database/backend.hxx"
	@ONLY
)

file(GLOB_RECURSE STORMBYTE_DATABASE_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/public/*.cxx"
)

if(BUILD_SHARED_LIBS)
	set(_mode shared)
else()
	set(_mode static)
endif()

buildmaster_component(
	StormByte-Database
	"StormByte Suite - Database"
	"${STORMBYTE_DATABASE_SOURCES}"
	""
	${_mode}
	"StormByte-Database"
	"BACKEND=host;ALIAS=StormByte::Database"
)

if(BUILD_SHARED_LIBS)
	target_compile_definitions(StormByte-Database
		PRIVATE StormByte_Database_EXPORTS
		PUBLIC STORMBYTE_DATABASE_SHARED
	)
endif()

set_target_properties(StormByte-Database PROPERTIES
	LINKER_LANGUAGE CXX
	SOVERSION ${CMAKE_PROJECT_VERSION}
	VERSION ${CMAKE_PROJECT_VERSION}
	POSITION_INDEPENDENT_CODE ON
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON
)

if(NOT WIN32 AND ENABLE_ASAN)
	target_compile_options(StormByte-Database PUBLIC ${SANITIZER_FLAGS} -g -fno-omit-frame-pointer)
	target_link_options(StormByte-Database PUBLIC ${SANITIZER_FLAGS})
endif()

if(NOT WITH_SQLITE STREQUAL "OFF")
	file(GLOB_RECURSE STORMBYTE_DATABASE_SQLITE_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_LIST_DIR}/optional/StormByte/database/sqlite/*.cxx"
		"${CMAKE_CURRENT_LIST_DIR}/private/StormByte/database/sqlite/*.cxx"
	)
	target_sources(StormByte-Database PRIVATE
		${STORMBYTE_DATABASE_SQLITE_SOURCES}
	)
endif()

if(NOT WITH_POSTGRES STREQUAL "OFF")
	file(GLOB_RECURSE STORMBYTE_DATABASE_POSTGRES_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_LIST_DIR}/optional/StormByte/database/postgres/*.cxx"
		"${CMAKE_CURRENT_LIST_DIR}/private/StormByte/database/postgres/*.cxx"
	)
	target_sources(StormByte-Database PRIVATE
		${STORMBYTE_DATABASE_POSTGRES_SOURCES}
	)
endif()

if(NOT WITH_MARIADB STREQUAL "OFF")
	file(GLOB_RECURSE STORMBYTE_DATABASE_MARIADB_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_LIST_DIR}/optional/StormByte/database/mariadb/*.cxx"
		"${CMAKE_CURRENT_LIST_DIR}/private/StormByte/database/mariadb/*.cxx"
	)
	target_sources(StormByte-Database PRIVATE
		${STORMBYTE_DATABASE_MARIADB_SOURCES}
	)
	if(WITH_MARIADB STREQUAL "BUNDLED")
		target_include_directories(StormByte-Database SYSTEM PRIVATE
			"${BUILDMASTER_INSTALL_INCLUDEDIR}/mariadb"
		)
	endif()
endif()

if(NOT WITH_MSSQL STREQUAL "OFF")
	file(GLOB_RECURSE STORMBYTE_DATABASE_MSSQL_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_LIST_DIR}/optional/StormByte/database/mssql/*.cxx"
		"${CMAKE_CURRENT_LIST_DIR}/private/StormByte/database/mssql/*.cxx"
	)
	target_sources(StormByte-Database PRIVATE
		${STORMBYTE_DATABASE_MSSQL_SOURCES}
	)
	target_include_directories(StormByte-Database SYSTEM PRIVATE
		"${BUILDMASTER_INSTALL_INCLUDEDIR}"
	)
endif()

if(MSVC)
	target_compile_options(StormByte-Database PRIVATE
		/EHsc
		$<$<CONFIG:Release>:/O2>
		$<$<CONFIG:Debug>:/Od>
	)
	target_compile_definitions(StormByte-Database PUBLIC UNICODE NOMINMAX)
else()
	target_compile_options(StormByte-Database PRIVATE
		$<$<CONFIG:Debug>:-pipe -g -ggdb -Wall -Wextra -Wnon-virtual-dtor -pedantic -pedantic-errors -O0>
	)
endif()

target_include_directories(StormByte-Database
	SYSTEM BEFORE PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
		"${CMAKE_CURRENT_LIST_DIR}/public"
		"${CMAKE_CURRENT_LIST_DIR}/private"
		"${CMAKE_CURRENT_LIST_DIR}/optional"
)
