#include "ket_numeric.h"

#include <limits>
#include <string> // IWYU pragma: keep  // NOLINT(misc-include-cleaner)

#include <gtest/gtest.h>

namespace
{
	static_assert(ket::numeric::InRange<int>(0U), "zero is in signed int range");
	static_assert(!ket::numeric::InRange<unsigned>(-1), "negative value is outside unsigned range");
	static_assert(ket::numeric::Clamp(10, 0, 7) == 7, "Clamp is constexpr");
	static_assert(ket::numeric::AbsDiff(-3, 2) == 5U, "AbsDiff is constexpr");
	static_assert(ket::numeric::AbsDiff(std::numeric_limits<int>::min(),
										std::numeric_limits<int>::max()) ==
					  std::numeric_limits<unsigned>::max(),
				  "AbsDiff covers full signed int span");
	static_assert(ket::numeric::SaturatingAdd(std::numeric_limits<int>::max(), 1) ==
					  std::numeric_limits<int>::max(),
				  "SaturatingAdd upper bound is constexpr");
	static_assert(ket::numeric::SaturatingSub(std::numeric_limits<int>::min(), 1) ==
					  std::numeric_limits<int>::min(),
				  "SaturatingSub lower bound is constexpr");

} // namespace

/**
 * @test
 * @brief 整数範囲判定の境界確認。
 * @details
 * signed/unsignedの組み合わせと変換先の最小最大付近を入力し、範囲内外が戻り値で分かることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetNumericTest, DetectsIntegralRange)
{
	const auto zero_to_signed = ket::numeric::InRange<int>(0U);
	const auto negative_to_unsigned = ket::numeric::InRange<unsigned>(-1);
	const auto max_int_to_short = ket::numeric::InRange<short>(std::numeric_limits<int>::max());
	const auto max_short_to_int = ket::numeric::InRange<int>(std::numeric_limits<short>::max());
	const auto max_unsigned_to_int =
		ket::numeric::InRange<int>(std::numeric_limits<unsigned>::max());

	EXPECT_TRUE(zero_to_signed);
	EXPECT_FALSE(negative_to_unsigned);
	EXPECT_FALSE(max_int_to_short);
	EXPECT_TRUE(max_short_to_int);
	EXPECT_FALSE(max_unsigned_to_int);
}

/**
 * @test
 * @brief clampと絶対差の境界確認。
 * @details 範囲下限、範囲上限、signed最小値を含む差分を入力し、結果が境界へ収まることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetNumericTest, ClampsAndComputesAbsoluteDifference)
{
	const auto below = ket::numeric::Clamp(-1, 0, 10);
	const auto inside = ket::numeric::Clamp(5, 0, 10);
	const auto above = ket::numeric::Clamp(11, 0, 10);
	const auto min_to_zero = ket::numeric::AbsDiff(std::numeric_limits<int>::min(), 0);
	const auto full_span =
		ket::numeric::AbsDiff(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

	EXPECT_EQ(below, 0);
	EXPECT_EQ(inside, 5);
	EXPECT_EQ(above, 10);
	EXPECT_EQ(min_to_zero, static_cast<unsigned>(std::numeric_limits<int>::max()) + 1U);
	EXPECT_EQ(full_span, std::numeric_limits<unsigned>::max());
}

/**
 * @test
 * @brief unsigned整数の切り上げ除算確認。
 * @details divisor 0、1、割り切れる値、余りがある値を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, DividesUnsignedValuesRoundUp)
{
	unsigned out = 99U;
	const auto zero_divisor = ket::numeric::TryDivideRoundUp(10U, 0U, out);
	EXPECT_FALSE(zero_divisor);
	EXPECT_EQ(out, 99U);

	const auto divisor_one = ket::numeric::TryDivideRoundUp(10U, 1U, out);
	EXPECT_TRUE(divisor_one);
	EXPECT_EQ(out, 10U);

	const auto exact = ket::numeric::TryDivideRoundUp(12U, 4U, out);
	EXPECT_TRUE(exact);
	EXPECT_EQ(out, 3U);

	const auto rounded = ket::numeric::TryDivideRoundUp(13U, 4U, out);
	EXPECT_TRUE(rounded);
	EXPECT_EQ(out, 4U);
}

/**
 * @test
 * @brief unsigned整数のalignment切り上げ確認。
 * @details alignment 0、1、整列済み値、overflow境界を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, AlignsUnsignedValuesUp)
{
	const auto max_value = std::numeric_limits<unsigned>::max();
	unsigned out = 99U;

	const auto zero_alignment = ket::numeric::TryAlignUp(10U, 0U, out);
	EXPECT_FALSE(zero_alignment);
	EXPECT_EQ(out, 99U);

	const auto alignment_one = ket::numeric::TryAlignUp(10U, 1U, out);
	EXPECT_TRUE(alignment_one);
	EXPECT_EQ(out, 10U);

	const auto exact = ket::numeric::TryAlignUp(16U, 8U, out);
	EXPECT_TRUE(exact);
	EXPECT_EQ(out, 16U);

	const auto rounded = ket::numeric::TryAlignUp(17U, 8U, out);
	EXPECT_TRUE(rounded);
	EXPECT_EQ(out, 24U);

	out = 99U;
	const auto overflow = ket::numeric::TryAlignUp(max_value - 1U, 4U, out);
	EXPECT_FALSE(overflow);
	EXPECT_EQ(out, 99U);
}

/**
 * @test
 * @brief unsigned整数のalignment切り下げ確認。
 * @details alignment 0、整列済み値、非整列値を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, AlignsUnsignedValuesDown)
{
	unsigned out = 99U;

	const auto zero_alignment = ket::numeric::TryAlignDown(10U, 0U, out);
	EXPECT_FALSE(zero_alignment);
	EXPECT_EQ(out, 99U);

	const auto exact = ket::numeric::TryAlignDown(16U, 8U, out);
	EXPECT_TRUE(exact);
	EXPECT_EQ(out, 16U);

	const auto rounded = ket::numeric::TryAlignDown(17U, 8U, out);
	EXPECT_TRUE(rounded);
	EXPECT_EQ(out, 16U);
}

/**
 * @test
 * @brief checked addのmin/max境界確認。
 * @details signed/unsignedの上限超過とsigned下限超過を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, ChecksAdditionOverflow)
{
	const auto max_int = std::numeric_limits<int>::max();
	const auto min_int = std::numeric_limits<int>::min();
	int signed_out = 123;

	const auto signed_success = ket::numeric::TryAdd(40, 2, signed_out);
	EXPECT_TRUE(signed_success);
	EXPECT_EQ(signed_out, 42);

	signed_out = 123;
	const auto signed_upper_overflow = ket::numeric::TryAdd(max_int, 1, signed_out);
	EXPECT_FALSE(signed_upper_overflow);
	EXPECT_EQ(signed_out, 123);

	const auto signed_lower_overflow = ket::numeric::TryAdd(min_int, -1, signed_out);
	EXPECT_FALSE(signed_lower_overflow);
	EXPECT_EQ(signed_out, 123);

	unsigned unsigned_out = 123U;
	const auto unsigned_overflow =
		ket::numeric::TryAdd(std::numeric_limits<unsigned>::max(), 1U, unsigned_out);
	EXPECT_FALSE(unsigned_overflow);
	EXPECT_EQ(unsigned_out, 123U);
}

/**
 * @test
 * @brief checked subのmin/max境界確認。
 * @details signed/unsignedの下限超過とsigned上限超過を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, ChecksSubtractionOverflow)
{
	const auto max_int = std::numeric_limits<int>::max();
	const auto min_int = std::numeric_limits<int>::min();
	int signed_out = 123;

	const auto signed_success = ket::numeric::TrySub(40, 2, signed_out);
	EXPECT_TRUE(signed_success);
	EXPECT_EQ(signed_out, 38);

	signed_out = 123;
	const auto signed_lower_overflow = ket::numeric::TrySub(min_int, 1, signed_out);
	EXPECT_FALSE(signed_lower_overflow);
	EXPECT_EQ(signed_out, 123);

	const auto signed_upper_overflow = ket::numeric::TrySub(max_int, -1, signed_out);
	EXPECT_FALSE(signed_upper_overflow);
	EXPECT_EQ(signed_out, 123);

	unsigned unsigned_out = 123U;
	const auto unsigned_underflow = ket::numeric::TrySub(0U, 1U, unsigned_out);
	EXPECT_FALSE(unsigned_underflow);
	EXPECT_EQ(unsigned_out, 123U);
}

/**
 * @test
 * @brief checked mulのmin/max境界確認。
 * @details
 * signed/unsignedの乗算overflowとsigned最小値の-1倍を入力し、失敗時は出力が変化しないことを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, ChecksMultiplicationOverflow)
{
	const auto max_int = std::numeric_limits<int>::max();
	const auto min_int = std::numeric_limits<int>::min();
	int signed_out = 123;

	const auto signed_success = ket::numeric::TryMul(6, 7, signed_out);
	EXPECT_TRUE(signed_success);
	EXPECT_EQ(signed_out, 42);

	signed_out = 123;
	const auto signed_positive_overflow = ket::numeric::TryMul(max_int, 2, signed_out);
	EXPECT_FALSE(signed_positive_overflow);
	EXPECT_EQ(signed_out, 123);

	const auto signed_min_negated = ket::numeric::TryMul(min_int, -1, signed_out);
	EXPECT_FALSE(signed_min_negated);
	EXPECT_EQ(signed_out, 123);

	unsigned unsigned_out = 123U;
	const auto unsigned_overflow =
		ket::numeric::TryMul(std::numeric_limits<unsigned>::max(), 2U, unsigned_out);
	EXPECT_FALSE(unsigned_overflow);
	EXPECT_EQ(unsigned_out, 123U);
}

/**
 * @test
 * @brief saturating add/subの上下限確認。
 * @details
 * signed/unsignedの上下限を越える加算と減算を入力し、結果がmin/maxまたは0へ飽和することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetNumericTest, SaturatesAtIntegralBounds)
{
	const auto max_int = std::numeric_limits<int>::max();
	const auto min_int = std::numeric_limits<int>::min();
	const auto signed_add_upper = ket::numeric::SaturatingAdd(max_int, 1);
	const auto signed_add_lower = ket::numeric::SaturatingAdd(min_int, -1);
	const auto signed_sub_lower = ket::numeric::SaturatingSub(min_int, 1);
	const auto signed_sub_upper = ket::numeric::SaturatingSub(max_int, -1);
	const auto unsigned_add = ket::numeric::SaturatingAdd(std::numeric_limits<unsigned>::max(), 1U);
	const auto unsigned_sub = ket::numeric::SaturatingSub(0U, 1U);

	EXPECT_EQ(signed_add_upper, max_int);
	EXPECT_EQ(signed_add_lower, min_int);
	EXPECT_EQ(signed_sub_lower, min_int);
	EXPECT_EQ(signed_sub_upper, max_int);
	EXPECT_EQ(unsigned_add, std::numeric_limits<unsigned>::max());
	EXPECT_EQ(unsigned_sub, 0U);
}

/**
 * @test
 * @brief checked castのsigned/unsigned境界確認。
 * @details signed/unsigned境界とnarrowing境界を入力し、成功時だけ出力が変更されることを確認。
 * @pre C++17以降。
 * @post 成功ケースの出力値だけが期待値へ変化。失敗ケースの出力値は入力時の値を保持。
 */
TEST(KetNumericTest, ChecksCastRange)
{
	unsigned unsigned_out = 99U;
	const auto signed_to_unsigned = ket::numeric::TryCast(-1, unsigned_out);
	EXPECT_FALSE(signed_to_unsigned);
	EXPECT_EQ(unsigned_out, 99U);

	const auto signed_zero_to_unsigned = ket::numeric::TryCast(0, unsigned_out);
	EXPECT_TRUE(signed_zero_to_unsigned);
	EXPECT_EQ(unsigned_out, 0U);

	short short_out = 123;
	const auto int_to_short = ket::numeric::TryCast(std::numeric_limits<int>::max(), short_out);
	EXPECT_FALSE(int_to_short);
	EXPECT_EQ(short_out, 123);

	const auto max_short_to_short =
		ket::numeric::TryCast(std::numeric_limits<short>::max(), short_out);
	EXPECT_TRUE(max_short_to_short);
	EXPECT_EQ(short_out, std::numeric_limits<short>::max());

	unsigned short unsigned_short_out = 123U;
	const auto unsigned_boundary =
		ket::numeric::TryCast(std::numeric_limits<unsigned short>::max(), unsigned_short_out);
	EXPECT_TRUE(unsigned_boundary);
	EXPECT_EQ(unsigned_short_out, std::numeric_limits<unsigned short>::max());

	const auto unsigned_overflow = ket::numeric::TryCast(
		static_cast<unsigned>(std::numeric_limits<unsigned short>::max()) + 1U, unsigned_short_out);
	EXPECT_FALSE(unsigned_overflow);
	EXPECT_EQ(unsigned_short_out, std::numeric_limits<unsigned short>::max());
}
