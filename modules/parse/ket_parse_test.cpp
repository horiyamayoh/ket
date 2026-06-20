#include "ket_parse.h"

#include <cstdint>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

static_assert(ket::parse::detail::kIsSignedIntegral<std::int8_t>,
			  "std::int8_t is a supported signed parse target");
static_assert(ket::parse::detail::kIsUnsignedIntegral<std::uint8_t>,
			  "std::uint8_t is a supported unsigned parse target");
static_assert(ket::parse::detail::kIsUnsignedIntegral<std::uint64_t>,
			  "std::uint64_t is a supported unsigned parse target");
static_assert(!ket::parse::detail::kIsIntegral<bool>, "bool is not a parse integer target");
static_assert(!ket::parse::detail::kIsIntegral<char>,
			  "plain char signedness is not a parse target");
static_assert(!ket::parse::detail::kIsIntegral<const int>,
			  "cv-qualified integer targets are rejected");
static_assert(!ket::parse::detail::kIsIntegral<wchar_t>, "wide character targets are rejected");

constexpr bool CompileTimeTryBoolParsesTrue() noexcept
{
	bool value = false;
	const auto parsed = ket::parse::TryBool("true", value);

	return parsed && value;
}

constexpr bool CompileTimeTryBoolKeepsOutputOnFailure() noexcept
{
	bool value = true;
	const auto parsed = ket::parse::TryBool("True", value);

	return !parsed && value;
}

constexpr auto kCompileTimeBoolZero = ket::parse::Bool("0");

static_assert(CompileTimeTryBoolParsesTrue(), "TryBool is constexpr for accepted bool text");
static_assert(CompileTimeTryBoolKeepsOutputOnFailure(),
			  "TryBool is constexpr for rejected bool text");
static_assert(kCompileTimeBoolZero.has_value(),
			  "Bool returns constexpr optional for accepted bool text");
static_assert(!kCompileTimeBoolZero.value(), "Bool parses constexpr zero as false");

/**
 * @test
 * @brief 符号付き10進整数parseの正常系確認。
 * @details
 * 0、負数、最大値、最小値を入力し、TryInt、Int、IntOrが符号付き整数へ完全消費で変換することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetParseTest, ParsesSignedDecimalIntegers)
{
	std::int32_t zero = 999;
	std::int32_t negative = 999;
	std::int32_t maximum = 999;
	std::int32_t minimum = 999;

	const auto zero_ok = ket::parse::TryInt("0", zero);
	const auto negative_ok = ket::parse::TryInt("-42", negative);
	const auto maximum_ok = ket::parse::TryInt("2147483647", maximum);
	const auto minimum_ok = ket::parse::TryInt("-2147483648", minimum);
	const auto optional_value = ket::parse::Int<std::int32_t>("-7");
	const auto parsed_or = ket::parse::IntOr<std::int32_t>("-8", 123);

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(negative_ok);
	EXPECT_TRUE(maximum_ok);
	EXPECT_TRUE(minimum_ok);
	EXPECT_EQ(zero, 0);
	EXPECT_EQ(negative, -42);
	EXPECT_EQ(maximum, std::numeric_limits<std::int32_t>::max());
	EXPECT_EQ(minimum, std::numeric_limits<std::int32_t>::min());
	EXPECT_EQ(optional_value, std::optional<std::int32_t>(-7));
	EXPECT_EQ(parsed_or, -8);
}

/**
 * @test
 * @brief 符号付き10進整数parseの失敗条件確認。
 * @details
 * 空文字列、空白、部分消費、先頭不正文字、先頭`+`、overflow、underflowを入力し、失敗時にoutが不変であることを確認。
 * @pre C++17以降。
 * @post 失敗したTryIntのoutは入力時の値を保持。外部状態の変更なし。
 */
TEST(KetParseTest, RejectsInvalidSignedDecimalIntegers)
{
	std::int32_t empty_value = 77;
	std::int32_t leading_space_value = 77;
	std::int32_t trailing_space_value = 77;
	std::int32_t partial_value = 77;
	std::int32_t invalid_value = 77;
	std::int32_t plus_value = 77;
	std::int32_t overflow_value = 77;
	std::int32_t underflow_value = 77;

	const auto empty_ok = ket::parse::TryInt("", empty_value);
	const auto leading_space_ok = ket::parse::TryInt(" 1", leading_space_value);
	const auto trailing_space_ok = ket::parse::TryInt("1 ", trailing_space_value);
	const auto partial_ok = ket::parse::TryInt("1x", partial_value);
	const auto invalid_ok = ket::parse::TryInt("x", invalid_value);
	const auto plus_ok = ket::parse::TryInt("+1", plus_value);
	const auto overflow_ok = ket::parse::TryInt("2147483648", overflow_value);
	const auto underflow_ok = ket::parse::TryInt("-2147483649", underflow_value);
	const auto invalid_optional = ket::parse::Int<std::int32_t>("1x");
	const auto fallback = ket::parse::IntOr<std::int32_t>("1x", 123);

	EXPECT_FALSE(empty_ok);
	EXPECT_FALSE(leading_space_ok);
	EXPECT_FALSE(trailing_space_ok);
	EXPECT_FALSE(partial_ok);
	EXPECT_FALSE(invalid_ok);
	EXPECT_FALSE(plus_ok);
	EXPECT_FALSE(overflow_ok);
	EXPECT_FALSE(underflow_ok);
	EXPECT_EQ(empty_value, 77);
	EXPECT_EQ(leading_space_value, 77);
	EXPECT_EQ(trailing_space_value, 77);
	EXPECT_EQ(partial_value, 77);
	EXPECT_EQ(invalid_value, 77);
	EXPECT_EQ(plus_value, 77);
	EXPECT_EQ(overflow_value, 77);
	EXPECT_EQ(underflow_value, 77);
	EXPECT_EQ(invalid_optional, std::nullopt);
	EXPECT_EQ(fallback, 123);
}

/**
 * @test
 * @brief 符号なし10進整数parseの正常系確認。
 * @details
 * 0、通常値、最大値を入力し、TryUInt、UInt、UIntOrが符号なし整数へ完全消費で変換することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetParseTest, ParsesUnsignedDecimalIntegers)
{
	std::uint32_t zero = 999U;
	std::uint32_t normal = 999U;
	std::uint32_t maximum = 999U;

	const auto zero_ok = ket::parse::TryUInt("0", zero);
	const auto normal_ok = ket::parse::TryUInt("42", normal);
	const auto maximum_ok = ket::parse::TryUInt("4294967295", maximum);
	const auto optional_value = ket::parse::UInt<std::uint32_t>("7");
	const auto parsed_or = ket::parse::UIntOr<std::uint32_t>("8", 123U);

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(normal_ok);
	EXPECT_TRUE(maximum_ok);
	EXPECT_EQ(zero, 0U);
	EXPECT_EQ(normal, 42U);
	EXPECT_EQ(maximum, std::numeric_limits<std::uint32_t>::max());
	EXPECT_EQ(optional_value, std::optional<std::uint32_t>(7U));
	EXPECT_EQ(parsed_or, 8U);
}

/**
 * @test
 * @brief 符号なし10進整数parseの失敗条件確認。
 * @details
 * 空文字列、空白、部分消費、先頭不正文字、overflow、符号文字を入力し、失敗時にoutが不変であることを確認。
 * @pre C++17以降。
 * @post 失敗したTryUIntのoutは入力時の値を保持。外部状態の変更なし。
 */
TEST(KetParseTest, RejectsInvalidUnsignedDecimalIntegers)
{
	std::uint32_t empty_value = 77U;
	std::uint32_t leading_space_value = 77U;
	std::uint32_t trailing_space_value = 77U;
	std::uint32_t partial_value = 77U;
	std::uint32_t invalid_value = 77U;
	std::uint32_t overflow_value = 77U;
	std::uint32_t negative_value = 77U;
	std::uint32_t plus_value = 77U;

	const auto empty_ok = ket::parse::TryUInt("", empty_value);
	const auto leading_space_ok = ket::parse::TryUInt(" 1", leading_space_value);
	const auto trailing_space_ok = ket::parse::TryUInt("1 ", trailing_space_value);
	const auto partial_ok = ket::parse::TryUInt("1x", partial_value);
	const auto invalid_ok = ket::parse::TryUInt("x", invalid_value);
	const auto overflow_ok = ket::parse::TryUInt("4294967296", overflow_value);
	const auto negative_ok = ket::parse::TryUInt("-1", negative_value);
	const auto plus_ok = ket::parse::TryUInt("+1", plus_value);
	const auto invalid_optional = ket::parse::UInt<std::uint32_t>("-1");
	const auto fallback = ket::parse::UIntOr<std::uint32_t>("-1", 123U);

	EXPECT_FALSE(empty_ok);
	EXPECT_FALSE(leading_space_ok);
	EXPECT_FALSE(trailing_space_ok);
	EXPECT_FALSE(partial_ok);
	EXPECT_FALSE(invalid_ok);
	EXPECT_FALSE(overflow_ok);
	EXPECT_FALSE(negative_ok);
	EXPECT_FALSE(plus_ok);
	EXPECT_EQ(empty_value, 77U);
	EXPECT_EQ(leading_space_value, 77U);
	EXPECT_EQ(trailing_space_value, 77U);
	EXPECT_EQ(partial_value, 77U);
	EXPECT_EQ(invalid_value, 77U);
	EXPECT_EQ(overflow_value, 77U);
	EXPECT_EQ(negative_value, 77U);
	EXPECT_EQ(plus_value, 77U);
	EXPECT_EQ(invalid_optional, std::nullopt);
	EXPECT_EQ(fallback, 123U);
}

/**
 * @test
 * @brief 整数parseの型幅境界確認。
 * @details
 * std::int8_t、std::uint8_t、std::uint64_tの境界値を入力し、対象型の範囲内だけ成功することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetParseTest, ParsesIntegralWidthBoundaries)
{
	std::int8_t signed_minimum = 0;
	std::int8_t signed_maximum = 0;
	std::uint8_t unsigned_maximum = 0U;
	std::uint64_t wide_maximum = 0U;

	const auto signed_minimum_ok = ket::parse::TryInt("-128", signed_minimum);
	const auto signed_maximum_ok = ket::parse::TryInt("127", signed_maximum);
	const auto unsigned_maximum_ok = ket::parse::TryUInt("255", unsigned_maximum);
	const auto wide_maximum_ok = ket::parse::TryUInt("18446744073709551615", wide_maximum);

	EXPECT_TRUE(signed_minimum_ok);
	EXPECT_TRUE(signed_maximum_ok);
	EXPECT_TRUE(unsigned_maximum_ok);
	EXPECT_TRUE(wide_maximum_ok);
	EXPECT_EQ(static_cast<int>(signed_minimum), -128);
	EXPECT_EQ(static_cast<int>(signed_maximum), 127);
	EXPECT_EQ(static_cast<unsigned>(unsigned_maximum), 255U);
	EXPECT_EQ(wide_maximum, std::numeric_limits<std::uint64_t>::max());
}

/**
 * @test
 * @brief 整数parseの型幅overflow確認。
 * @details
 * std::int8_t、std::uint8_t、std::uint64_tの範囲外入力で失敗し、outが不変であることを確認。
 * @pre C++17以降。
 * @post 失敗したTryInt/TryUIntのoutは入力時の値を保持。外部状態の変更なし。
 */
TEST(KetParseTest, RejectsIntegralWidthOverflows)
{
	std::int8_t signed_overflow_value = 7;
	std::int8_t signed_underflow_value = 7;
	std::uint8_t unsigned_overflow_value = 7U;
	std::uint64_t wide_overflow_value = 7U;

	const auto signed_overflow_ok = ket::parse::TryInt("128", signed_overflow_value);
	const auto signed_underflow_ok = ket::parse::TryInt("-129", signed_underflow_value);
	const auto unsigned_overflow_ok = ket::parse::TryUInt("256", unsigned_overflow_value);
	const auto wide_overflow_ok = ket::parse::TryUInt("18446744073709551616", wide_overflow_value);

	EXPECT_FALSE(signed_overflow_ok);
	EXPECT_FALSE(signed_underflow_ok);
	EXPECT_FALSE(unsigned_overflow_ok);
	EXPECT_FALSE(wide_overflow_ok);
	EXPECT_EQ(static_cast<int>(signed_overflow_value), 7);
	EXPECT_EQ(static_cast<int>(signed_underflow_value), 7);
	EXPECT_EQ(static_cast<unsigned>(unsigned_overflow_value), 7U);
	EXPECT_EQ(wide_overflow_value, 7U);
}

/**
 * @test
 * @brief 16進整数parseのprefix許容確認。
 * @details
 * prefixなし、`0x`、`0X`、大文字hex、signed型の非負hexを入力し、TryHexとHexが16進整数へ変換することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetParseTest, ParsesHexIntegersWithOptionalPrefix)
{
	std::uint32_t zero = 999U;
	std::uint32_t prefixed_zero = 999U;
	std::uint32_t without_prefix = 0U;
	std::uint32_t lower_prefix = 0U;
	std::uint32_t upper_prefix = 0U;
	std::uint32_t maximum = 0U;
	std::int32_t signed_maximum = 0;

	const auto zero_ok = ket::parse::TryHex("0", zero);
	const auto prefixed_zero_ok = ket::parse::TryHex("0x0", prefixed_zero);
	const auto without_prefix_ok = ket::parse::TryHex("ff", without_prefix);
	const auto lower_prefix_ok = ket::parse::TryHex("0xff", lower_prefix);
	const auto upper_prefix_ok = ket::parse::TryHex("0X10", upper_prefix);
	const auto maximum_ok = ket::parse::TryHex("FFFFFFFF", maximum);
	const auto signed_maximum_ok = ket::parse::TryHex("7fffffff", signed_maximum);
	const auto optional_value = ket::parse::Hex<std::uint32_t>("0x2a");

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(prefixed_zero_ok);
	EXPECT_TRUE(without_prefix_ok);
	EXPECT_TRUE(lower_prefix_ok);
	EXPECT_TRUE(upper_prefix_ok);
	EXPECT_TRUE(maximum_ok);
	EXPECT_TRUE(signed_maximum_ok);
	EXPECT_EQ(zero, 0U);
	EXPECT_EQ(prefixed_zero, 0U);
	EXPECT_EQ(without_prefix, 255U);
	EXPECT_EQ(lower_prefix, 255U);
	EXPECT_EQ(upper_prefix, 16U);
	EXPECT_EQ(maximum, std::numeric_limits<std::uint32_t>::max());
	EXPECT_EQ(signed_maximum, std::numeric_limits<std::int32_t>::max());
	EXPECT_EQ(optional_value, std::optional<std::uint32_t>(42U));
}

/**
 * @test
 * @brief 16進整数parseの失敗条件確認。
 * @details
 * 空文字列、prefixのみ、不正hex、部分消費、空白、符号文字、overflowを入力し、失敗時にoutが不変であることを確認。
 * @pre C++17以降。
 * @post 失敗したTryHexのoutは入力時の値を保持。外部状態の変更なし。
 */
TEST(KetParseTest, RejectsInvalidHexIntegers)
{
	std::uint32_t empty_value = 77U;
	std::uint32_t prefix_only_value = 77U;
	std::uint32_t invalid_value = 77U;
	std::uint32_t partial_value = 77U;
	std::uint32_t leading_whitespace_value = 77U;
	std::uint32_t trailing_whitespace_value = 77U;
	std::uint32_t overflow_value = 77U;
	std::uint32_t negative_value = 77U;
	std::uint32_t negative_prefix_value = 77U;
	std::uint32_t prefix_negative_value = 77U;
	std::uint32_t plus_value = 77U;
	std::int32_t signed_overflow_value = 77;

	const auto empty_ok = ket::parse::TryHex("", empty_value);
	const auto prefix_only_ok = ket::parse::TryHex("0x", prefix_only_value);
	const auto invalid_ok = ket::parse::TryHex("0xg", invalid_value);
	const auto partial_ok = ket::parse::TryHex("0xffx", partial_value);
	const auto leading_whitespace_ok = ket::parse::TryHex(" ff", leading_whitespace_value);
	const auto trailing_whitespace_ok = ket::parse::TryHex("ff ", trailing_whitespace_value);
	const auto overflow_ok = ket::parse::TryHex("100000000", overflow_value);
	const auto negative_ok = ket::parse::TryHex("-1", negative_value);
	const auto negative_prefix_ok = ket::parse::TryHex("-0x1", negative_prefix_value);
	const auto prefix_negative_ok = ket::parse::TryHex("0x-1", prefix_negative_value);
	const auto plus_ok = ket::parse::TryHex("+1", plus_value);
	const auto signed_overflow_ok = ket::parse::TryHex("80000000", signed_overflow_value);
	const auto invalid_optional = ket::parse::Hex<std::uint32_t>("0xg");

	EXPECT_FALSE(empty_ok);
	EXPECT_FALSE(prefix_only_ok);
	EXPECT_FALSE(invalid_ok);
	EXPECT_FALSE(partial_ok);
	EXPECT_FALSE(leading_whitespace_ok);
	EXPECT_FALSE(trailing_whitespace_ok);
	EXPECT_FALSE(overflow_ok);
	EXPECT_FALSE(negative_ok);
	EXPECT_FALSE(negative_prefix_ok);
	EXPECT_FALSE(prefix_negative_ok);
	EXPECT_FALSE(plus_ok);
	EXPECT_FALSE(signed_overflow_ok);
	EXPECT_EQ(empty_value, 77U);
	EXPECT_EQ(prefix_only_value, 77U);
	EXPECT_EQ(invalid_value, 77U);
	EXPECT_EQ(partial_value, 77U);
	EXPECT_EQ(leading_whitespace_value, 77U);
	EXPECT_EQ(trailing_whitespace_value, 77U);
	EXPECT_EQ(overflow_value, 77U);
	EXPECT_EQ(negative_value, 77U);
	EXPECT_EQ(negative_prefix_value, 77U);
	EXPECT_EQ(prefix_negative_value, 77U);
	EXPECT_EQ(plus_value, 77U);
	EXPECT_EQ(signed_overflow_value, 77);
	EXPECT_EQ(invalid_optional, std::nullopt);
}

/**
 * @test
 * @brief bool文字列parseの許容候補確認。
 * @details
 * `true`、`false`、`1`、`0`を入力し、TryBoolとBoolがcase-sensitiveなbool値へ変換することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetParseTest, ParsesBoolCandidates)
{
	bool text_true = false;
	bool text_false = true;
	bool one = false;
	bool zero = true;

	const auto text_true_ok = ket::parse::TryBool("true", text_true);
	const auto text_false_ok = ket::parse::TryBool("false", text_false);
	const auto one_ok = ket::parse::TryBool("1", one);
	const auto zero_ok = ket::parse::TryBool("0", zero);
	const auto optional_true = ket::parse::Bool("true");
	const auto optional_false = ket::parse::Bool("false");

	EXPECT_TRUE(text_true_ok);
	EXPECT_TRUE(text_false_ok);
	EXPECT_TRUE(one_ok);
	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(text_true);
	EXPECT_FALSE(text_false);
	EXPECT_TRUE(one);
	EXPECT_FALSE(zero);
	EXPECT_EQ(optional_true, std::optional<bool>(true));
	EXPECT_EQ(optional_false, std::optional<bool>(false));
}

/**
 * @test
 * @brief bool文字列parseの未採用表現拒否確認。
 * @details
 * 空文字列、大文字小文字違い、yes/no/on/off、空白、部分消費を入力し、失敗時にoutが不変であることを確認。
 * @pre C++17以降。
 * @post 失敗したTryBoolのoutは入力時の値を保持。外部状態の変更なし。
 */
TEST(KetParseTest, RejectsInvalidBoolCandidates)
{
	bool empty_value = true;
	bool case_value = true;
	bool yes_value = true;
	bool no_value = true;
	bool on_value = true;
	bool off_value = true;
	bool whitespace_value = true;
	bool partial_value = true;

	const auto empty_ok = ket::parse::TryBool("", empty_value);
	const auto case_ok = ket::parse::TryBool("True", case_value);
	const auto yes_ok = ket::parse::TryBool("yes", yes_value);
	const auto no_ok = ket::parse::TryBool("no", no_value);
	const auto on_ok = ket::parse::TryBool("on", on_value);
	const auto off_ok = ket::parse::TryBool("off", off_value);
	const auto whitespace_ok = ket::parse::TryBool(" true", whitespace_value);
	const auto partial_ok = ket::parse::TryBool("truex", partial_value);
	const auto invalid_optional = ket::parse::Bool("yes");

	EXPECT_FALSE(empty_ok);
	EXPECT_FALSE(case_ok);
	EXPECT_FALSE(yes_ok);
	EXPECT_FALSE(no_ok);
	EXPECT_FALSE(on_ok);
	EXPECT_FALSE(off_ok);
	EXPECT_FALSE(whitespace_ok);
	EXPECT_FALSE(partial_ok);
	EXPECT_TRUE(empty_value);
	EXPECT_TRUE(case_value);
	EXPECT_TRUE(yes_value);
	EXPECT_TRUE(no_value);
	EXPECT_TRUE(on_value);
	EXPECT_TRUE(off_value);
	EXPECT_TRUE(whitespace_value);
	EXPECT_TRUE(partial_value);
	EXPECT_EQ(invalid_optional, std::nullopt);
}
