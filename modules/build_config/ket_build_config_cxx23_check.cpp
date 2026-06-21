// clang-format off
#include "ket_build_config.h"
// clang-format on

#ifndef KET_CXX_VERSION
#error "KET_CXX_VERSION must be defined in C++23."
#endif

#ifndef KET_CXX_AT_LEAST
#error "KET_CXX_AT_LEAST must be defined in C++23."
#endif

#ifdef KET_DETAIL_CXX_STANDARD_VALUE
#error "KET_DETAIL_CXX_STANDARD_VALUE must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_CXX_VERSION_VALUE
#error "KET_DETAIL_CXX_VERSION_VALUE must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_CXX_AT_LEAST
#error "KET_DETAIL_CXX_AT_LEAST must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_HAS_INCLUDE_OPERATOR
#error "KET_DETAIL_HAS_INCLUDE_OPERATOR must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_HAS_STD_OPTIONAL_HEADER
#error "KET_DETAIL_HAS_STD_OPTIONAL_HEADER must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_HAS_STD_STRING_VIEW_HEADER
#error "KET_DETAIL_HAS_STD_STRING_VIEW_HEADER must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_HAS_STD_SPAN_HEADER
#error "KET_DETAIL_HAS_STD_SPAN_HEADER must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_HAS_STD_FORMAT_HEADER
#error "KET_DETAIL_HAS_STD_FORMAT_HEADER must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_APPLE_OSX_TARGET
#error "KET_DETAIL_APPLE_OSX_TARGET must not leak from ket_build_config.h."
#endif

#ifdef KET_DETAIL_APPLE_MACCATALYST_TARGET
#error "KET_DETAIL_APPLE_MACCATALYST_TARGET must not leak from ket_build_config.h."
#endif

namespace
{
	static_assert(KET_CXX_VERSION >= 202302L, "C++23 compile-only check reports C++23 or later.");
	static_assert(KET_CXX_AT_LEAST(201103L), "C++23 satisfies the C++11 lower bound.");
	static_assert(KET_CXX_AT_LEAST(201402L), "C++23 satisfies the C++14 lower bound.");
	static_assert(KET_CXX_AT_LEAST(201703L), "C++23 satisfies the C++17 lower bound.");
	static_assert(KET_CXX_AT_LEAST(202002L), "C++23 satisfies the C++20 lower bound.");
	static_assert(KET_CXX_AT_LEAST(202302L), "C++23 satisfies the C++23 lower bound.");
	static_assert(!KET_CXX_AT_LEAST(KET_CXX_VERSION + 1L),
				  "CXX_AT_LEAST rejects values above the reported standard.");
	static_assert(KET_HAS_STD_OPTIONAL == 0 || KET_HAS_STD_OPTIONAL == 1,
				  "std::optional macro is normalized.");
	static_assert(KET_HAS_STD_STRING_VIEW == 0 || KET_HAS_STD_STRING_VIEW == 1,
				  "std::string_view macro is normalized.");
	static_assert(KET_HAS_STD_SPAN == 0 || KET_HAS_STD_SPAN == 1, "std::span macro is normalized.");
	static_assert(KET_HAS_STD_FORMAT == 0 || KET_HAS_STD_FORMAT == 1,
				  "std::format macro is normalized.");
	static_assert(KET_COMPILER_CLANG == 0 || KET_COMPILER_CLANG == 1, "clang macro is normalized.");
	static_assert(KET_COMPILER_GCC == 0 || KET_COMPILER_GCC == 1, "GCC macro is normalized.");
	static_assert(KET_COMPILER_MSVC == 0 || KET_COMPILER_MSVC == 1, "MSVC macro is normalized.");
	static_assert(KET_OS_WINDOWS == 0 || KET_OS_WINDOWS == 1, "Windows macro is normalized.");
	static_assert(KET_OS_LINUX == 0 || KET_OS_LINUX == 1, "Linux macro is normalized.");
	static_assert(KET_OS_MACOS == 0 || KET_OS_MACOS == 1, "macOS macro is normalized.");

} // namespace
