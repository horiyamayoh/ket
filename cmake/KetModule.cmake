include(CMakeParseArguments)
include(GoogleTest)

function(ket_apply_sanitizers target)
	if(NOT KET_ENABLE_SANITIZERS)
		return()
	endif()

	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		message(FATAL_ERROR "KET_ENABLE_SANITIZERS is supported only with GNU or Clang.")
	endif()

	set(sanitizer_options
		-fsanitize=address,undefined
		-fno-omit-frame-pointer
	)

	target_compile_options(${target} PRIVATE ${sanitizer_options})

	get_target_property(target_type ${target} TYPE)
	if(NOT target_type STREQUAL "OBJECT_LIBRARY")
		target_link_options(${target} PRIVATE -fsanitize=address,undefined)
	endif()
endfunction()

function(ket_apply_strict_warnings target)
	if(NOT KET_ENABLE_STRICT_WARNINGS)
		return()
	endif()

	set(common_warning_options
		-Wall
		-Wextra
		-Wpedantic
		-Wconversion
		-Wsign-conversion
		-Wshadow
		-Wold-style-cast
		-Wdouble-promotion
		-Wformat=2
		-Wundef
		-Wnon-virtual-dtor
		-Woverloaded-virtual
		-Wnull-dereference
	)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(
			${target}
			PRIVATE
				${common_warning_options}
				-Wduplicated-cond
				-Wduplicated-branches
				-Wlogical-op
				-Wuseless-cast
		)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(
			${target}
			PRIVATE
				${common_warning_options}
				-Wextra-semi
				-Wimplicit-fallthrough
		)
	elseif(MSVC)
		target_compile_options(${target} PRIVATE /W4)
	endif()

	if(KET_WARNINGS_AS_ERRORS)
		if(MSVC)
			target_compile_options(${target} PRIVATE /WX)
		else()
			target_compile_options(${target} PRIVATE -Werror)
		endif()
	endif()
endfunction()

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
	ket_apply_strict_warnings(${target})
	ket_apply_sanitizers(${target})

	gtest_discover_tests(${target})
endfunction()

function(ket_resolve_compile_check_sources target output_variable)
	set(resolved_sources)
	set(header_index 0)

	foreach(source IN LISTS KET_COMPILE_SOURCES)
		get_filename_component(source_extension "${source}" EXT)

		if(source_extension MATCHES "^\\.(h|hh|hpp|hxx)$")
			math(EXPR header_index "${header_index} + 1")
			get_filename_component(source_absolute "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
			file(RELATIVE_PATH source_relative "${CMAKE_CURRENT_SOURCE_DIR}" "${source_absolute}")
			file(TO_CMAKE_PATH "${source_absolute}" source_include)
			string(MAKE_C_IDENTIFIER "${source_relative}" source_identifier)
			set(generated_source
				"${CMAKE_CURRENT_BINARY_DIR}/ket_compile_checks/${target}_${header_index}_${source_identifier}.cpp"
			)

			file(GENERATE OUTPUT "${generated_source}" CONTENT "#include \"${source_include}\"\n")
			list(APPEND resolved_sources "${generated_source}")
		else()
			list(APPEND resolved_sources "${source}")
		endif()
	endforeach()

	set(${output_variable} "${resolved_sources}" PARENT_SCOPE)
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

	ket_resolve_compile_check_sources(${target} compile_check_sources)
	add_library(${target} OBJECT EXCLUDE_FROM_ALL ${compile_check_sources})
	set_target_properties(
		${target}
		PROPERTIES
			CXX_STANDARD ${KET_COMPILE_CXX_STANDARD}
			CXX_STANDARD_REQUIRED ON
			CXX_EXTENSIONS OFF
	)
	ket_apply_strict_warnings(${target})
	ket_apply_sanitizers(${target})

	add_custom_target(${target}_check DEPENDS ${target})
endfunction()
