#include "ket_port.h"

#include <cstdint>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
	/**
	 * @brief optional portの期待値一致判定。
	 * @param[in] value 判定対象のoptional port。
	 * @param[in] expected 期待するport番号。
	 * @retval true `value`が値を持ち、期待値と一致。
	 * @retval false 値なし、または期待値不一致。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	constexpr bool OptionalPortEquals(std::optional<ket::port::Port> value,
									  std::uint16_t expected) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return value->value == expected;
	}

	/**
	 * @brief TryFromUInt成功時の値一致判定。
	 * @param[in] value 変換対象の整数値。
	 * @param[in] expected 期待するport番号。
	 * @retval true 変換に成功し、期待値と一致。
	 * @retval false 変換失敗、または期待値不一致。
	 * @pre なし。
	 * @post 外部状態の変更なし。
	 */
	constexpr bool TryFromUIntEquals(std::uint32_t value, std::uint16_t expected) noexcept
	{
		ket::port::Port port;
		const auto ok = ket::port::TryFromUInt(value, port);
		if (!ok)
		{
			return false;
		}

		return port.value == expected;
	}

	/**
	 * @brief TryFromUInt失敗時の出力保持判定。
	 * @param[in] value 変換対象の整数値。
	 * @retval true 変換に失敗し、出力値が入力時の値を保持。
	 * @retval false 変換成功、または出力値の変更あり。
	 * @pre なし。
	 * @post 外部状態の変更なし。
	 */
	constexpr bool TryFromUIntRejectsWithoutChanging(std::uint32_t value) noexcept
	{
		ket::port::Port port{80U};
		const auto ok = ket::port::TryFromUInt(value, port);
		if (ok)
		{
			return false;
		}

		return port.value == 80U;
	}

	static_assert(TryFromUIntEquals(0U, 0U), "zero port is constexpr");
	static_assert(TryFromUIntEquals(65535U, 65535U), "maximum port is constexpr");
	static_assert(TryFromUIntRejectsWithoutChanging(65536U), "out-of-range port is constexpr");

} // namespace

/**
 * @test
 * @brief wide整数からport値型への境界値変換確認。
 * @details 0、1、65535を入力し、port値型へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetPortTest, ConvertsUIntBoundariesToPort)
{
	ket::port::Port zero{123U};
	ket::port::Port one{123U};
	ket::port::Port maximum{123U};

	const auto zero_ok = ket::port::TryFromUInt(0U, zero);
	const auto one_ok = ket::port::TryFromUInt(1U, one);
	const auto maximum_ok = ket::port::TryFromUInt(65535U, maximum);

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(one_ok);
	EXPECT_TRUE(maximum_ok);
	EXPECT_EQ(zero.value, std::uint16_t{0U});
	EXPECT_EQ(one.value, std::uint16_t{1U});
	EXPECT_EQ(maximum.value, std::uint16_t{65535U});
}

/**
 * @test
 * @brief wide整数からport値型への範囲外拒否確認。
 * @details 65535を超える値を入力し、変換失敗と出力引数の保持を確認。
 * @pre C++17以降。
 * @post 出力引数は入力時の値を保持。出力引数以外の外部状態の変更なし。
 */
TEST(KetPortTest, RejectsOutOfRangeUIntWithoutChangingOutput)
{
	ket::port::Port output{80U};

	const auto ok = ket::port::TryFromUInt(65536U, output);

	EXPECT_FALSE(ok);
	EXPECT_EQ(output.value, std::uint16_t{80U});
}

/**
 * @test
 * @brief 10進文字列からport値型への境界値parse確認。
 * @details 0、1、65535を入力し、port値型へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPortTest, ParsesDecimalBoundaries)
{
	const auto zero = ket::port::Parse("0");
	const auto one = ket::port::Parse("1");
	const auto maximum = ket::port::Parse("65535");

	const auto zero_matches = OptionalPortEquals(zero, 0U);
	const auto one_matches = OptionalPortEquals(one, 1U);
	const auto maximum_matches = OptionalPortEquals(maximum, 65535U);

	EXPECT_TRUE(zero_matches);
	EXPECT_TRUE(one_matches);
	EXPECT_TRUE(maximum_matches);
}

/**
 * @test
 * @brief 10進文字列parseの空白、符号、不正文字拒否確認。
 * @details 空文字列、leading/trailing
 * whitespace、符号、不正文字を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPortTest, RejectsNonCanonicalDecimalText)
{
	const auto empty = ket::port::Parse("");
	const auto leading_space = ket::port::Parse(" 80");
	const auto trailing_space = ket::port::Parse("80 ");
	const auto plus = ket::port::Parse("+80");
	const auto minus = ket::port::Parse("-1");
	const auto alphabet = ket::port::Parse("8A");
	const auto embedded_space = ket::port::Parse("8 0");

	EXPECT_EQ(empty, std::nullopt);
	EXPECT_EQ(leading_space, std::nullopt);
	EXPECT_EQ(trailing_space, std::nullopt);
	EXPECT_EQ(plus, std::nullopt);
	EXPECT_EQ(minus, std::nullopt);
	EXPECT_EQ(alphabet, std::nullopt);
	EXPECT_EQ(embedded_space, std::nullopt);
}

/**
 * @test
 * @brief 10進文字列parseの範囲外とleading zero拒否確認。
 * @details 65535超過、6桁入力、複数桁のleading zeroを入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPortTest, RejectsOverflowAndLeadingZero)
{
	const auto overflow_by_one = ket::port::Parse("65536");
	const auto overflow_five_digits = ket::port::Parse("99999");
	const auto overflow_six_digits = ket::port::Parse("100000");
	const auto double_zero = ket::port::Parse("00");
	const auto leading_zero = ket::port::Parse("01");
	const auto padded_zero = ket::port::Parse("00000");

	EXPECT_EQ(overflow_by_one, std::nullopt);
	EXPECT_EQ(overflow_five_digits, std::nullopt);
	EXPECT_EQ(overflow_six_digits, std::nullopt);
	EXPECT_EQ(double_zero, std::nullopt);
	EXPECT_EQ(leading_zero, std::nullopt);
	EXPECT_EQ(padded_zero, std::nullopt);
}

/**
 * @test
 * @brief port値型の10進文字列format確認。
 * @details 0、1、代表値、65535を入力し、leading zeroなしの10進文字列を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPortTest, FormatsPortWithoutLeadingZeros)
{
	const auto zero = ket::port::Format(ket::port::Port{0U});
	const auto one = ket::port::Format(ket::port::Port{1U});
	const auto common = ket::port::Format(ket::port::Port{443U});
	const auto maximum = ket::port::Format(ket::port::Port{65535U});

	EXPECT_EQ(zero, std::string("0"));
	EXPECT_EQ(one, std::string("1"));
	EXPECT_EQ(common, std::string("443"));
	EXPECT_EQ(maximum, std::string("65535"));
}
