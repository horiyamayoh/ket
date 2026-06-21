#if defined(__has_include)
#if __has_include(<span>)
#include <span>
#endif
#if __has_include(<format>)
#include <format> // IWYU pragma: keep
#endif
#endif

// clang-format off
#include "ket_build_config.h"
// clang-format on

namespace
{
#if KET_CXX_AT_LEAST(202002L) && defined(__has_include)
#if __has_include(<span>) && defined(__cpp_lib_span)
	static_assert(KET_HAS_STD_SPAN == 1, "std::span is detected after standard-first include.");
#else
	static_assert(KET_HAS_STD_SPAN == 0, "std::span remains disabled when unavailable.");
#endif
#else
	static_assert(KET_HAS_STD_SPAN == 0, "std::span requires C++20 and __has_include.");
#endif

#if KET_CXX_AT_LEAST(202002L) && defined(__has_include)
#if __has_include(<format>) && defined(__cpp_lib_format)
	static_assert(KET_HAS_STD_FORMAT == 1, "std::format is detected after standard-first include.");
#else
	static_assert(KET_HAS_STD_FORMAT == 0, "std::format remains disabled when unavailable.");
#endif
#else
	static_assert(KET_HAS_STD_FORMAT == 0, "std::format requires C++20 and __has_include.");
#endif

} // namespace

void KetBuildConfigCxx20StandardFirstCheck()
{
#if KET_HAS_STD_SPAN
	int values[] = {1, 2};
	const std::span<int> span_value(values);
	static_cast<void>(span_value);
#endif
}
