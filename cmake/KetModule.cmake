include(CMakeParseArguments)
include(GoogleTest)

function(ket_add_module_test target)
	set(options)
	set(one_value_args)
	set(multi_value_args SOURCES)
	cmake_parse_arguments(KET_TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	if(NOT KET_TEST_SOURCES)
		message(FATAL_ERROR "ket_add_module_test(${target}) requires SOURCES.")
	endif()

	add_executable(${target} ${KET_TEST_SOURCES})
	target_compile_features(${target} PRIVATE cxx_std_17)
	set_target_properties(
		${target}
		PROPERTIES
			CXX_EXTENSIONS OFF
	)
	target_link_libraries(${target} PRIVATE GTest::gtest_main)

	gtest_discover_tests(${target})
endfunction()

function(ket_add_compile_check target)
	set(options)
	set(one_value_args CXX_STANDARD)
	set(multi_value_args SOURCES)
	cmake_parse_arguments(KET_COMPILE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	if(NOT KET_COMPILE_CXX_STANDARD)
		message(FATAL_ERROR "ket_add_compile_check(${target}) requires CXX_STANDARD.")
	endif()

	if(NOT KET_COMPILE_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
		message(
			FATAL_ERROR
			"ket_add_compile_check(${target}) CXX_STANDARD must be one of 11, 14, 17, 20, 23."
		)
	endif()

	if(NOT KET_COMPILE_SOURCES)
		message(FATAL_ERROR "ket_add_compile_check(${target}) requires SOURCES.")
	endif()

	add_library(${target} OBJECT EXCLUDE_FROM_ALL ${KET_COMPILE_SOURCES})
	set_target_properties(
		${target}
		PROPERTIES
			CXX_STANDARD ${KET_COMPILE_CXX_STANDARD}
			CXX_STANDARD_REQUIRED ON
			CXX_EXTENSIONS OFF
	)

	add_custom_target(${target}_check DEPENDS ${target})
endfunction()
