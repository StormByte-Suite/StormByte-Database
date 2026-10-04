if(NOT DEFINED DATABASE_BINARY_DIR OR NOT DEFINED DATABASE_SOURCE_DIR OR NOT DEFINED SMOKE_ROOT)
	message(FATAL_ERROR "Installed consumer smoke test was not given its build paths")
endif()

set(_install_prefix "${SMOKE_ROOT}/prefix")
set(_consumer_source "${SMOKE_ROOT}/consumer")
set(_consumer_build "${SMOKE_ROOT}/consumer-build")
file(MAKE_DIRECTORY "${_consumer_source}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" --install "${DATABASE_BINARY_DIR}" --prefix "${_install_prefix}"
	RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
	message(FATAL_ERROR "Could not install Database for the external-consumer test")
endif()

file(WRITE "${_consumer_source}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.28)
project(StormByteDatabaseInstalledConsumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_library(DATABASE_LIBRARY NAMES StormByte-Database PATHS "${DATABASE_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_library(LOGGER_LIBRARY NAMES StormByte-Logger PATHS "${DEPENDENCY_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_library(BASE_LIBRARY NAMES StormByte PATHS "${DEPENDENCY_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
add_executable(InstalledConsumer "${DATABASE_SOURCE_DIR}/test/install_consumer.cxx")
target_include_directories(InstalledConsumer PRIVATE "${DATABASE_INCLUDE_DIR}")
target_include_directories(InstalledConsumer SYSTEM PRIVATE "${DEPENDENCY_INCLUDE_DIR}")
target_link_libraries(InstalledConsumer PRIVATE "${DATABASE_LIBRARY}" "${LOGGER_LIBRARY}" "${BASE_LIBRARY}")
set_target_properties(InstalledConsumer PROPERTIES BUILD_RPATH "${DATABASE_LIBRARY_DIR};${DEPENDENCY_LIBRARY_DIR}")
if(ENABLE_SQLITE)
	target_compile_definitions(InstalledConsumer PRIVATE STORMBYTE_TEST_SQLITE)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# Logger's ThrottleSpec::Level member trips GCC's default-on changes-meaning error.
	target_compile_options(InstalledConsumer PRIVATE -Wno-changes-meaning)
endif()
]=])

set(_database_lib_dir "${_install_prefix}/${DATABASE_INSTALL_LIBDIR}")
set(_dependency_prefix "${DATABASE_BINARY_DIR}/stormbyte-buildmaster/install")
if(NOT IS_DIRECTORY "${_dependency_prefix}")
	set(_dependency_prefix "${DATABASE_BINARY_DIR}/buildmaster/install")
endif()
set(_configure_command "${CMAKE_COMMAND}" -S "${_consumer_source}" -B "${_consumer_build}" -G "${DATABASE_GENERATOR}"
	"-DDATABASE_SOURCE_DIR=${DATABASE_SOURCE_DIR}"
	"-DDATABASE_INCLUDE_DIR=${_install_prefix}/include"
	"-DDEPENDENCY_INCLUDE_DIR=${_dependency_prefix}/include"
	"-DDATABASE_LIBRARY_DIR=${_database_lib_dir}"
	"-DDEPENDENCY_LIBRARY_DIR=${_dependency_prefix}/${DATABASE_INSTALL_LIBDIR}"
	"-DENABLE_SQLITE=${DATABASE_ENABLE_SQLITE}"
	"-DCMAKE_CXX_COMPILER=${DATABASE_CXX_COMPILER}"
)
if(DEFINED DATABASE_CXX_FLAGS AND NOT DATABASE_CXX_FLAGS STREQUAL "")
	list(APPEND _configure_command "-DCMAKE_CXX_FLAGS=${DATABASE_CXX_FLAGS}")
endif()
if(DEFINED DATABASE_BUILD_TYPE AND NOT DATABASE_BUILD_TYPE STREQUAL "")
	list(APPEND _configure_command "-DCMAKE_BUILD_TYPE=${DATABASE_BUILD_TYPE}")
endif()
execute_process(COMMAND ${_configure_command} RESULT_VARIABLE _configure_result)
if(NOT _configure_result EQUAL 0)
	message(FATAL_ERROR "Could not configure external consumer against installed Database")
endif()

set(_build_command "${CMAKE_COMMAND}" --build "${_consumer_build}" --parallel 32)
if(DEFINED DATABASE_BUILD_CONFIG AND NOT DATABASE_BUILD_CONFIG STREQUAL "")
	list(APPEND _build_command --config "${DATABASE_BUILD_CONFIG}")
endif()
execute_process(COMMAND ${_build_command} RESULT_VARIABLE _build_result)
if(NOT _build_result EQUAL 0)
	message(FATAL_ERROR "External consumer did not compile and link against installed Database")
endif()

set(_consumer_executable "${_consumer_build}/InstalledConsumer${DATABASE_EXECUTABLE_SUFFIX}")
if(DATABASE_GENERATOR MATCHES "Multi-Config|Visual Studio|Xcode" AND DEFINED DATABASE_BUILD_CONFIG)
	set(_consumer_executable "${_consumer_build}/${DATABASE_BUILD_CONFIG}/InstalledConsumer${DATABASE_EXECUTABLE_SUFFIX}")
endif()
if(WIN32)
	set(_runtime_path "$ENV{PATH};${_database_lib_dir};${_dependency_prefix}/bin")
	set(_runtime_variable "PATH")
elseif(APPLE)
	set(_runtime_path "${_database_lib_dir}:${_dependency_prefix}/${DATABASE_INSTALL_LIBDIR}:$ENV{DYLD_LIBRARY_PATH}")
	set(_runtime_variable "DYLD_LIBRARY_PATH")
else()
	set(_runtime_path "${_database_lib_dir}:${_dependency_prefix}/${DATABASE_INSTALL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
	set(_runtime_variable "LD_LIBRARY_PATH")
endif()
execute_process(
	COMMAND "${CMAKE_COMMAND}" -E env "${_runtime_variable}=${_runtime_path}" "${_consumer_executable}"
	RESULT_VARIABLE _run_result
)
if(NOT _run_result EQUAL 0)
	message(FATAL_ERROR "Installed external consumer failed at runtime: ${_run_result}")
endif()
