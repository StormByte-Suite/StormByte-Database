if(WIN32)
	set(WITH_STORMBYTE "BUNDLED" CACHE STRING "Use StormByte components (SYSTEM/BUNDLED)" FORCE)
else()
	set(WITH_STORMBYTE "BUNDLED" CACHE STRING "Use StormByte components (SYSTEM/BUNDLED)")
endif()

if(WITH_STORMBYTE STREQUAL "SYSTEM")
	find_package(StormByte REQUIRED Base Logger)
else()
	add_subdirectory(base)
	add_subdirectory(logger)
endif()

buildmaster_link(StormByte-Database StormByte-Logger)
