#include <memory>
#include <optional>
#include <string_view>

#include <gtest/gtest.h>

#include "ket_build_config.h"

/**
 * @test
 * @brief 標準headerを先にincludeした場合のfeature判定確認。
 * @details
 * `<optional>` と `<string_view>` を先にincludeしてから `ket_build_config.h` をincludeしても、
 * feature-test macro と header availability に基づく値が保持されることを確認。
 * @pre C++17以降。標準headerを `ket_build_config.h` より先にinclude済み。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigStandardFirstTest, DetectsFeaturesAfterStandardHeaders)
{
#if KET_CXX_AT_LEAST(201703L) && defined(__has_include)
#if __has_include(<optional>) && defined(__cpp_lib_optional)
	const auto expected_optional = 1;
#else
	const auto expected_optional = 0;
#endif
#else
	const auto expected_optional = 0;
#endif

#if KET_CXX_AT_LEAST(201703L) && defined(__has_include)
#if __has_include(<string_view>) && defined(__cpp_lib_string_view)
	const auto expected_string_view = 1;
#else
	const auto expected_string_view = 0;
#endif
#else
	const auto expected_string_view = 0;
#endif

	const auto optional_is_expected = KET_HAS_STD_OPTIONAL == expected_optional;
	const auto string_view_is_expected = KET_HAS_STD_STRING_VIEW == expected_string_view;
	const auto optional_probe = std::optional<int>(1);
	const auto optional_probe_has_value = optional_probe.has_value();
	const auto string_view_probe = std::string_view("ket");
	const auto string_view_probe_size = string_view_probe.size();
	const auto memory_probe = std::unique_ptr<int>();
	auto* const memory_probe_pointer = memory_probe.get();

	EXPECT_EQ(optional_is_expected, true);
	EXPECT_EQ(string_view_is_expected, true);
	EXPECT_EQ(optional_probe_has_value, true);
	EXPECT_EQ(string_view_probe_size, 3U);
	EXPECT_EQ(memory_probe_pointer, nullptr);
}
