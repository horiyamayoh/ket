#include "ket_build_config.h"

#if KET_HAS_STD_OPTIONAL
#include <optional>
#endif

#if KET_HAS_STD_STRING_VIEW
#include <string_view>
#endif

#if KET_HAS_STD_SPAN
#include <span>
#endif

#if KET_HAS_STD_FORMAT
#include <format>
#endif

#include <memory>

#include <gtest/gtest.h>

#ifndef KET_CXX_VERSION
#error "KET_CXX_VERSION must be defined."
#endif

#ifndef KET_CXX_AT_LEAST
#error "KET_CXX_AT_LEAST must be defined."
#endif

#ifndef KET_HAS_STD_OPTIONAL
#error "KET_HAS_STD_OPTIONAL must be defined."
#endif

#ifndef KET_HAS_STD_STRING_VIEW
#error "KET_HAS_STD_STRING_VIEW must be defined."
#endif

#ifndef KET_HAS_STD_SPAN
#error "KET_HAS_STD_SPAN must be defined."
#endif

#ifndef KET_HAS_STD_FORMAT
#error "KET_HAS_STD_FORMAT must be defined."
#endif

#ifndef KET_COMPILER_CLANG
#error "KET_COMPILER_CLANG must be defined."
#endif

#ifndef KET_COMPILER_GCC
#error "KET_COMPILER_GCC must be defined."
#endif

#ifndef KET_COMPILER_MSVC
#error "KET_COMPILER_MSVC must be defined."
#endif

#ifndef KET_OS_WINDOWS
#error "KET_OS_WINDOWS must be defined."
#endif

#ifndef KET_OS_LINUX
#error "KET_OS_LINUX must be defined."
#endif

#ifndef KET_OS_MACOS
#error "KET_OS_MACOS must be defined."
#endif

static_assert(KET_CXX_VERSION == 201103L || KET_CXX_VERSION == 201402L ||
				  KET_CXX_VERSION == 201703L || KET_CXX_VERSION == 202002L ||
				  KET_CXX_VERSION == 202302L,
			  "KET_CXX_VERSION is normalized to a supported standard literal.");
static_assert(KET_CXX_AT_LEAST(201103L), "C++11 minimum is always satisfied.");
static_assert(KET_HAS_STD_OPTIONAL == 0 || KET_HAS_STD_OPTIONAL == 1,
			  "optional feature macro is normalized.");
static_assert(KET_HAS_STD_STRING_VIEW == 0 || KET_HAS_STD_STRING_VIEW == 1,
			  "string_view feature macro is normalized.");
static_assert(KET_HAS_STD_SPAN == 0 || KET_HAS_STD_SPAN == 1, "span feature macro is normalized.");
static_assert(KET_HAS_STD_FORMAT == 0 || KET_HAS_STD_FORMAT == 1,
			  "format feature macro is normalized.");
static_assert(KET_COMPILER_CLANG == 0 || KET_COMPILER_CLANG == 1, "clang macro is normalized.");
static_assert(KET_COMPILER_GCC == 0 || KET_COMPILER_GCC == 1, "GCC macro is normalized.");
static_assert(KET_COMPILER_MSVC == 0 || KET_COMPILER_MSVC == 1, "MSVC macro is normalized.");
static_assert(KET_OS_WINDOWS == 0 || KET_OS_WINDOWS == 1, "Windows macro is normalized.");
static_assert(KET_OS_LINUX == 0 || KET_OS_LINUX == 1, "Linux macro is normalized.");
static_assert(KET_OS_MACOS == 0 || KET_OS_MACOS == 1, "macOS macro is normalized.");

/**
 * @test
 * @brief 公開macroの定義と0/1値確認。
 * @details
 * feature、compiler、OS判定macroを読み取り、仕様が要求する0または1の値へ正規化されることを確認。
 * @pre C++17以降。`ket_build_config.h` を標準headerより先にinclude済み。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigTest, DefinesPublicMacrosAsNormalizedValues)
{
	const auto has_optional_is_normalized = KET_HAS_STD_OPTIONAL == 0 || KET_HAS_STD_OPTIONAL == 1;
	const auto has_string_view_is_normalized =
		KET_HAS_STD_STRING_VIEW == 0 || KET_HAS_STD_STRING_VIEW == 1;
	const auto has_span_is_normalized = KET_HAS_STD_SPAN == 0 || KET_HAS_STD_SPAN == 1;
	const auto has_format_is_normalized = KET_HAS_STD_FORMAT == 0 || KET_HAS_STD_FORMAT == 1;
	const auto compiler_clang_is_normalized = KET_COMPILER_CLANG == 0 || KET_COMPILER_CLANG == 1;
	const auto compiler_gcc_is_normalized = KET_COMPILER_GCC == 0 || KET_COMPILER_GCC == 1;
	const auto compiler_msvc_is_normalized = KET_COMPILER_MSVC == 0 || KET_COMPILER_MSVC == 1;
	const auto os_windows_is_normalized = KET_OS_WINDOWS == 0 || KET_OS_WINDOWS == 1;
	const auto os_linux_is_normalized = KET_OS_LINUX == 0 || KET_OS_LINUX == 1;
	const auto os_macos_is_normalized = KET_OS_MACOS == 0 || KET_OS_MACOS == 1;

	EXPECT_EQ(has_optional_is_normalized, true);
	EXPECT_EQ(has_string_view_is_normalized, true);
	EXPECT_EQ(has_span_is_normalized, true);
	EXPECT_EQ(has_format_is_normalized, true);
	EXPECT_EQ(compiler_clang_is_normalized, true);
	EXPECT_EQ(compiler_gcc_is_normalized, true);
	EXPECT_EQ(compiler_msvc_is_normalized, true);
	EXPECT_EQ(os_windows_is_normalized, true);
	EXPECT_EQ(os_linux_is_normalized, true);
	EXPECT_EQ(os_macos_is_normalized, true);
}

/**
 * @test
 * @brief C++標準値と下限判定macroの境界確認。
 * @details
 * C++11、14、17、20、23の境界値を入力し、`KET_CXX_VERSION`との大小関係に一致することを確認。
 * @pre C++17以降。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigTest, ReportsCxxVersionBoundaries)
{
	const auto at_least_cxx11 = KET_CXX_AT_LEAST(201103L);
	const auto at_least_cxx14 = KET_CXX_AT_LEAST(201402L);
	const auto at_least_cxx17 = KET_CXX_AT_LEAST(201703L);
	const auto at_least_cxx20 = KET_CXX_AT_LEAST(202002L);
	const auto at_least_cxx23 = KET_CXX_AT_LEAST(202302L);

	EXPECT_EQ(at_least_cxx11, KET_CXX_VERSION >= 201103L);
	EXPECT_EQ(at_least_cxx14, KET_CXX_VERSION >= 201402L);
	EXPECT_EQ(at_least_cxx17, KET_CXX_VERSION >= 201703L);
	EXPECT_EQ(at_least_cxx20, KET_CXX_VERSION >= 202002L);
	EXPECT_EQ(at_least_cxx23, KET_CXX_VERSION >= 202302L);
}

/**
 * @test
 * @brief compiler判定macroの条件確認。
 * @details active compilerのpredefined
 * macroに合わせ、clang、GCC、MSVC判定が相互排他的になることを確認。
 * @pre C++17以降。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigTest, DetectsCompilerMacros)
{
#if defined(__clang__)
	const auto expected_clang = 1;
	const auto expected_gcc = 0;
	const auto expected_msvc = 0;
#elif defined(__GNUC__)
	const auto expected_clang = 0;
	const auto expected_gcc = 1;
	const auto expected_msvc = 0;
#elif defined(_MSC_VER)
	const auto expected_clang = 0;
	const auto expected_gcc = 0;
	const auto expected_msvc = 1;
#else
	const auto expected_clang = 0;
	const auto expected_gcc = 0;
	const auto expected_msvc = 0;
#endif
	const auto detected_compiler_count = KET_COMPILER_CLANG + KET_COMPILER_GCC + KET_COMPILER_MSVC;
	const auto compiler_count_is_valid = detected_compiler_count <= 1;

	EXPECT_EQ(KET_COMPILER_CLANG, expected_clang);
	EXPECT_EQ(KET_COMPILER_GCC, expected_gcc);
	EXPECT_EQ(KET_COMPILER_MSVC, expected_msvc);
	EXPECT_EQ(compiler_count_is_valid, true);
}

/**
 * @test
 * @brief OS判定macroの条件確認。
 * @details active OSのpredefined
 * macroに合わせ、Windows、Linux、macOS判定が現在のplatformを示すことを確認。 unknown
 * OSでは3macroすべて0を許容する。
 * @pre C++17以降。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigTest, DetectsOperatingSystemMacros)
{
#if defined(_WIN32)
	const auto expected_windows = 1;
#else
	const auto expected_windows = 0;
#endif

#if defined(__linux__)
	const auto expected_linux = 1;
#else
	const auto expected_linux = 0;
#endif

#if defined(__APPLE__) && defined(__MACH__)
	const auto expected_macos = 1;
#else
	const auto expected_macos = 0;
#endif

	const auto detected_os_count = KET_OS_WINDOWS + KET_OS_LINUX + KET_OS_MACOS;
	const auto os_count_is_valid = detected_os_count <= 1;

	EXPECT_EQ(KET_OS_WINDOWS, expected_windows);
	EXPECT_EQ(KET_OS_LINUX, expected_linux);
	EXPECT_EQ(KET_OS_MACOS, expected_macos);
	EXPECT_EQ(os_count_is_valid, true);
}

/**
 * @test
 * @brief standard library feature判定macroの条件確認。
 * @details header availabilityとfeature-test
 * macroが確認できるfeatureだけ1になり、対象標準未満では0へ倒れることを確認。
 * @pre C++17以降。`ket_build_config.h` を標準headerより先にinclude済み。
 * @post テスト対象macroと外部状態の変更なし。
 */
TEST(KetBuildConfigTest, DetectsStandardLibraryFeatureMacros)
{
#if KET_CXX_AT_LEAST(201703L) && defined(__cpp_lib_optional)
	const auto expected_optional = 1;
#else
	const auto expected_optional = 0;
#endif

#if KET_CXX_AT_LEAST(201703L) && defined(__cpp_lib_string_view)
	const auto expected_string_view = 1;
#else
	const auto expected_string_view = 0;
#endif

#if KET_CXX_AT_LEAST(202002L) && defined(__cpp_lib_span)
	const auto expected_span = 1;
#else
	const auto expected_span = 0;
#endif

#if KET_CXX_AT_LEAST(202002L) && defined(__cpp_lib_format)
	const auto expected_format = 1;
#else
	const auto expected_format = 0;
#endif

	EXPECT_EQ(KET_HAS_STD_OPTIONAL, expected_optional);
	EXPECT_EQ(KET_HAS_STD_STRING_VIEW, expected_string_view);
	EXPECT_EQ(KET_HAS_STD_SPAN, expected_span);
	EXPECT_EQ(KET_HAS_STD_FORMAT, expected_format);

	const auto standard_header_probe = std::unique_ptr<int>();
	EXPECT_EQ(standard_header_probe.get(), nullptr);

#if KET_HAS_STD_OPTIONAL
	const auto optional_value = std::optional<int>(1);
	EXPECT_EQ(optional_value.has_value(), true);
#endif

#if KET_HAS_STD_STRING_VIEW
	const auto string_view_value = std::string_view("ket");
	EXPECT_EQ(string_view_value.size(), 3U);
#endif

#if KET_HAS_STD_SPAN
	int values[] = {1, 2};
	const auto span_value = std::span<int>(values);
	EXPECT_EQ(span_value.size(), 2U);
#endif

#if KET_HAS_STD_FORMAT
	const auto formatted_size = std::format("{}", 1).size();
	EXPECT_EQ(formatted_size, 1U);
#endif
}
