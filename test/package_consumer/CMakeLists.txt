cmake_minimum_required(VERSION 3.28)
project(StormByteDatabasePackageConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(PREPEND CMAKE_MODULE_PATH
	"${DATABASE_PREFIX}/share/cmake/Modules"
	"${DEPENDENCY_PREFIX}/share/cmake/Modules"
)
list(PREPEND CMAKE_PREFIX_PATH "${DATABASE_PREFIX}" "${DEPENDENCY_PREFIX}")
set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)

find_package(StormByte REQUIRED COMPONENTS Base Database)

foreach(path IN ITEMS "${STORMBYTE_Database_INCLUDE_DIR}" "${STORMBYTE_Database_LIBRARY}")
	cmake_path(IS_PREFIX DATABASE_PREFIX "${path}" NORMALIZE _in_database_prefix)
	if(NOT _in_database_prefix)
		message(FATAL_ERROR "Database was not found in ${DATABASE_PREFIX}: ${path}")
	endif()
endforeach()
foreach(path IN ITEMS "${STORMBYTE_LIBRARY}" "${STORMBYTE_Logger_LIBRARY}")
	cmake_path(IS_PREFIX DEPENDENCY_PREFIX "${path}" NORMALIZE _in_dependency_prefix)
	if(NOT _in_dependency_prefix)
		message(FATAL_ERROR "StormByte dependency was not found in ${DEPENDENCY_PREFIX}: ${path}")
	endif()
endforeach()

add_executable(database_package_consumer "${DATABASE_SOURCE_DIR}/test/install_consumer.cxx")
target_link_libraries(database_package_consumer PRIVATE StormByte::Database)
if(DATABASE_ENABLE_SQLITE)
	target_compile_definitions(database_package_consumer PRIVATE STORMBYTE_TEST_SQLITE)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(database_package_consumer PRIVATE -Wno-changes-meaning)
endif()
set_target_properties(database_package_consumer PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
