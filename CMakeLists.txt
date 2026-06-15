cmake_minimum_required(VERSION 3.24)

project(ket VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)

option(KET_BUILD_TESTS "Build ket tests." ON)
option(KET_ENABLE_FORMAT_TARGETS "Enable format and check-format targets." ON)
option(KET_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers." OFF)
option(KET_ENABLE_STRICT_WARNINGS "Enable strict warning options for ket targets." ON)
option(KET_WARNINGS_AS_ERRORS "Treat ket target warnings as errors." ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(KET_ENABLE_FORMAT_TARGETS)
	find_package(Python3 COMPONENTS Interpreter)

	if(Python3_Interpreter_FOUND)
		add_custom_target(
			format
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/format.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Formatting files with clang-format and Prettier"
		)

		add_custom_target(
			check-format
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_format.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Checking format with clang-format and Prettier"
		)

		add_custom_target(
			check-tidy
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tidy.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Running clang-tidy for ket sources"
		)

		add_custom_target(
			check-cppcheck
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_cppcheck.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Running cppcheck for ket sources"
		)

		add_custom_target(
			check-iwyu
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_iwyu.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Running include-what-you-use for ket sources"
		)

		add_custom_target(
			check-static
			DEPENDS check-tidy check-cppcheck check-iwyu
		)

		add_custom_target(
			check-conventions
			COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_conventions.py"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Checking ket source conventions"
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

	ket_add_module_test(
		ket_bcd_test
		SOURCES
			modules/bcd/ket_bcd.cpp
			modules/bcd/ket_bcd_test.cpp
	)

	ket_add_module_test(
		ket_date_test
		SOURCES
			modules/date/ket_date_test.cpp
	)

	ket_add_compile_check(
		ket_date_cxx11_check
		CXX_STANDARD 11
		SOURCES
			modules/date/ket_date_compile_check.cpp
	)

	ket_add_compile_check(
		ket_date_cxx14_check
		CXX_STANDARD 14
		SOURCES
			modules/date/ket_date_compile_check.cpp
	)

	ket_add_module_test(
		ket_deadline_test
		SOURCES
			modules/deadline/ket_deadline.cpp
			modules/deadline/ket_deadline_test.cpp
	)

	ket_add_compile_check(
		ket_deadline_cxx11_check
		CXX_STANDARD 11
		SOURCES
			modules/deadline/ket_deadline.cpp
			modules/deadline/ket_deadline.h
	)
	add_dependencies(ket_deadline_test ket_deadline_cxx11_check)

	ket_add_module_test(
		ket_string_test
		SOURCES
			modules/string/ket_string_test.cpp
	)

	set(ket_ranges_cxx11_check_source "${CMAKE_CURRENT_BINARY_DIR}/ket_ranges_cxx11_check.cpp")
	file(
		WRITE
		"${ket_ranges_cxx11_check_source}"
		"#include \"${CMAKE_CURRENT_SOURCE_DIR}/modules/ranges/ket_ranges.h\"\n"
		"\n"
		"#include <cstddef>\n"
		"#include <vector>\n"
		"\n"
		"void KetRangesCxx11Check()\n"
		"{\n"
		"\tstd::vector<int> values = {1, 2};\n"
		"\tket::ranges::ForEachWithIndex(values, [](std::size_t index, int& value) {\n"
		"\t\tvalue += static_cast<int>(index);\n"
		"\t});\n"
		"\tstd::size_t found_index = 0U;\n"
		"\tconst auto found = ket::ranges::FindIndexIf(values, [](int value) {\n"
		"\t\treturn value == 3;\n"
		"\t}, found_index);\n"
		"\tstatic_cast<void>(found);\n"
		"}\n"
	)

	ket_add_compile_check(
		ket_ranges_cxx11_check
		CXX_STANDARD 11
		SOURCES
			"${ket_ranges_cxx11_check_source}"
	)

	ket_add_module_test(
		ket_ranges_test
		SOURCES
			modules/ranges/ket_ranges_test.cpp
	)
	add_dependencies(ket_ranges_test ket_ranges_cxx11_check)
endif()
