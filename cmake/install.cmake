install(TARGETS StormByte-Database
	ARCHIVE 		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY 		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME 		DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/public/StormByte/"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte"
	FILES_MATCHING
	PATTERN "*.h"
	PATTERN "*.hxx"
)
if (NOT WITH_SQLITE STREQUAL "OFF")
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/optional/StormByte/database/sqlite"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte/database"
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hxx"
	)
endif()
if (NOT WITH_POSTGRES STREQUAL "OFF")
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/optional/StormByte/database/postgres"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte/database"
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hxx"
	)
endif()
if (NOT WITH_MARIADB STREQUAL "OFF")
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/optional/StormByte/database/mariadb"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte/database"
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hxx"
	)
endif()
if (NOT WITH_MSSQL STREQUAL "OFF")
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/optional/StormByte/database/mssql"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte/database"
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hxx"
	)
endif()