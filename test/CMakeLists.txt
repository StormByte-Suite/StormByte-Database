option(ENABLE_TEST "Enable Unit Tests" OFF)
if(ENABLE_TEST)
	enable_testing()

	add_executable(DummyTests dummy_test.cxx)
	target_link_libraries(DummyTests StormByte::Database)
	add_test(NAME DummyTests COMMAND DummyTests)
	
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
				"-DSMOKE_ROOT=${PROJECT_BINARY_DIR}/test/install-consumer-smoke"
				-P "${CMAKE_CURRENT_LIST_DIR}/install_consumer_smoke.cmake"
		)
	endif()
endif()
