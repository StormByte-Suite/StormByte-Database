option(ENABLE_TEST "Enable Unit Tests" OFF)
if(ENABLE_TEST)
	enable_testing()

	add_test(NAME OptimizationTests
		COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/optimizations_test.cmake")
	set_tests_properties(OptimizationTests PROPERTIES TIMEOUT 15)

	add_executable(DummyTests dummy_test.cxx)
	target_link_libraries(DummyTests StormByte::Database)
	foreach(_backend IN ITEMS POSTGRES MARIADB SQLITE MSSQL)
		target_compile_definitions(DummyTests PRIVATE
			"STORMBYTE_TEST_WITH_${_backend}=$<NOT:$<STREQUAL:${WITH_${_backend}},OFF>>"
		)
	endforeach()
	add_test(NAME DummyTests COMMAND DummyTests)

	if(TARGET gperf AND NOT CMAKE_CROSSCOMPILING)
		set(_gperf_executable "${BUILDMASTER_INSTALL_BINDIR}/gperf${CMAKE_EXECUTABLE_SUFFIX}")
		set(_gperf_output "${CMAKE_CURRENT_BINARY_DIR}/gperf_smoke_generated.h")
		add_custom_command(
			OUTPUT "${_gperf_output}"
			COMMAND "${_gperf_executable}" "--output-file=${_gperf_output}"
				"${CMAKE_CURRENT_SOURCE_DIR}/gperf_smoke.gperf"
			DEPENDS gperf_install "${CMAKE_CURRENT_SOURCE_DIR}/gperf_smoke.gperf"
			VERBATIM
		)
		add_executable(BundledGperfTests gperf_smoke_test.c "${_gperf_output}")
		target_include_directories(BundledGperfTests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
		add_test(NAME BundledGperfTests COMMAND BundledGperfTests)
		add_test(NAME BundledGperfVersion COMMAND "${_gperf_executable}" --version)
		set_tests_properties(BundledGperfVersion PROPERTIES
			PASS_REGULAR_EXPRESSION "GNU gperf 3[.]3")
	endif()
	
	if (NOT WITH_SQLITE STREQUAL "OFF")
		add_executable(SQLiteTests sqlite_test.cxx)
		target_link_libraries(SQLiteTests StormByte::Database)
		add_test(NAME SQLiteTests COMMAND SQLiteTests)
	endif()

	if (NOT WITH_POSTGRES STREQUAL "OFF")
		add_executable(PostgresTests postgres_test.cxx)
		target_link_libraries(PostgresTests StormByte::Database)
		add_test(NAME PostgresTests COMMAND PostgresTests)
	endif()

	if (NOT WITH_MARIADB STREQUAL "OFF")
		add_executable(MariaDBTests mariadb_test.cxx)
		target_link_libraries(MariaDBTests StormByte::Database)
		add_test(NAME MariaDBTests COMMAND MariaDBTests)
	endif()

	if (NOT WITH_MSSQL STREQUAL "OFF")
		add_executable(MSSQLTests mssql_test.cxx)
		target_link_libraries(MSSQLTests StormByte::Database)
		add_test(NAME MSSQLTests COMMAND MSSQLTests)
		set_tests_properties(MSSQLTests PROPERTIES SKIP_RETURN_CODE 77)
	endif()

	if(BUILD_SHARED_LIBS)
		set(_smoke_system_lib_dir "")
		set(_smoke_system_include_dir "")
		if(TARGET StormByte-Logger)
			get_target_property(_smoke_logger_imported StormByte-Logger IMPORTED)
			if(_smoke_logger_imported)
				get_target_property(_smoke_logger_loc StormByte-Logger IMPORTED_LOCATION)
				if(_smoke_logger_loc)
					get_filename_component(_smoke_system_lib_dir "${_smoke_logger_loc}" DIRECTORY)
				endif()
				get_target_property(_smoke_logger_inc StormByte-Logger INTERFACE_INCLUDE_DIRECTORIES)
				if(_smoke_logger_inc)
					list(GET _smoke_logger_inc 0 _smoke_system_include_dir)
				endif()
			endif()
		endif()
		add_test(NAME InstalledConsumerSmoke
			COMMAND "${CMAKE_COMMAND}"
				"-DDATABASE_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
				"-DDATABASE_BINARY_DIR=${PROJECT_BINARY_DIR}"
				"-DDATABASE_GENERATOR=${CMAKE_GENERATOR}"
				"-DDATABASE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
				"-DDATABASE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
				"-DDATABASE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
				"-DDATABASE_ENABLE_SQLITE=$<BOOL:$<NOT:$<STREQUAL:${WITH_SQLITE},OFF>>>"
				"-DDATABASE_OPENSSL_MODE=${WITH_OPENSSL}"
				"-DDATABASE_BUNDLED_TLS_CLIENT=$<OR:$<STREQUAL:${WITH_POSTGRES},BUNDLED>,$<STREQUAL:${WITH_MARIADB},BUNDLED>,$<STREQUAL:${WITH_MSSQL},BUNDLED>>"
				"-DDATABASE_OPENSSL_SSL_LIBRARY=${OPENSSL_SSL_LIBRARY}"
				"-DDATABASE_OPENSSL_CRYPTO_LIBRARY=${OPENSSL_CRYPTO_LIBRARY}"
				"-DDATABASE_BUILD_CONFIG=$<CONFIG>"
				"-DDATABASE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
				"-DDATABASE_EXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
				"-DDATABASE_SYSTEM_LIBRARY_DIR=${_smoke_system_lib_dir}"
				"-DDATABASE_SYSTEM_INCLUDE_DIR=${_smoke_system_include_dir}"
				"-DSMOKE_ROOT=${PROJECT_BINARY_DIR}/test/install-consumer-smoke"
				-P "${CMAKE_CURRENT_LIST_DIR}/install_consumer_smoke.cmake"
		)
	endif()

	if(WITH_STORMBYTE STREQUAL "BUNDLED" AND NOT CMAKE_CROSSCOMPILING)
		set(_package_cxx_flags "${CMAKE_CXX_FLAGS}")
		if(COMMAND _pgo_strip_profile_flags)
			_pgo_strip_profile_flags("${_package_cxx_flags}" _package_cxx_flags)
		endif()

		add_test(NAME PackageDiscoveryTests
			COMMAND "${CMAKE_COMMAND}"
				"-DDATABASE_BUILD_DIR=${PROJECT_BINARY_DIR}"
				"-DDATABASE_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
				"-DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package_consumer"
				"-DDEPENDENCY_PREFIX=${BUILDMASTER_INSTALL_DIR}"
				"-DDATABASE_ENABLE_SQLITE=$<BOOL:$<NOT:$<STREQUAL:${WITH_SQLITE},OFF>>>"
				"-DBUILD_CONFIG=$<CONFIG>"
				"-DGENERATOR=${CMAKE_GENERATOR}"
				"-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
				"-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
				"-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
				"-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
				"-DCXX_COMPILER_TARGET=${CMAKE_CXX_COMPILER_TARGET}"
				"-DCXX_FLAGS=${_package_cxx_flags}"
				"-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
				"-DPACKAGE_TEST_ROOT=${PROJECT_BINARY_DIR}/test/package-discovery"
				-P "${CMAKE_CURRENT_SOURCE_DIR}/package_discovery_test.cmake")
		set_tests_properties(PackageDiscoveryTests PROPERTIES TIMEOUT 120)
	endif()
endif()
