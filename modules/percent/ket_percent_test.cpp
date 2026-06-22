#include "ket_percent.h"

#include <cstdint>
#include <limits>
#include <string> // NOLINT(misc-include-cleaner)

#include <gtest/gtest.h>

namespace
{
	constexpr ket::percent::Percent kDefaultPercent;

	static_assert(kDefaultPercent.BasisPoints() == 0U, "default percent is constexpr zero");
	static_assert(kDefaultPercent.ToPercent() == 0.0, "percent conversion is constexpr");
	static_assert(kDefaultPercent.ToRatio() == 0.0, "ratio conversion is constexpr");
	static_assert(kDefaultPercent == ket::percent::Percent{}, "equality is constexpr");
	static_assert(!(kDefaultPercent != ket::percent::Percent{}), "inequality is constexpr");
	static_assert(kDefaultPercent <= ket::percent::Percent{}, "less-equal is constexpr");
	static_assert(kDefaultPercent >= ket::percent::Percent{}, "greater-equal is constexpr");

	/**
	 * @brief basis points指定のPercent値生成。
	 * @param[in] basis_points 入力basis points。
	 * @retval value `basis_points`相当のPercent値。
	 * @pre `basis_points <= 10000`。
	 * @post 外部状態の変更なし。
	 */
	ket::percent::Percent MakePercent(std::uint32_t basis_points)
	{
		ket::percent::Percent value;
		const auto created = ket::percent::Percent::TryFromBasisPoints(basis_points, value);
		EXPECT_TRUE(created);

		return value;
	}

} // namespace

/**
 * @test
 * @brief default constructorの0%確認。
 * @details Percentを既定構築し、保持値、percent単位、ratio単位がすべて0になることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPercentTest, DefaultConstructsZeroPercent)
{
	const auto value = ket::percent::Percent{};
	const auto basis_points = value.BasisPoints();
	const auto percent = value.ToPercent();
	const auto ratio = value.ToRatio();

	EXPECT_EQ(basis_points, 0U);
	EXPECT_DOUBLE_EQ(percent, 0.0);
	EXPECT_DOUBLE_EQ(ratio, 0.0);
}

/**
 * @test
 * @brief basis points入力の境界値確認。
 * @details 0、1、10000 basis pointsを入力し、それぞれ保持値として採用されることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 成功した出力先だけが入力basis points相当へ変化。
 */
TEST(KetPercentTest, CreatesFromBasisPointBoundaries)
{
	ket::percent::Percent zero;
	ket::percent::Percent one;
	ket::percent::Percent hundred_percent;

	const auto zero_created = ket::percent::Percent::TryFromBasisPoints(0U, zero);
	const auto one_created = ket::percent::Percent::TryFromBasisPoints(1U, one);
	const auto hundred_created = ket::percent::Percent::TryFromBasisPoints(10000U, hundred_percent);

	EXPECT_TRUE(zero_created);
	EXPECT_TRUE(one_created);
	EXPECT_TRUE(hundred_created);
	EXPECT_EQ(zero.BasisPoints(), 0U);
	EXPECT_EQ(one.BasisPoints(), 1U);
	EXPECT_EQ(hundred_percent.BasisPoints(), 10000U);
}

/**
 * @test
 * @brief basis points入力の範囲外拒否確認。
 * @details 10001 basis
 * pointsと`std::uint32_t`最大値を入力し、falseを返して出力先を保持することを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 失敗した出力先は入力前のPercent値を保持。
 */
TEST(KetPercentTest, RejectsOutOfRangeBasisPointsWithoutChangingOut)
{
	const auto sentinel = MakePercent(1234U);
	auto just_over_out = sentinel;
	auto max_out = sentinel;

	const auto just_over_created = ket::percent::Percent::TryFromBasisPoints(10001U, just_over_out);
	const auto max_created = ket::percent::Percent::TryFromBasisPoints(
		std::numeric_limits<std::uint32_t>::max(), max_out);

	EXPECT_FALSE(just_over_created);
	EXPECT_FALSE(max_created);
	EXPECT_EQ(just_over_out, sentinel);
	EXPECT_EQ(max_out, sentinel);
}

/**
 * @test
 * @brief percent単位入力の丸め確認。
 * @details 0、0.004、0.005、12.345、99.994、99.995、100 percentを入力し、nearest basis
 * pointへ丸められることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 成功した出力先だけが丸め後のPercent値へ変化。
 */
TEST(KetPercentTest, CreatesFromPercentWithNearestBasisPointRounding)
{
	ket::percent::Percent zero;
	ket::percent::Percent rounds_to_zero;
	ket::percent::Percent rounds_to_one;
	ket::percent::Percent fractional;
	ket::percent::Percent rounds_down_near_hundred;
	ket::percent::Percent rounds_to_hundred;
	ket::percent::Percent hundred;

	const auto zero_created = ket::percent::Percent::TryFromPercent(0.0, zero);
	const auto rounds_to_zero_created =
		ket::percent::Percent::TryFromPercent(0.004, rounds_to_zero);
	const auto rounds_to_one_created = ket::percent::Percent::TryFromPercent(0.005, rounds_to_one);
	const auto fractional_created = ket::percent::Percent::TryFromPercent(12.345, fractional);
	const auto rounds_down_near_hundred_created =
		ket::percent::Percent::TryFromPercent(99.994, rounds_down_near_hundred);
	const auto rounds_to_hundred_created =
		ket::percent::Percent::TryFromPercent(99.995, rounds_to_hundred);
	const auto hundred_created = ket::percent::Percent::TryFromPercent(100.0, hundred);

	EXPECT_TRUE(zero_created);
	EXPECT_TRUE(rounds_to_zero_created);
	EXPECT_TRUE(rounds_to_one_created);
	EXPECT_TRUE(fractional_created);
	EXPECT_TRUE(rounds_down_near_hundred_created);
	EXPECT_TRUE(rounds_to_hundred_created);
	EXPECT_TRUE(hundred_created);
	EXPECT_EQ(zero.BasisPoints(), 0U);
	EXPECT_EQ(rounds_to_zero.BasisPoints(), 0U);
	EXPECT_EQ(rounds_to_one.BasisPoints(), 1U);
	EXPECT_EQ(fractional.BasisPoints(), 1235U);
	EXPECT_EQ(rounds_down_near_hundred.BasisPoints(), 9999U);
	EXPECT_EQ(rounds_to_hundred.BasisPoints(), 10000U);
	EXPECT_EQ(hundred.BasisPoints(), 10000U);
}

/**
 * @test
 * @brief percent単位入力の失敗値確認。
 * @details
 * 端点直外、double最大/最小値、NaN、正負Infを入力し、falseを返して出力先を保持することを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 失敗した出力先は入力前のPercent値を保持。
 */
TEST(KetPercentTest, RejectsInvalidPercentInputsWithoutChangingOut)
{
	const auto sentinel = MakePercent(1234U);
	const auto quiet_nan = std::numeric_limits<double>::quiet_NaN();
	const auto signaling_nan = std::numeric_limits<double>::signaling_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();
	const auto double_max = std::numeric_limits<double>::max();
	const auto double_lowest = std::numeric_limits<double>::lowest();
	const auto has_signaling_nan = std::numeric_limits<double>::has_signaling_NaN;

	auto negative_out = sentinel;
	auto just_below_out = sentinel;
	auto too_large_out = sentinel;
	auto just_over_out = sentinel;
	auto max_out = sentinel;
	auto lowest_out = sentinel;
	auto nan_out = sentinel;
	auto signaling_nan_out = sentinel;
	auto positive_infinity_out = sentinel;
	auto negative_infinity_out = sentinel;

	const auto negative_created = ket::percent::Percent::TryFromPercent(-0.01, negative_out);
	const auto just_below_created = ket::percent::Percent::TryFromPercent(-0.0001, just_below_out);
	const auto too_large_created = ket::percent::Percent::TryFromPercent(100.01, too_large_out);
	const auto just_over_created = ket::percent::Percent::TryFromPercent(100.0001, just_over_out);
	const auto max_created = ket::percent::Percent::TryFromPercent(double_max, max_out);
	const auto lowest_created = ket::percent::Percent::TryFromPercent(double_lowest, lowest_out);
	const auto nan_created = ket::percent::Percent::TryFromPercent(quiet_nan, nan_out);
	auto signaling_nan_created = false;
	if (has_signaling_nan)
	{
		signaling_nan_created =
			ket::percent::Percent::TryFromPercent(signaling_nan, signaling_nan_out);
	}
	const auto positive_infinity_created =
		ket::percent::Percent::TryFromPercent(infinity, positive_infinity_out);
	const auto negative_infinity_created =
		ket::percent::Percent::TryFromPercent(-infinity, negative_infinity_out);

	EXPECT_FALSE(negative_created);
	EXPECT_FALSE(just_below_created);
	EXPECT_FALSE(too_large_created);
	EXPECT_FALSE(just_over_created);
	EXPECT_FALSE(max_created);
	EXPECT_FALSE(lowest_created);
	EXPECT_FALSE(nan_created);
	EXPECT_FALSE(signaling_nan_created);
	EXPECT_FALSE(positive_infinity_created);
	EXPECT_FALSE(negative_infinity_created);
	EXPECT_EQ(negative_out, sentinel);
	EXPECT_EQ(just_below_out, sentinel);
	EXPECT_EQ(too_large_out, sentinel);
	EXPECT_EQ(just_over_out, sentinel);
	EXPECT_EQ(max_out, sentinel);
	EXPECT_EQ(lowest_out, sentinel);
	EXPECT_EQ(nan_out, sentinel);
	EXPECT_EQ(signaling_nan_out, sentinel);
	EXPECT_EQ(positive_infinity_out, sentinel);
	EXPECT_EQ(negative_infinity_out, sentinel);
}

/**
 * @test
 * @brief ratio単位入力の丸め確認。
 * @details 0、0.00004、0.00005、0.12345、1 ratioを入力し、nearest basis
 * pointへ丸められることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 成功した出力先だけが丸め後のPercent値へ変化。
 */
TEST(KetPercentTest, CreatesFromRatioWithNearestBasisPointRounding)
{
	ket::percent::Percent zero;
	ket::percent::Percent rounds_to_zero;
	ket::percent::Percent rounds_to_one;
	ket::percent::Percent fractional;
	ket::percent::Percent one;

	const auto zero_created = ket::percent::Percent::TryFromRatio(0.0, zero);
	const auto rounds_to_zero_created =
		ket::percent::Percent::TryFromRatio(0.00004, rounds_to_zero);
	const auto rounds_to_one_created = ket::percent::Percent::TryFromRatio(0.00005, rounds_to_one);
	const auto fractional_created = ket::percent::Percent::TryFromRatio(0.12345, fractional);
	const auto one_created = ket::percent::Percent::TryFromRatio(1.0, one);

	EXPECT_TRUE(zero_created);
	EXPECT_TRUE(rounds_to_zero_created);
	EXPECT_TRUE(rounds_to_one_created);
	EXPECT_TRUE(fractional_created);
	EXPECT_TRUE(one_created);
	EXPECT_EQ(zero.BasisPoints(), 0U);
	EXPECT_EQ(rounds_to_zero.BasisPoints(), 0U);
	EXPECT_EQ(rounds_to_one.BasisPoints(), 1U);
	EXPECT_EQ(fractional.BasisPoints(), 1235U);
	EXPECT_EQ(one.BasisPoints(), 10000U);
}

/**
 * @test
 * @brief ratio単位入力の失敗値確認。
 * @details
 * 端点直外、double最大/最小値、NaN、正負Infを入力し、falseを返して出力先を保持することを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post 失敗した出力先は入力前のPercent値を保持。
 */
TEST(KetPercentTest, RejectsInvalidRatioInputsWithoutChangingOut)
{
	const auto sentinel = MakePercent(1234U);
	const auto quiet_nan = std::numeric_limits<double>::quiet_NaN();
	const auto signaling_nan = std::numeric_limits<double>::signaling_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();
	const auto double_max = std::numeric_limits<double>::max();
	const auto double_lowest = std::numeric_limits<double>::lowest();
	const auto has_signaling_nan = std::numeric_limits<double>::has_signaling_NaN;

	auto negative_out = sentinel;
	auto just_below_out = sentinel;
	auto too_large_out = sentinel;
	auto just_over_out = sentinel;
	auto max_out = sentinel;
	auto lowest_out = sentinel;
	auto nan_out = sentinel;
	auto signaling_nan_out = sentinel;
	auto positive_infinity_out = sentinel;
	auto negative_infinity_out = sentinel;

	const auto negative_created = ket::percent::Percent::TryFromRatio(-0.01, negative_out);
	const auto just_below_created = ket::percent::Percent::TryFromRatio(-0.0001, just_below_out);
	const auto too_large_created = ket::percent::Percent::TryFromRatio(1.01, too_large_out);
	const auto just_over_created = ket::percent::Percent::TryFromRatio(1.0001, just_over_out);
	const auto max_created = ket::percent::Percent::TryFromRatio(double_max, max_out);
	const auto lowest_created = ket::percent::Percent::TryFromRatio(double_lowest, lowest_out);
	const auto nan_created = ket::percent::Percent::TryFromRatio(quiet_nan, nan_out);
	auto signaling_nan_created = false;
	if (has_signaling_nan)
	{
		signaling_nan_created =
			ket::percent::Percent::TryFromRatio(signaling_nan, signaling_nan_out);
	}
	const auto positive_infinity_created =
		ket::percent::Percent::TryFromRatio(infinity, positive_infinity_out);
	const auto negative_infinity_created =
		ket::percent::Percent::TryFromRatio(-infinity, negative_infinity_out);

	EXPECT_FALSE(negative_created);
	EXPECT_FALSE(just_below_created);
	EXPECT_FALSE(too_large_created);
	EXPECT_FALSE(just_over_created);
	EXPECT_FALSE(max_created);
	EXPECT_FALSE(lowest_created);
	EXPECT_FALSE(nan_created);
	EXPECT_FALSE(signaling_nan_created);
	EXPECT_FALSE(positive_infinity_created);
	EXPECT_FALSE(negative_infinity_created);
	EXPECT_EQ(negative_out, sentinel);
	EXPECT_EQ(just_below_out, sentinel);
	EXPECT_EQ(too_large_out, sentinel);
	EXPECT_EQ(just_over_out, sentinel);
	EXPECT_EQ(max_out, sentinel);
	EXPECT_EQ(lowest_out, sentinel);
	EXPECT_EQ(nan_out, sentinel);
	EXPECT_EQ(signaling_nan_out, sentinel);
	EXPECT_EQ(positive_infinity_out, sentinel);
	EXPECT_EQ(negative_infinity_out, sentinel);
}

/**
 * @test
 * @brief percent単位入力のclamp確認。
 * @details 有限値、範囲外、NaN、正負Infを入力し、0〜100%へ丸め込まれることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPercentTest, ClampsPercentInputs)
{
	const auto quiet_nan = std::numeric_limits<double>::quiet_NaN();
	const auto signaling_nan = std::numeric_limits<double>::signaling_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();
	const auto has_signaling_nan = std::numeric_limits<double>::has_signaling_NaN;

	const auto exact_zero = ket::percent::Percent::FromPercentClamped(0.0);
	const auto negative = ket::percent::Percent::FromPercentClamped(-12.0);
	const auto nan = ket::percent::Percent::FromPercentClamped(quiet_nan);
	auto signaling_nan_clamped = ket::percent::Percent::FromPercentClamped(0.0);
	if (has_signaling_nan)
	{
		signaling_nan_clamped = ket::percent::Percent::FromPercentClamped(signaling_nan);
	}
	const auto negative_infinity = ket::percent::Percent::FromPercentClamped(-infinity);
	const auto exact_hundred = ket::percent::Percent::FromPercentClamped(100.0);
	const auto too_large = ket::percent::Percent::FromPercentClamped(120.0);
	const auto positive_infinity = ket::percent::Percent::FromPercentClamped(infinity);
	const auto rounds_to_zero = ket::percent::Percent::FromPercentClamped(0.004);
	const auto rounds_to_one = ket::percent::Percent::FromPercentClamped(0.005);
	const auto fractional = ket::percent::Percent::FromPercentClamped(12.345);
	const auto rounds_down_near_hundred = ket::percent::Percent::FromPercentClamped(99.994);
	const auto rounds_to_hundred = ket::percent::Percent::FromPercentClamped(99.995);

	EXPECT_EQ(exact_zero.BasisPoints(), 0U);
	EXPECT_EQ(negative.BasisPoints(), 0U);
	EXPECT_EQ(nan.BasisPoints(), 0U);
	EXPECT_EQ(signaling_nan_clamped.BasisPoints(), 0U);
	EXPECT_EQ(negative_infinity.BasisPoints(), 0U);
	EXPECT_EQ(exact_hundred.BasisPoints(), 10000U);
	EXPECT_EQ(too_large.BasisPoints(), 10000U);
	EXPECT_EQ(positive_infinity.BasisPoints(), 10000U);
	EXPECT_EQ(rounds_to_zero.BasisPoints(), 0U);
	EXPECT_EQ(rounds_to_one.BasisPoints(), 1U);
	EXPECT_EQ(fractional.BasisPoints(), 1235U);
	EXPECT_EQ(rounds_down_near_hundred.BasisPoints(), 9999U);
	EXPECT_EQ(rounds_to_hundred.BasisPoints(), 10000U);
}

/**
 * @test
 * @brief percent値の単位変換確認。
 * @details 1234 basis pointsを保持する値からpercent単位とratio単位への変換結果を確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPercentTest, ConvertsStoredBasisPointsToPercentAndRatio)
{
	const auto value = MakePercent(1234U);
	const auto percent = value.ToPercent();
	const auto ratio = value.ToRatio();

	EXPECT_DOUBLE_EQ(percent, 12.34);
	EXPECT_DOUBLE_EQ(ratio, 0.1234);
}

/**
 * @test
 * @brief Percent比較演算の整数順序確認。
 * @details 1、1234、10000 basis pointsとDoxygen例の値を比較し、保持basis
 * pointsの整数比較になることを確認。
 * @pre C++17以降のGoogleTest実行環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPercentTest, ComparesByBasisPoints)
{
	const auto low = MakePercent(1U);
	const auto middle = MakePercent(1234U);
	const auto same_middle = MakePercent(1234U);
	const auto high = MakePercent(10000U);
	const auto documented_low = ket::percent::Percent::FromPercentClamped(12.34);
	const auto documented_same = ket::percent::Percent::FromPercentClamped(12.34);
	const auto documented_high = ket::percent::Percent::FromPercentClamped(56.78);

	const auto low_is_less = low < middle;
	const auto middle_is_less_or_equal = middle <= same_middle;
	const auto high_is_greater = high > middle;
	const auto middle_is_greater_or_equal = middle >= same_middle;
	const auto middle_equals_same = middle == same_middle;
	const auto low_not_equals_high = low != high;
	const auto middle_is_less_than_same = middle < same_middle;
	const auto middle_not_equals_same = middle != same_middle;
	const auto low_is_greater = low > middle;
	const auto high_is_less_or_equal = high <= middle;
	const auto documented_same_equal = documented_low == documented_same;
	const auto documented_ordered = documented_low < documented_high;

	EXPECT_TRUE(low_is_less);
	EXPECT_TRUE(middle_is_less_or_equal);
	EXPECT_TRUE(high_is_greater);
	EXPECT_TRUE(middle_is_greater_or_equal);
	EXPECT_TRUE(middle_equals_same);
	EXPECT_TRUE(low_not_equals_high);
	EXPECT_FALSE(middle_is_less_than_same);
	EXPECT_FALSE(middle_not_equals_same);
	EXPECT_FALSE(low_is_greater);
	EXPECT_FALSE(high_is_less_or_equal);
	EXPECT_TRUE(documented_same_equal);
	EXPECT_TRUE(documented_ordered);
}
