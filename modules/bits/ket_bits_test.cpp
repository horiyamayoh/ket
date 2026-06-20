#include "ket_bits.h"

#include <cstdint>
#include <limits>
// NOLINTNEXTLINE(misc-include-cleaner): GoogleTest printer path requires this for IWYU.
#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

namespace
{
	static_assert(ket::bits::IsNibble(static_cast<std::uint8_t>(0x0FU)), "max nibble is constexpr");
	static_assert(!ket::bits::IsNibble(static_cast<std::uint8_t>(0x10U)),
				  "out-of-range nibble is constexpr");
	static_assert(ket::bits::HighNibble(static_cast<std::uint8_t>(0xABU)) ==
					  static_cast<std::uint8_t>(0x0AU),
				  "high nibble is constexpr");
	static_assert(ket::bits::LowNibble(static_cast<std::uint8_t>(0xABU)) ==
					  static_cast<std::uint8_t>(0x0BU),
				  "low nibble is constexpr");
	static_assert(ket::bits::TypeBitWidth<std::uint8_t>() == 8U, "uint8_t bit width");
	static_assert(ket::bits::TypeBitWidth<std::uint16_t>() == 16U, "uint16_t bit width");
	static_assert(ket::bits::TypeBitWidth<std::uint32_t>() == 32U, "uint32_t bit width");
	static_assert(ket::bits::TypeBitWidth<std::uint64_t>() == 64U, "uint64_t bit width");
	static_assert(ket::bits::HasBit<std::uint8_t>(static_cast<std::uint8_t>(0x80U), 7U),
				  "high bit is constexpr");
	static_assert(!ket::bits::HasBit<std::uint8_t>(static_cast<std::uint8_t>(0x80U), 8U),
				  "out-of-range bit is constexpr");
	static_assert(ket::bits::PopCount<std::uint8_t>(static_cast<std::uint8_t>(0xFFU)) == 8U,
				  "popcount is constexpr");
	static_assert(ket::bits::IsPowerOfTwo<std::uint8_t>(static_cast<std::uint8_t>(0x80U)),
				  "power-of-two is constexpr");
	static_assert(!ket::bits::IsPowerOfTwo<std::uint8_t>(static_cast<std::uint8_t>(0x81U)),
				  "non-power-of-two is constexpr");
	static_assert(ket::bits::Rotl<std::uint8_t>(static_cast<std::uint8_t>(0x81U), 1U) ==
					  static_cast<std::uint8_t>(0x03U),
				  "left rotate is constexpr");
	static_assert(ket::bits::Rotr<std::uint8_t>(static_cast<std::uint8_t>(0x81U), 1U) ==
					  static_cast<std::uint8_t>(0xC0U),
				  "right rotate is constexpr");

} // namespace

/**
 * @test
 * @brief nibble境界と抽出結果の確認。
 * @details nibbleの最小値、最大値、範囲外値を判定し、0xABから上位/下位nibbleを取り出すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, SplitsAndValidatesNibbles)
{
	const auto zero_is_nibble = ket::bits::IsNibble(static_cast<std::uint8_t>(0x00U));
	const auto max_is_nibble = ket::bits::IsNibble(static_cast<std::uint8_t>(0x0FU));
	const auto outside_is_nibble = ket::bits::IsNibble(static_cast<std::uint8_t>(0x10U));
	const auto high = ket::bits::HighNibble(static_cast<std::uint8_t>(0xABU));
	const auto low = ket::bits::LowNibble(static_cast<std::uint8_t>(0xABU));

	EXPECT_TRUE(zero_is_nibble);
	EXPECT_TRUE(max_is_nibble);
	EXPECT_FALSE(outside_is_nibble);
	EXPECT_EQ(high, static_cast<std::uint8_t>(0x0AU));
	EXPECT_EQ(low, static_cast<std::uint8_t>(0x0BU));
}

/**
 * @test
 * @brief 2つのnibbleからのbyte生成確認。
 * @details 正常なnibbleを入力するとpacked
 * byteを生成し、不正nibbleではfalseを返して出力を変えないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力だけ期待値へ変化。失敗ケースの出力は入力時の値を保持。
 */
TEST(KetBitsTest, PacksByteOnlyFromValidNibbles)
{
	std::uint8_t packed = 0xEEU;
	std::uint8_t invalid_high = 0xEEU;
	std::uint8_t invalid_low = 0xEEU;

	const auto packed_ok = ket::bits::TryPackNibbles(0x0AU, 0x0BU, packed);
	const auto invalid_high_ok = ket::bits::TryPackNibbles(0x10U, 0x00U, invalid_high);
	const auto invalid_low_ok = ket::bits::TryPackNibbles(0x00U, 0x10U, invalid_low);

	EXPECT_TRUE(packed_ok);
	EXPECT_EQ(packed, static_cast<std::uint8_t>(0xABU));
	EXPECT_FALSE(invalid_high_ok);
	EXPECT_FALSE(invalid_low_ok);
	EXPECT_EQ(invalid_high, static_cast<std::uint8_t>(0xEEU));
	EXPECT_EQ(invalid_low, static_cast<std::uint8_t>(0xEEU));
}

/**
 * @test
 * @brief bit幅とbit存在判定の境界確認。
 * @details bit 0、最上位bit、範囲外bitを判定し、範囲外はfalseへ倒れることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, ReportsTypeBitWidthAndHasBitBoundaries)
{
	const auto uint8_width = ket::bits::TypeBitWidth<std::uint8_t>();
	const auto uint16_width = ket::bits::TypeBitWidth<std::uint16_t>();
	const auto has_low_bit = ket::bits::HasBit<std::uint8_t>(0x81U, 0U);
	const auto has_high_bit = ket::bits::HasBit<std::uint8_t>(0x81U, 7U);
	const auto has_clear_bit = ket::bits::HasBit<std::uint8_t>(0x81U, 1U);
	const auto has_out_of_range_bit = ket::bits::HasBit<std::uint8_t>(0x81U, 8U);

	EXPECT_EQ(uint8_width, 8U);
	EXPECT_EQ(uint16_width, 16U);
	EXPECT_TRUE(has_low_bit);
	EXPECT_TRUE(has_high_bit);
	EXPECT_FALSE(has_clear_bit);
	EXPECT_FALSE(has_out_of_range_bit);
}

/**
 * @test
 * @brief bit更新APIの境界と出力不変確認。
 * @details bit
 * set、clear、toggleの正常系と範囲外bitを入力し、範囲外では出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力だけ期待値へ変化。失敗ケースの出力は入力時の値を保持。
 */
TEST(KetBitsTest, UpdatesBitsOnlyWhenIndexIsInRange)
{
	std::uint8_t set_low = 0xEEU;
	std::uint8_t clear_high = 0xEEU;
	std::uint8_t toggle_high = 0xEEU;
	std::uint8_t invalid_set = 0xCCU;
	std::uint8_t invalid_clear = 0xCCU;
	std::uint8_t invalid_toggle = 0xCCU;

	const auto set_low_ok = ket::bits::TrySetBit<std::uint8_t>(0x80U, 0U, set_low);
	const auto clear_high_ok = ket::bits::TryClearBit<std::uint8_t>(0xFFU, 7U, clear_high);
	const auto toggle_high_ok = ket::bits::TryToggleBit<std::uint8_t>(0x80U, 7U, toggle_high);
	const auto invalid_set_ok = ket::bits::TrySetBit<std::uint8_t>(0x00U, 8U, invalid_set);
	const auto invalid_clear_ok = ket::bits::TryClearBit<std::uint8_t>(0xFFU, 8U, invalid_clear);
	const auto invalid_toggle_ok = ket::bits::TryToggleBit<std::uint8_t>(0x00U, 8U, invalid_toggle);

	EXPECT_TRUE(set_low_ok);
	EXPECT_TRUE(clear_high_ok);
	EXPECT_TRUE(toggle_high_ok);
	EXPECT_EQ(set_low, static_cast<std::uint8_t>(0x81U));
	EXPECT_EQ(clear_high, static_cast<std::uint8_t>(0x7FU));
	EXPECT_EQ(toggle_high, static_cast<std::uint8_t>(0x00U));
	EXPECT_FALSE(invalid_set_ok);
	EXPECT_FALSE(invalid_clear_ok);
	EXPECT_FALSE(invalid_toggle_ok);
	EXPECT_EQ(invalid_set, static_cast<std::uint8_t>(0xCCU));
	EXPECT_EQ(invalid_clear, static_cast<std::uint8_t>(0xCCU));
	EXPECT_EQ(invalid_toggle, static_cast<std::uint8_t>(0xCCU));
}

/**
 * @test
 * @brief mask生成の幅境界確認。
 * @details 幅0、部分幅、型の全幅、超過幅を入力し、超過幅だけfalseで出力不変となることを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力だけ期待値へ変化。失敗ケースの出力は入力時の値を保持。
 */
TEST(KetBitsTest, CreatesMasksForValidWidths)
{
	std::uint8_t zero_width = 0xEEU;
	std::uint8_t four_width = 0xEEU;
	std::uint8_t full_width = 0xEEU;
	std::uint8_t too_wide = 0xEEU;
	std::uint16_t full_uint16 = 0xEEEEU;

	const auto zero_width_ok = ket::bits::TryMask<std::uint8_t>(0U, zero_width);
	const auto four_width_ok = ket::bits::TryMask<std::uint8_t>(4U, four_width);
	const auto full_width_ok = ket::bits::TryMask<std::uint8_t>(8U, full_width);
	const auto too_wide_ok = ket::bits::TryMask<std::uint8_t>(9U, too_wide);
	const auto full_uint16_ok = ket::bits::TryMask<std::uint16_t>(16U, full_uint16);

	EXPECT_TRUE(zero_width_ok);
	EXPECT_TRUE(four_width_ok);
	EXPECT_TRUE(full_width_ok);
	EXPECT_FALSE(too_wide_ok);
	EXPECT_TRUE(full_uint16_ok);
	EXPECT_EQ(zero_width, static_cast<std::uint8_t>(0x00U));
	EXPECT_EQ(four_width, static_cast<std::uint8_t>(0x0FU));
	EXPECT_EQ(full_width, std::numeric_limits<std::uint8_t>::max());
	EXPECT_EQ(too_wide, static_cast<std::uint8_t>(0xEEU));
	EXPECT_EQ(full_uint16, std::numeric_limits<std::uint16_t>::max());
}

/**
 * @test
 * @brief popcountと2の累乗判定の境界確認。
 * @details 0、1、複数bit、全bitを入力し、立っているbit数と2の累乗判定が期待値となることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, CountsBitsAndDetectsPowerOfTwo)
{
	const auto zero_count = ket::bits::PopCount<std::uint8_t>(0x00U);
	const auto one_count = ket::bits::PopCount<std::uint8_t>(0x01U);
	const auto mixed_count = ket::bits::PopCount<std::uint8_t>(0x81U);
	const auto full_count = ket::bits::PopCount<std::uint8_t>(0xFFU);
	const auto zero_is_power = ket::bits::IsPowerOfTwo<std::uint8_t>(0x00U);
	const auto one_is_power = ket::bits::IsPowerOfTwo<std::uint8_t>(0x01U);
	const auto high_is_power = ket::bits::IsPowerOfTwo<std::uint8_t>(0x80U);
	const auto mixed_is_power = ket::bits::IsPowerOfTwo<std::uint8_t>(0x81U);

	EXPECT_EQ(zero_count, 0U);
	EXPECT_EQ(one_count, 1U);
	EXPECT_EQ(mixed_count, 2U);
	EXPECT_EQ(full_count, 8U);
	EXPECT_FALSE(zero_is_power);
	EXPECT_TRUE(one_is_power);
	EXPECT_TRUE(high_is_power);
	EXPECT_FALSE(mixed_is_power);
}

/**
 * @test
 * @brief rotateの0幅、通常幅、幅以上count確認。
 * @details count 0、1、bit幅以上を入力し、countがbit幅で剰余化されてrotateされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, RotatesWithNormalizedCount)
{
	const auto value = static_cast<std::uint8_t>(0x81U);

	const auto left_zero = ket::bits::Rotl<std::uint8_t>(value, 0U);
	const auto left_one = ket::bits::Rotl<std::uint8_t>(value, 1U);
	const auto left_width_plus_one = ket::bits::Rotl<std::uint8_t>(value, 9U);
	const auto right_zero = ket::bits::Rotr<std::uint8_t>(value, 0U);
	const auto right_one = ket::bits::Rotr<std::uint8_t>(value, 1U);
	const auto right_width_plus_one = ket::bits::Rotr<std::uint8_t>(value, 9U);

	EXPECT_EQ(left_zero, value);
	EXPECT_EQ(left_one, static_cast<std::uint8_t>(0x03U));
	EXPECT_EQ(left_width_plus_one, static_cast<std::uint8_t>(0x03U));
	EXPECT_EQ(right_zero, value);
	EXPECT_EQ(right_one, static_cast<std::uint8_t>(0xC0U));
	EXPECT_EQ(right_width_plus_one, static_cast<std::uint8_t>(0xC0U));
}
