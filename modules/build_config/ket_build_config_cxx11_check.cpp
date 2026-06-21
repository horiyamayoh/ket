// clang-format off
#include "ket_build_config.h"
// clang-format on

#ifndef KET_CXX_VERSION
#error "KET_CXX_VERSION must be defined in C++11."
#endif

#ifndef KET_CXX_AT_LEAST
#error "KET_CXX_AT_LEAST must be defined in C++11."
#endif

#ifndef KET_HAS_STD_OPTIONAL
#error "KET_HAS_STD_OPTIONAL must be defined in C++11."
#endif

#ifndef KET_HAS_STD_STRING_VIEW
#error "KET_HAS_STD_STRING_VIEW must be defined in C++11."
#endif

#ifndef KET_HAS_STD_SPAN
#error "KET_HAS_STD_SPAN must be defined in C++11."
#endif

#ifndef KET_HAS_STD_FORMAT
#error "KET_HAS_STD_FORMAT must be defined in C++11."
#endif

namespace
{
	static_assert(KET_CXX_VERSION == 201103L,
				  "C++11 compile-only check reports the C++11 standard literal.");
	static_assert(KET_CXX_AT_LEAST(201103L), "C++11 satisfies the C++11 lower bound.");
	static_assert(!KET_CXX_AT_LEAST(201402L), "C++11 does not satisfy the C++14 lower bound.");
	static_assert(KET_HAS_STD_OPTIONAL == 0, "std::optional is not reported for C++11.");
	static_assert(KET_HAS_STD_STRING_VIEW == 0, "std::string_view is not reported for C++11.");
	static_assert(KET_HAS_STD_SPAN == 0, "std::span is not reported for C++11.");
	static_assert(KET_HAS_STD_FORMAT == 0, "std::format is not reported for C++11.");
	static_assert(KET_COMPILER_CLANG == 0 || KET_COMPILER_CLANG == 1, "clang macro is normalized.");
	static_assert(KET_COMPILER_GCC == 0 || KET_COMPILER_GCC == 1, "GCC macro is normalized.");
	static_assert(KET_COMPILER_MSVC == 0 || KET_COMPILER_MSVC == 1, "MSVC macro is normalized.");
	static_assert(KET_OS_WINDOWS == 0 || KET_OS_WINDOWS == 1, "Windows macro is normalized.");
	static_assert(KET_OS_LINUX == 0 || KET_OS_LINUX == 1, "Linux macro is normalized.");
	static_assert(KET_OS_MACOS == 0 || KET_OS_MACOS == 1, "macOS macro is normalized.");

} // namespace
