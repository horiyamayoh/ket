#include "ket_bcd.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	/**
	 * @brief optional整数の期待値一致判定。
	 * @param[in] value 判定対象のoptional整数。
	 * @param[in] expected 期待する整数値。
	 * @retval true `value`が値を持ち、期待値と一致。
	 * @retval false 値なし、または期待値不一致。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	template <typename T>
	constexpr bool OptionalEquals(std::optional<T> value, T expected) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return *value == expected;
	}

	/**
	 * @brief optional整数の空判定。
	 * @param[in] value 判定対象のoptional整数。
	 * @retval true 値なし。
	 * @retval false 値あり。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	template <typename T>
	constexpr bool OptionalIsEmpty(std::optional<T> value) noexcept
	{
		const auto value_has_value = value.has_value();
		return !value_has_value;
	}

	static_assert(OptionalEquals(ket::bcd::ToInt(static_cast<std::uint8_t>(0x42U)), 42),
				  "uint8_t BCD is constexpr");
	static_assert(OptionalEquals(ket::bcd::ToInt(static_cast<std::uint16_t>(0x1234U)), 1234),
				  "uint16_t BCD is constexpr");
	static_assert(OptionalEquals(ket::bcd::ToInt(std::uint32_t{0x20260613U}), 20260613),
				  "uint32_t BCD is constexpr");
	static_assert(OptionalIsEmpty(ket::bcd::ToInt(static_cast<std::uint8_t>(0x0AU))),
				  "invalid BCD is constexpr");
	static_assert(OptionalEquals(ket::bcd::FromInt<std::uint8_t>(42), static_cast<std::uint8_t>(0x42U)),
				  "uint8_t BCD output is constexpr");
	static_assert(OptionalEquals(ket::bcd::FromInt<std::uint16_t>(1234), static_cast<std::uint16_t>(0x1234U)),
				  "uint16_t BCD output is constexpr");
	static_assert(OptionalEquals(ket::bcd::FromInt<std::uint32_t>(20260613), std::uint32_t{0x20260613U}),
				  "uint32_t BCD output is constexpr");
	static_assert(OptionalIsEmpty(ket::bcd::FromInt<std::uint8_t>(100)), "out-of-range BCD output is constexpr");

} // namespace

/**
 * @test
 * @brief 2桁固定幅パックBCDの正常系確認。
 * @details 代表値と境界値を入力し、10進整数へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesUint8Bcd)
{
	const auto zero = ket::bcd::ToInt(static_cast<std::uint8_t>(0x00U));
	const auto nine = ket::bcd::ToInt(static_cast<std::uint8_t>(0x09U));
	const auto ten = ket::bcd::ToInt(static_cast<std::uint8_t>(0x10U));
	const auto ninety_nine = ket::bcd::ToInt(static_cast<std::uint8_t>(0x99U));

	EXPECT_EQ(zero, std::optional<int>(0));
	EXPECT_EQ(nine, std::optional<int>(9));
	EXPECT_EQ(ten, std::optional<int>(10));
	EXPECT_EQ(ninety_nine, std::optional<int>(99));
}

/**
 * @test
 * @brief 4桁固定幅パックBCDの正常系確認。
 * @details 通常値と先頭ゼロを含む値を入力し、10進整数へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesUint16Bcd)
{
	const auto normal = ket::bcd::ToInt(static_cast<std::uint16_t>(0x1234U));
	const auto leading_zero = ket::bcd::ToInt(static_cast<std::uint16_t>(0x0042U));

	EXPECT_EQ(normal, std::optional<int>(1234));
	EXPECT_EQ(leading_zero, std::optional<int>(42));
}

/**
 * @test
 * @brief 8桁固定幅パックBCDの正常系確認。
 * @details 日付形式に見える8桁BCD値を入力し、10進整数へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesUint32Bcd)
{
	const auto date = ket::bcd::ToInt(std::uint32_t{0x20260613U});

	EXPECT_EQ(date, std::optional<int>(20260613));
}

/**
 * @test
 * @brief 2桁固定幅パックBCDの不正nibble拒否確認。
 * @details 下位nibble、上位nibble、両nibbleが不正な値を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsInvalidUint8Bcd)
{
	const auto invalid_low = ket::bcd::ToInt(static_cast<std::uint8_t>(0x0AU));
	const auto invalid_high = ket::bcd::ToInt(static_cast<std::uint8_t>(0xA0U));
	const auto invalid_both = ket::bcd::ToInt(static_cast<std::uint8_t>(0xFFU));

	EXPECT_EQ(invalid_low, std::nullopt);
	EXPECT_EQ(invalid_high, std::nullopt);
	EXPECT_EQ(invalid_both, std::nullopt);
}

/**
 * @test
 * @brief 4桁および8桁固定幅パックBCDの不正nibble拒否確認。
 * @details 中間桁に不正nibbleを含む値を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsInvalidWideBcd)
{
	const auto invalid_uint16 = ket::bcd::ToInt(static_cast<std::uint16_t>(0x12A4U));
	const auto invalid_uint32 = ket::bcd::ToInt(std::uint32_t{0x20260A13U});

	EXPECT_EQ(invalid_uint16, std::nullopt);
	EXPECT_EQ(invalid_uint32, std::nullopt);
}

/**
 * @test
 * @brief 2桁固定幅パックBCDへの10進整数変換確認。
 * @details 代表値と境界値を入力し、2桁packed BCDへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ConvertsIntToUint8Bcd)
{
	const auto zero = ket::bcd::FromInt<std::uint8_t>(0);
	const auto nine = ket::bcd::FromInt<std::uint8_t>(9);
	const auto forty_two = ket::bcd::FromInt<std::uint8_t>(42);
	const auto ninety_nine = ket::bcd::FromInt<std::uint8_t>(99);

	EXPECT_EQ(zero, std::optional<std::uint8_t>(0x00U));
	EXPECT_EQ(nine, std::optional<std::uint8_t>(0x09U));
	EXPECT_EQ(forty_two, std::optional<std::uint8_t>(0x42U));
	EXPECT_EQ(ninety_nine, std::optional<std::uint8_t>(0x99U));
}

/**
 * @test
 * @brief 4桁固定幅パックBCDへの10進整数変換確認。
 * @details 先頭ゼロ表現と最大値を含む値を入力し、4桁packed BCDへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ConvertsIntToUint16Bcd)
{
	const auto leading_zero = ket::bcd::FromInt<std::uint16_t>(42);
	const auto normal = ket::bcd::FromInt<std::uint16_t>(1234);
	const auto maximum = ket::bcd::FromInt<std::uint16_t>(9999);

	EXPECT_EQ(leading_zero, std::optional<std::uint16_t>(0x0042U));
	EXPECT_EQ(normal, std::optional<std::uint16_t>(0x1234U));
	EXPECT_EQ(maximum, std::optional<std::uint16_t>(0x9999U));
}

/**
 * @test
 * @brief 8桁固定幅パックBCDへの10進整数変換確認。
 * @details 日付形式に見える値と最大値を入力し、8桁packed BCDへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ConvertsIntToUint32Bcd)
{
	const auto date = ket::bcd::FromInt<std::uint32_t>(20260613);
	const auto maximum = ket::bcd::FromInt<std::uint32_t>(99999999);

	EXPECT_EQ(date, std::optional<std::uint32_t>(0x20260613U));
	EXPECT_EQ(maximum, std::optional<std::uint32_t>(0x99999999U));
}

/**
 * @test
 * @brief 固定幅パックBCDへの10進整数変換の範囲外拒否確認。
 * @details 負数と各固定幅の桁数を超える値を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsOutOfRangeIntToBcd)
{
	const auto negative_uint8 = ket::bcd::FromInt<std::uint8_t>(-1);
	const auto too_large_uint8 = ket::bcd::FromInt<std::uint8_t>(100);
	const auto negative_uint16 = ket::bcd::FromInt<std::uint16_t>(-1);
	const auto too_large_uint16 = ket::bcd::FromInt<std::uint16_t>(10000);
	const auto negative_uint32 = ket::bcd::FromInt<std::uint32_t>(-1);
	const auto too_large_uint32 = ket::bcd::FromInt<std::uint32_t>(100000000);

	EXPECT_EQ(negative_uint8, std::nullopt);
	EXPECT_EQ(too_large_uint8, std::nullopt);
	EXPECT_EQ(negative_uint16, std::nullopt);
	EXPECT_EQ(too_large_uint16, std::nullopt);
	EXPECT_EQ(negative_uint32, std::nullopt);
	EXPECT_EQ(too_large_uint32, std::nullopt);
}

/**
 * @test
 * @brief 任意バイト長パックBCDの10進文字列変換確認。
 * @details 複数バイト値と先頭ゼロを含む値を入力し、桁を保持した文字列へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, FormatsBcdBytesAsDecimalString)
{
	const auto four_digits = std::array<std::uint8_t, 2>{{0x12U, 0x34U}};
	const auto leading_zero = std::array<std::uint8_t, 2>{{0x00U, 0x42U}};
	const auto date = std::array<std::uint8_t, 4>{{0x20U, 0x26U, 0x06U, 0x13U}};

	const auto four_digits_text = ket::bcd::Format(four_digits.data(), four_digits.size());
	const auto leading_zero_text =
		ket::bcd::Format(leading_zero.data(), leading_zero.size());
	const auto date_text = ket::bcd::Format(date.data(), date.size());

	EXPECT_EQ(four_digits_text, std::optional<std::string>("1234"));
	EXPECT_EQ(leading_zero_text, std::optional<std::string>("0042"));
	EXPECT_EQ(date_text, std::optional<std::string>("20260613"));
}

/**
 * @test
 * @brief 10進文字列の任意バイト長パックBCD変換確認。
 * @details 偶数桁、奇数桁、先頭ゼロを含む文字列を入力し、packed BCD列へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesDecimalStringToBcdBytes)
{
	const auto even_digits = ket::bcd::Parse("1234");
	const auto leading_zero = ket::bcd::Parse("0042");
	const auto odd_digits = ket::bcd::Parse("123");
	const auto single_digit = ket::bcd::Parse("7");

	const auto expected_even_digits = std::optional<std::vector<std::uint8_t>>(
		std::vector<std::uint8_t>{std::uint8_t{0x12U}, std::uint8_t{0x34U}});
	const auto expected_leading_zero = std::optional<std::vector<std::uint8_t>>(
		std::vector<std::uint8_t>{std::uint8_t{0x00U}, std::uint8_t{0x42U}});
	const auto expected_odd_digits = std::optional<std::vector<std::uint8_t>>(
		std::vector<std::uint8_t>{std::uint8_t{0x01U}, std::uint8_t{0x23U}});
	const auto expected_single_digit =
		std::optional<std::vector<std::uint8_t>>(std::vector<std::uint8_t>{std::uint8_t{0x07U}});

	EXPECT_EQ(even_digits, expected_even_digits);
	EXPECT_EQ(leading_zero, expected_leading_zero);
	EXPECT_EQ(odd_digits, expected_odd_digits);
	EXPECT_EQ(single_digit, expected_single_digit);
}

/**
 * @test
 * @brief 10進文字列の任意バイト長パックBCD変換の不正入力拒否確認。
 * @details 空文字と10進数字以外を含む文字列を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsInvalidDecimalStringDuringParse)
{
	const auto empty = ket::bcd::Parse("");
	const auto single_alphabet = ket::bcd::Parse("A");
	const auto alphabet = ket::bcd::Parse("12A4");
	const auto sign = ket::bcd::Parse("-1");
	const auto space = ket::bcd::Parse("12 4");

	EXPECT_EQ(empty, std::nullopt);
	EXPECT_EQ(single_alphabet, std::nullopt);
	EXPECT_EQ(alphabet, std::nullopt);
	EXPECT_EQ(sign, std::nullopt);
	EXPECT_EQ(space, std::nullopt);
}

/**
 * @test
 * @brief 任意バイト長パックBCDの不正nibble拒否確認。
 * @details バイト列中に不正nibbleを含む入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsInvalidBcdBytes)
{
	const auto invalid_first = std::array<std::uint8_t, 2>{{0x1AU, 0x34U}};
	const auto invalid_high_nibble = std::array<std::uint8_t, 2>{{0xA1U, 0x34U}};
	const auto invalid_second = std::array<std::uint8_t, 2>{{0x12U, 0x3AU}};

	const auto invalid_first_text =
		ket::bcd::Format(invalid_first.data(), invalid_first.size());
	const auto invalid_high_nibble_text =
		ket::bcd::Format(invalid_high_nibble.data(), invalid_high_nibble.size());
	const auto invalid_second_text =
		ket::bcd::Format(invalid_second.data(), invalid_second.size());

	EXPECT_EQ(invalid_first_text, std::nullopt);
	EXPECT_EQ(invalid_high_nibble_text, std::nullopt);
	EXPECT_EQ(invalid_second_text, std::nullopt);
}

/**
 * @test
 * @brief 任意バイト長パックBCDの空入力およびnullptr拒否確認。
 * @details nullptrと空入力を渡し、BCD値なしとしてstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, RejectsEmptyOrNullInput)
{
	const auto value = std::uint8_t{0x12U};

	const auto null_input = ket::bcd::Format(nullptr, 1U);
	const auto null_empty_input = ket::bcd::Format(nullptr, 0U);
	const auto empty_input = ket::bcd::Format(&value, 0U);

	EXPECT_EQ(null_input, std::nullopt);
	EXPECT_EQ(null_empty_input, std::nullopt);
	EXPECT_EQ(empty_input, std::nullopt);
}
