// clang-format off
#include "ket_build_config.h"

#include <version>
// clang-format on

#if KET_HAS_STD_SPAN
#include <span>
#endif

#if KET_HAS_STD_FORMAT
#include <format>
#endif

#ifndef KET_CXX_VERSION
#error "KET_CXX_VERSION must be defined in C++20."
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
	static_assert(KET_CXX_VERSION >= 202002L, "C++20 compile-only check reports C++20 or later.");
	static_assert(KET_CXX_AT_LEAST(201703L), "C++20 satisfies the C++17 lower bound.");
	static_assert(KET_CXX_AT_LEAST(202002L), "C++20 satisfies the C++20 lower bound.");
	static_assert(!KET_CXX_AT_LEAST(KET_CXX_VERSION + 1L),
				  "CXX_AT_LEAST rejects values above the reported standard.");
	static_assert(KET_HAS_STD_OPTIONAL == 0 || KET_HAS_STD_OPTIONAL == 1,
				  "std::optional macro is normalized.");
	static_assert(KET_HAS_STD_STRING_VIEW == 0 || KET_HAS_STD_STRING_VIEW == 1,
				  "std::string_view macro is normalized.");
	static_assert(KET_HAS_STD_SPAN == 0 || KET_HAS_STD_SPAN == 1, "std::span macro is normalized.");
	static_assert(KET_HAS_STD_FORMAT == 0 || KET_HAS_STD_FORMAT == 1,
				  "std::format macro is normalized.");

#if KET_HAS_STD_SPAN && !defined(__cpp_lib_span)
#error "KET_HAS_STD_SPAN requires __cpp_lib_span."
#endif

#if KET_CXX_AT_LEAST(202002L) && defined(__has_include)
#if __has_include(<span>) && defined(__cpp_lib_span)
	static_assert(KET_HAS_STD_SPAN == 1, "std::span is detected when ket header is first.");
#else
	static_assert(KET_HAS_STD_SPAN == 0, "std::span remains disabled when unavailable.");
#endif
#else
	static_assert(KET_HAS_STD_SPAN == 0, "std::span requires C++20 and __has_include.");
#endif

#if KET_HAS_STD_FORMAT && !defined(__cpp_lib_format)
#error "KET_HAS_STD_FORMAT requires __cpp_lib_format."
#endif

#if KET_CXX_AT_LEAST(202002L) && defined(__has_include)
#if __has_include(<format>) && defined(__cpp_lib_format)
	static_assert(KET_HAS_STD_FORMAT == 1, "std::format is detected when ket header is first.");
#else
	static_assert(KET_HAS_STD_FORMAT == 0, "std::format remains disabled when unavailable.");
#endif
#else
	static_assert(KET_HAS_STD_FORMAT == 0, "std::format requires C++20 and __has_include.");
#endif

} // namespace

void KetBuildConfigCxx20Check()
{
#if KET_HAS_STD_SPAN
	int values[] = {1, 2};
	const std::span<int> span_value(values);
	static_cast<void>(span_value);
#endif

#if KET_HAS_STD_FORMAT
	const auto formatted = std::format("{}", 1);
	static_cast<void>(formatted);
#endif
}
