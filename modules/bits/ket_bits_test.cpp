#include "ket_bits.h"

#include <cstdint>
#include <limits>
#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

namespace
{
	static_assert(ket::IsNibble(0x00U), "zero is a constexpr nibble");
	static_assert(ket::IsNibble(0x0FU), "0x0F is a constexpr nibble");
	static_assert(!ket::IsNibble(0x10U), "0x10 is not a constexpr nibble");
	static_assert(ket::HighNibble(0xABU) == 0x0AU, "high nibble is constexpr");
	static_assert(ket::LowNibble(0xABU) == 0x0BU, "low nibble is constexpr");
	static_assert(ket::BitWidth<std::uint8_t>() == 8U, "uint8_t bit width is constexpr");
	static_assert(ket::HasBit<std::uint8_t>(0x80U, 7U), "HasBit is constexpr");
	static_assert(!ket::HasBit<std::uint8_t>(0x80U, 8U), "out-of-range HasBit is constexpr");
	static_assert(ket::PopCount<std::uint8_t>(0x81U) == 2U, "PopCount is constexpr");
	static_assert(ket::IsPowerOfTwo<std::uint8_t>(0x80U), "IsPowerOfTwo is constexpr");
	static_assert(!ket::IsPowerOfTwo<std::uint8_t>(0U), "zero is not a power of two");
	static_assert(ket::Rotl<std::uint8_t>(0x81U, 1U) == 0x03U, "Rotl is constexpr");
	static_assert(ket::Rotr<std::uint8_t>(0x81U, 1U) == 0xC0U, "Rotr is constexpr");

} // namespace

/**
 * @test
 * @brief nibble境界とbyte分割の確認。
 * @details 0x00、0x0F、0x10、0xFFを入力し、4bit nibble範囲の境界を固定。
 * 代表byte 0xABと0x00/0xFFから上位・下位nibbleが取得されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, ChecksNibbleBoundariesAndExtractsNibbles)
{
	const auto zero_is_nibble = ket::IsNibble(0x00U);
	const auto max_is_nibble = ket::IsNibble(0x0FU);
	const auto overflow_is_nibble = ket::IsNibble(0x10U);
	const auto byte_max_is_nibble = ket::IsNibble(0xFFU);
	const auto high_ab = ket::HighNibble(0xABU);
	const auto low_ab = ket::LowNibble(0xABU);
	const auto high_zero = ket::HighNibble(0x00U);
	const auto low_max = ket::LowNibble(0xFFU);

	EXPECT_TRUE(zero_is_nibble);
	EXPECT_TRUE(max_is_nibble);
	EXPECT_FALSE(overflow_is_nibble);
	EXPECT_FALSE(byte_max_is_nibble);
	EXPECT_EQ(high_ab, 0x0AU);
	EXPECT_EQ(low_ab, 0x0BU);
	EXPECT_EQ(high_zero, 0x00U);
	EXPECT_EQ(low_max, 0x0FU);
}

/**
 * @test
 * @brief 2つのnibbleからのbyte生成確認。
 * @details 正常な上下nibbleでbyteが生成されることを確認。
 * 上位または下位が範囲外の場合にfalseを返し、出力値を保持することも確認。
 * @pre C++17以降。
 * @post 成功時だけ出力値が変化し、失敗時は入力時の出力値を保持。
 */
TEST(KetBitsTest, MakesByteFromNibblesAndPreservesOutputOnFailure)
{
	std::uint8_t value = 0x00U;
	const auto made_byte = ket::TryMakeByteFromNibbles(0x0AU, 0x0BU, value);

	std::uint8_t invalid_high_output = 0xCCU;
	const auto rejected_high = ket::TryMakeByteFromNibbles(0x10U, 0x00U, invalid_high_output);

	std::uint8_t invalid_low_output = 0xDDU;
	const auto rejected_low = ket::TryMakeByteFromNibbles(0x00U, 0xFFU, invalid_low_output);

	EXPECT_TRUE(made_byte);
	EXPECT_EQ(value, 0xABU);
	EXPECT_FALSE(rejected_high);
	EXPECT_EQ(invalid_high_output, 0xCCU);
	EXPECT_FALSE(rejected_low);
	EXPECT_EQ(invalid_low_output, 0xDDU);
}

/**
 * @test
 * @brief bit幅とbit有無判定の境界確認。
 * @details std::uint8_tとstd::uint16_tのbit幅を確認。
 * bit 0、最上位bit、0のbit、範囲外bit indexでHasBitの戻り値を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, ReportsBitWidthAndChecksBitPresence)
{
	const auto uint8_width = ket::BitWidth<std::uint8_t>();
	const auto uint16_width = ket::BitWidth<std::uint16_t>();
	const auto has_low_bit = ket::HasBit<std::uint8_t>(0x81U, 0U);
	const auto has_high_bit = ket::HasBit<std::uint8_t>(0x81U, 7U);
	const auto has_clear_bit = ket::HasBit<std::uint8_t>(0x81U, 6U);
	const auto has_out_of_range_bit = ket::HasBit<std::uint8_t>(0x81U, 8U);
	const auto has_uint16_high_bit = ket::HasBit<std::uint16_t>(0x8000U, 15U);
	const auto has_uint16_out_of_range_bit = ket::HasBit<std::uint16_t>(0x8000U, 16U);

	EXPECT_EQ(uint8_width, 8U);
	EXPECT_EQ(uint16_width, 16U);
	EXPECT_TRUE(has_low_bit);
	EXPECT_TRUE(has_high_bit);
	EXPECT_FALSE(has_clear_bit);
	EXPECT_FALSE(has_out_of_range_bit);
	EXPECT_TRUE(has_uint16_high_bit);
	EXPECT_FALSE(has_uint16_out_of_range_bit);
}

/**
 * @test
 * @brief bit set/clear/toggleの境界確認。
 * @details bit 0と最上位bitを更新し、期待する値が出力されることを確認。
 * 範囲外bit indexではfalseを返し、出力値を保持することも確認。
 * @pre C++17以降。
 * @post 成功時だけ出力値が変化し、失敗時は入力時の出力値を保持。
 */
TEST(KetBitsTest, SetsClearsAndTogglesBits)
{
	std::uint8_t set_low = 0x00U;
	const auto set_low_succeeded = ket::TrySetBit<std::uint8_t>(0x00U, 0U, set_low);

	std::uint8_t set_high = 0x00U;
	const auto set_high_succeeded = ket::TrySetBit<std::uint8_t>(0x00U, 7U, set_high);

	std::uint8_t clear_high = 0x00U;
	const auto clear_high_succeeded = ket::TryClearBit<std::uint8_t>(0xFFU, 7U, clear_high);

	std::uint8_t toggle_middle = 0x00U;
	const auto toggle_middle_succeeded = ket::TryToggleBit<std::uint8_t>(0x03U, 1U, toggle_middle);

	std::uint8_t invalid_output = 0xAAU;
	const auto invalid_set_succeeded = ket::TrySetBit<std::uint8_t>(0x00U, 8U, invalid_output);
	const auto invalid_clear_succeeded = ket::TryClearBit<std::uint8_t>(0xFFU, 8U, invalid_output);
	const auto invalid_toggle_succeeded =
		ket::TryToggleBit<std::uint8_t>(0x00U, 8U, invalid_output);

	EXPECT_TRUE(set_low_succeeded);
	EXPECT_EQ(set_low, 0x01U);
	EXPECT_TRUE(set_high_succeeded);
	EXPECT_EQ(set_high, 0x80U);
	EXPECT_TRUE(clear_high_succeeded);
	EXPECT_EQ(clear_high, 0x7FU);
	EXPECT_TRUE(toggle_middle_succeeded);
	EXPECT_EQ(toggle_middle, 0x01U);
	EXPECT_FALSE(invalid_set_succeeded);
	EXPECT_FALSE(invalid_clear_succeeded);
	EXPECT_FALSE(invalid_toggle_succeeded);
	EXPECT_EQ(invalid_output, 0xAAU);
}

/**
 * @test
 * @brief mask生成の境界確認。
 * @details width 0、1、4、bit幅fullで成功し、期待maskが生成されることを確認。
 * bit幅超過ではfalseを返し、出力値を保持することも確認。
 * @pre C++17以降。
 * @post 成功時だけ出力値が変化し、失敗時は入力時の出力値を保持。
 */
TEST(KetBitsTest, MakesMasksAtBoundaryWidths)
{
	std::uint8_t zero_mask = 0xFFU;
	const auto zero_mask_succeeded = ket::TryMask<std::uint8_t>(0U, zero_mask);

	std::uint8_t one_bit_mask = 0x00U;
	const auto one_bit_mask_succeeded = ket::TryMask<std::uint8_t>(1U, one_bit_mask);

	std::uint8_t nibble_mask = 0x00U;
	const auto nibble_mask_succeeded = ket::TryMask<std::uint8_t>(4U, nibble_mask);

	std::uint8_t full_mask = 0x00U;
	const auto full_mask_succeeded = ket::TryMask<std::uint8_t>(8U, full_mask);

	std::uint32_t full_uint32_mask = 0U;
	const auto full_uint32_mask_succeeded = ket::TryMask<std::uint32_t>(32U, full_uint32_mask);

	std::uint8_t overflow_mask = 0xAAU;
	const auto overflow_mask_succeeded = ket::TryMask<std::uint8_t>(9U, overflow_mask);

	EXPECT_TRUE(zero_mask_succeeded);
	EXPECT_EQ(zero_mask, 0x00U);
	EXPECT_TRUE(one_bit_mask_succeeded);
	EXPECT_EQ(one_bit_mask, 0x01U);
	EXPECT_TRUE(nibble_mask_succeeded);
	EXPECT_EQ(nibble_mask, 0x0FU);
	EXPECT_TRUE(full_mask_succeeded);
	EXPECT_EQ(full_mask, 0xFFU);
	EXPECT_TRUE(full_uint32_mask_succeeded);
	EXPECT_EQ(full_uint32_mask, std::numeric_limits<std::uint32_t>::max());
	EXPECT_FALSE(overflow_mask_succeeded);
	EXPECT_EQ(overflow_mask, 0xAAU);
}

/**
 * @test
 * @brief popcountと2の冪判定の確認。
 * @details 0、全bit 1、複数bit値のpopcountを確認。
 * 0、2の冪、2の冪ではない値でIsPowerOfTwoの戻り値を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, CountsBitsAndDetectsPowerOfTwoValues)
{
	const auto zero_count = ket::PopCount<std::uint8_t>(0x00U);
	const auto full_count = ket::PopCount<std::uint8_t>(0xFFU);
	const auto sparse_count = ket::PopCount<std::uint8_t>(0x81U);
	const auto uint32_count = ket::PopCount<std::uint32_t>(0x80000001U);
	const auto zero_is_power = ket::IsPowerOfTwo<std::uint8_t>(0U);
	const auto one_is_power = ket::IsPowerOfTwo<std::uint8_t>(1U);
	const auto high_bit_is_power = ket::IsPowerOfTwo<std::uint8_t>(0x80U);
	const auto three_is_power = ket::IsPowerOfTwo<std::uint8_t>(3U);

	EXPECT_EQ(zero_count, 0U);
	EXPECT_EQ(full_count, 8U);
	EXPECT_EQ(sparse_count, 2U);
	EXPECT_EQ(uint32_count, 2U);
	EXPECT_FALSE(zero_is_power);
	EXPECT_TRUE(one_is_power);
	EXPECT_TRUE(high_bit_is_power);
	EXPECT_FALSE(three_is_power);
}

/**
 * @test
 * @brief rotateの0回転とbit幅以上回転の確認。
 * @details std::uint8_tとstd::uint16_tで左rotateと右rotateを確認。
 * count 0、1、bit幅、bit幅+1でcountがbit幅により剰余化されることを固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBitsTest, RotatesLeftAndRight)
{
	const auto rotl_zero = ket::Rotl<std::uint8_t>(0x81U, 0U);
	const auto rotl_one = ket::Rotl<std::uint8_t>(0x81U, 1U);
	const auto rotl_full_width = ket::Rotl<std::uint8_t>(0x81U, 8U);
	const auto rotl_over_width = ket::Rotl<std::uint8_t>(0x81U, 9U);
	const auto rotr_zero = ket::Rotr<std::uint8_t>(0x81U, 0U);
	const auto rotr_one = ket::Rotr<std::uint8_t>(0x81U, 1U);
	const auto rotr_full_width = ket::Rotr<std::uint8_t>(0x81U, 8U);
	const auto rotr_over_width = ket::Rotr<std::uint8_t>(0x81U, 9U);
	const auto rotl_uint16 = ket::Rotl<std::uint16_t>(0x8001U, 1U);
	const auto rotr_uint16 = ket::Rotr<std::uint16_t>(0x8001U, 1U);

	EXPECT_EQ(rotl_zero, 0x81U);
	EXPECT_EQ(rotl_one, 0x03U);
	EXPECT_EQ(rotl_full_width, 0x81U);
	EXPECT_EQ(rotl_over_width, 0x03U);
	EXPECT_EQ(rotr_zero, 0x81U);
	EXPECT_EQ(rotr_one, 0xC0U);
	EXPECT_EQ(rotr_full_width, 0x81U);
	EXPECT_EQ(rotr_over_width, 0xC0U);
	EXPECT_EQ(rotl_uint16, 0x0003U);
	EXPECT_EQ(rotr_uint16, 0xC000U);
}
