cmake_minimum_required(VERSION 3.24)

project(ket VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)

option(KET_BUILD_TESTS "Build ket tests." ON)
option(KET_ENABLE_FORMAT_TARGETS "Enable format and check-format targets." ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(KET_ENABLE_FORMAT_TARGETS)
	find_package(Python3 COMPONENTS Interpreter)

	if(Python3_Interpreter_FOUND)
		add_custom_target(
			format
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/format.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Formatting C++ files with clang-format"
		)

		add_custom_target(
			check-format
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_format.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Checking C++ format with clang-format"
		)
	endif()
endif()

if(KET_BUILD_TESTS)
	set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
		GIT_SHALLOW TRUE
	)
	FetchContent_MakeAvailable(googletest)

	include(KetModule)

	ket_add_module_test(
		ket_repository_smoke_test
		SOURCES
			tests/ket_repository_smoke_test.cpp
	)
endif()
