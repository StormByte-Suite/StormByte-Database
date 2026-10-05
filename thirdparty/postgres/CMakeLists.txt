if(NOT WITH_POSTGRES STREQUAL "OFF")
	if(WITH_POSTGRES STREQUAL "SYSTEM")
		find_package(PostgreSQL REQUIRED QUIET)
		message(STATUS "Using system PostgreSQL: ${PostgreSQL_VERSION}")
		add_library(PostgreSQL INTERFACE IMPORTED GLOBAL)
		target_link_libraries(PostgreSQL INTERFACE PostgreSQL::PostgreSQL)
	else()
		set(POSTGRES_OPTIONS
			docs=disabled
			docs_pdf=disabled
			llvm=disabled
			nls=disabled
			readline=disabled
			libxml=disabled
			libxslt=disabled
			icu=disabled
			zlib=disabled
			lz4=disabled
			zstd=disabled
			bonjour=disabled
			pam=disabled
			gssapi=disabled
			bsd_auth=disabled
			selinux=disabled
			systemd=disabled
			libnuma=disabled
			liburing=disabled
			plperl=disabled
			plpython=disabled
			pltcl=disabled
			uuid=none
			ssl=openssl
			ldap=disabled
			libcurl=disabled
			dtrace=disabled
			tap_tests=disabled
			cassert=false
		)
		if(WITH_OPENSSL STREQUAL "BUNDLED")
			list(APPEND POSTGRES_OPTIONS "pkg_config_path=${BUILDMASTER_INSTALL_LIBDIR}/pkgconfig")
		endif()

		if(WIN32)
			# Meson reads openssl.pc without --static, so static OpenSSL's Winsock/CryptoAPI deps are dropped.
			set(_pg_optstr "TOOLCHAIN=msvc;ALIAS=PostgreSQL::PostgreSQL;LINKFLAGS={ws2_32.lib;crypt32.lib}")
		else()
			set(_pg_optstr "ALIAS=PostgreSQL::PostgreSQL")
		endif()

		buildmaster_component(
			PostgreSQL
			"PostgreSQL"
			"${CMAKE_CURRENT_LIST_DIR}/src"
			"${POSTGRES_OPTIONS}"
			static
			"pq;pgport;pgport_shlib;pgcommon;pgcommon_shlib"
			"${_pg_optstr}"
		)
		buildmaster_link(PostgreSQL OpenSSL)
	endif()

	buildmaster_link(StormByte-Database PostgreSQL)
endif()
