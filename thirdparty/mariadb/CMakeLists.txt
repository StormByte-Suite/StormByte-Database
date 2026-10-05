if(NOT WITH_MARIADB STREQUAL "OFF")
	if(WITH_MARIADB STREQUAL "SYSTEM")
		list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
		find_package(MariaDBConnector REQUIRED QUIET)
		message(STATUS "Using system MariaDB Connector C")
		add_library(MariaDB-ConnectorC INTERFACE IMPORTED GLOBAL)
		target_link_libraries(MariaDB-ConnectorC INTERFACE MariaDB::ConnectorC)
	else()
		set(MARIADB_C_OPTIONS
			WITH_MYSQLCOMPAT=OFF
			WITH_SSL=OPENSSL
			${OPENSSL_BACKEND_OPTIONS}
			WITH_UNIT_TESTS=OFF
			ZLIB_BUILD_EXAMPLES=OFF
			INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
			INSTALL_PLUGINDIR=${CMAKE_INSTALL_LIBDIR}/mariadb/plugins
			INSTALL_PCDIR=${CMAKE_INSTALL_LIBDIR}/pkgconfig
		)

		if(MSVC)
			# Plugin DLLs link static OpenSSL via plain library paths, missing its Winsock/CryptoAPI deps.
			set(_mdb_optstr "ALIAS=MariaDB::ConnectorC;LINK={shlwapi;ws2_32;secur32;bcrypt;ncrypt;crypt32};LINKFLAGS={ws2_32.lib;crypt32.lib}")
		else()
			set(_mdb_optstr "ALIAS=MariaDB::ConnectorC")
		endif()

		buildmaster_component(
			MariaDB-ConnectorC
			"MariaDB Connector C"
			"${CMAKE_CURRENT_LIST_DIR}/mariadb-connector-c"
			"${MARIADB_C_OPTIONS}"
			static
			"mariadbclient"
			"${_mdb_optstr}"
		)
		buildmaster_link(MariaDB-ConnectorC OpenSSL)

		target_include_directories(MariaDB-ConnectorC
			SYSTEM INTERFACE
			"${BUILDMASTER_INSTALL_INCLUDEDIR}/mariadb"
		)
	endif()

	buildmaster_link(StormByte-Database MariaDB-ConnectorC)
endif()
