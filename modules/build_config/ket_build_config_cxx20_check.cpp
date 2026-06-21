// clang-format off
#include "ket_build_config.h"

#include <version>
// clang-format on

#ifndef KET_CXX_VERSION
#error "KET_CXX_VERSION must be defined in C++20."
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

#if KET_HAS_STD_FORMAT && !defined(__cpp_lib_format)
#error "KET_HAS_STD_FORMAT requires __cpp_lib_format."
#endif

} // namespace
