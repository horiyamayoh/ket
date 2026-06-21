#include "ket_math.h"

#include <cstdint>
#include <limits>
#include <string> // NOLINT(misc-include-cleaner)

#include <gtest/gtest.h>

namespace
{
	static_assert(ket::math::Lerp(10.0, 20.0, 0.25) == 12.5, "Lerp is constexpr");
	static_assert(ket::math::Lerp(10.0, 20.0, 1.5) == 25.0, "Lerp extrapolates");
	static_assert(ket::math::Lerp(std::numeric_limits<double>::max(),
								  -std::numeric_limits<double>::max(),
								  0.0) == std::numeric_limits<double>::max(),
				  "Lerp preserves t zero endpoint before subtracting endpoints");
	static_assert(ket::math::Lerp(std::numeric_limits<double>::max(),
								  -std::numeric_limits<double>::max(),
								  1.0) == -std::numeric_limits<double>::max(),
				  "Lerp preserves t one endpoint before subtracting endpoints");
	static_assert(ket::math::NearlyEqual(1.0, 1.001, 0.01),
				  "NearlyEqual is constexpr for close values");
	static_assert(!ket::math::NearlyEqual(1.0, 1.0, 0.0),
				  "NearlyEqual rejects zero epsilon in constexpr");
	static_assert(!ket::math::NearlyEqual(1.0, 2.0, std::numeric_limits<double>::infinity()),
				  "NearlyEqual rejects infinite epsilon in constexpr");
	static_assert(ket::math::NearlyEqual(ket::math::ToDegrees(ket::math::ToRadians(90.0)),
										 90.0,
										 0.000000000001),
				  "angle conversion is constexpr");
	static_assert(ket::math::BytesToKiB(std::uint64_t{2048U}) == 2.0, "BytesToKiB is constexpr");
	static_assert(ket::math::BytesToMiB(std::uint64_t{2097152U}) == 2.0, "BytesToMiB is constexpr");

} // namespace

/**
 * @test
 * @brief 線形補間の端点、中点、外挿確認。
 * @details `t`が0、0.5、1、範囲外の値を入力し、通常の浮動小数点演算で求まることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, LerpsEndpointsAndMidpoint)
{
	const auto start = ket::math::Lerp(10.0, 20.0, 0.0);
	const auto midpoint = ket::math::Lerp(10.0, 20.0, 0.5);
	const auto end = ket::math::Lerp(10.0, 20.0, 1.0);
	const auto before_start = ket::math::Lerp(10.0, 20.0, -0.5);
	const auto after_end = ket::math::Lerp(10.0, 20.0, 1.5);
	const auto huge_start = ket::math::Lerp(
		std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), 0.0);
	const auto huge_end = ket::math::Lerp(
		std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), 1.0);

	EXPECT_DOUBLE_EQ(start, 10.0);
	EXPECT_DOUBLE_EQ(midpoint, 15.0);
	EXPECT_DOUBLE_EQ(end, 20.0);
	EXPECT_DOUBLE_EQ(before_start, 5.0);
	EXPECT_DOUBLE_EQ(after_end, 25.0);
	EXPECT_EQ(huge_start, std::numeric_limits<double>::max());
	EXPECT_EQ(huge_end, -std::numeric_limits<double>::max());
}

/**
 * @test
 * @brief degree/radian変換の代表値とroundtrip確認。
 * @details 180 degree、負角、pi radianの変換、および45
 * degreeの往復変換が許容差内に収まることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, ConvertsAnglesBetweenDegreesAndRadians)
{
	const auto pi = 3.141592653589793238462643383279502884;

	const auto radians = ket::math::ToRadians(180.0);
	const auto negative_radians = ket::math::ToRadians(-90.0);
	const auto degrees = ket::math::ToDegrees(pi);
	const auto negative_degrees = ket::math::ToDegrees(-pi / 2.0);
	const auto roundtrip = ket::math::ToDegrees(ket::math::ToRadians(45.0));

	const auto radians_are_pi = ket::math::NearlyEqual(radians, pi, 0.000000000001);
	const auto negative_radians_are_negative_half_pi =
		ket::math::NearlyEqual(negative_radians, -pi / 2.0, 0.000000000001);
	const auto degrees_are_180 = ket::math::NearlyEqual(degrees, 180.0, 0.000000000001);
	const auto negative_degrees_are_negative_90 =
		ket::math::NearlyEqual(negative_degrees, -90.0, 0.000000000001);
	const auto roundtrip_is_45 = ket::math::NearlyEqual(roundtrip, 45.0, 0.000000000001);

	EXPECT_TRUE(radians_are_pi);
	EXPECT_TRUE(negative_radians_are_negative_half_pi);
	EXPECT_TRUE(degrees_are_180);
	EXPECT_TRUE(negative_degrees_are_negative_90);
	EXPECT_TRUE(roundtrip_is_45);
}

/**
 * @test
 * @brief 浮動小数点特殊値の伝播確認。
 * @details
 * Lerpと角度変換へNaNまたはInfを入力し、通常の浮動小数点演算どおり特殊値が伝播することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, PropagatesFloatingPointSpecialValues)
{
	const auto nan = std::numeric_limits<double>::quiet_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();

	const auto lerp_with_nan = ket::math::Lerp(nan, 1.0, 0.5);
	const auto lerp_with_infinity = ket::math::Lerp(1.0, infinity, 0.5);
	const auto nan_radians = ket::math::ToRadians(nan);
	const auto infinity_degrees = ket::math::ToDegrees(infinity);

	const auto lerp_nan_is_nan = lerp_with_nan != lerp_with_nan;
	const auto nan_radians_is_nan = nan_radians != nan_radians;

	EXPECT_TRUE(lerp_nan_is_nan);
	EXPECT_EQ(lerp_with_infinity, infinity);
	EXPECT_TRUE(nan_radians_is_nan);
	EXPECT_EQ(infinity_degrees, infinity);
}

/**
 * @test
 * @brief 正の絶対許容差による近似比較確認。
 * @details 許容差内、許容差ちょうど、許容差外、完全一致を入力し、仕様通りのbool値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, ComparesFloatingPointValuesWithPositiveAbsoluteEpsilon)
{
	const auto within = ket::math::NearlyEqual(1.0, 1.05, 0.1);
	const auto on_boundary = ket::math::NearlyEqual(1.0, 1.125, 0.125);
	const auto outside = ket::math::NearlyEqual(1.0, 1.25, 0.125);
	const auto exact = ket::math::NearlyEqual(-2.0, -2.0, 0.0001);

	EXPECT_TRUE(within);
	EXPECT_TRUE(on_boundary);
	EXPECT_FALSE(outside);
	EXPECT_TRUE(exact);
}

/**
 * @test
 * @brief 非正または非有限許容差の拒否確認。
 * @details
 * 0、負値、NaN、Infのepsilonを入力し、値が完全一致または有限差分でもfalseを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, RejectsNonPositiveOrNonFiniteEpsilon)
{
	const auto nan = std::numeric_limits<double>::quiet_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();

	const auto zero_epsilon = ket::math::NearlyEqual(1.0, 1.0, 0.0);
	const auto negative_epsilon = ket::math::NearlyEqual(1.0, 1.0, -0.1);
	const auto nan_epsilon = ket::math::NearlyEqual(1.0, 1.0, nan);
	const auto infinity_epsilon = ket::math::NearlyEqual(1.0, 2.0, infinity);

	EXPECT_FALSE(zero_epsilon);
	EXPECT_FALSE(negative_epsilon);
	EXPECT_FALSE(nan_epsilon);
	EXPECT_FALSE(infinity_epsilon);
}

/**
 * @test
 * @brief NaN入力の拒否とInf比較の確認。
 * @details
 * 比較対象にNaNを含む場合はfalse、同符号Inf同士はtrue、異符号Inf同士はfalseを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, HandlesNanAndInfinityInNearlyEqual)
{
	const auto nan = std::numeric_limits<double>::quiet_NaN();
	const auto infinity = std::numeric_limits<double>::infinity();

	const auto left_nan = ket::math::NearlyEqual(nan, 1.0, 0.1);
	const auto right_nan = ket::math::NearlyEqual(1.0, nan, 0.1);
	const auto same_infinity = ket::math::NearlyEqual(infinity, infinity, 0.1);
	const auto same_negative_infinity = ket::math::NearlyEqual(-infinity, -infinity, 0.1);
	const auto opposite_infinity = ket::math::NearlyEqual(infinity, -infinity, 0.1);
	const auto finite_to_infinity = ket::math::NearlyEqual(1.0, infinity, 0.1);
	const auto infinity_to_finite = ket::math::NearlyEqual(infinity, 1.0, 0.1);

	EXPECT_FALSE(left_nan);
	EXPECT_FALSE(right_nan);
	EXPECT_TRUE(same_infinity);
	EXPECT_TRUE(same_negative_infinity);
	EXPECT_FALSE(opposite_infinity);
	EXPECT_FALSE(finite_to_infinity);
	EXPECT_FALSE(infinity_to_finite);
}

/**
 * @test
 * @brief KiBからbyteへの変換確認。
 * @details 0、代表値、最大成功値を入力し、overflowしない範囲だけ出力が更新されることを確認。
 * @pre C++17以降。
 * @post 成功時のoutだけ期待するbyte数へ変化。失敗時はoutを保持。
 */
TEST(KetMathTest, ConvertsKiBToBytesWhenItFits)
{
	std::uint64_t zero_bytes = 99U;
	std::uint64_t four_kib_bytes = 99U;
	std::uint64_t max_bytes = 99U;

	const auto max_kib = std::numeric_limits<std::uint64_t>::max() / std::uint64_t{1024U};

	const auto zero_ok = ket::math::TryBytesFromKiB(0U, zero_bytes);
	const auto four_kib_ok = ket::math::TryBytesFromKiB(4U, four_kib_bytes);
	const auto max_ok = ket::math::TryBytesFromKiB(max_kib, max_bytes);

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(four_kib_ok);
	EXPECT_TRUE(max_ok);
	EXPECT_EQ(zero_bytes, std::uint64_t{0U});
	EXPECT_EQ(four_kib_bytes, std::uint64_t{4096U});
	EXPECT_EQ(max_bytes, max_kib * std::uint64_t{1024U});
}

/**
 * @test
 * @brief MiBからbyteへの変換確認。
 * @details 0、代表値、最大成功値を入力し、overflowしない範囲だけ出力が更新されることを確認。
 * @pre C++17以降。
 * @post 成功時のoutだけ期待するbyte数へ変化。失敗時はoutを保持。
 */
TEST(KetMathTest, ConvertsMiBToBytesWhenItFits)
{
	std::uint64_t zero_bytes = 99U;
	std::uint64_t two_mib_bytes = 99U;
	std::uint64_t max_bytes = 99U;

	const auto bytes_per_mib = std::uint64_t{1024U} * std::uint64_t{1024U};
	const auto max_mib = std::numeric_limits<std::uint64_t>::max() / bytes_per_mib;

	const auto zero_ok = ket::math::TryBytesFromMiB(0U, zero_bytes);
	const auto two_mib_ok = ket::math::TryBytesFromMiB(2U, two_mib_bytes);
	const auto max_ok = ket::math::TryBytesFromMiB(max_mib, max_bytes);

	EXPECT_TRUE(zero_ok);
	EXPECT_TRUE(two_mib_ok);
	EXPECT_TRUE(max_ok);
	EXPECT_EQ(zero_bytes, std::uint64_t{0U});
	EXPECT_EQ(two_mib_bytes, std::uint64_t{2097152U});
	EXPECT_EQ(max_bytes, max_mib * bytes_per_mib);
}

/**
 * @test
 * @brief KiB/MiBからbyteへのoverflow拒否確認。
 * @details 最大成功値を1超えるKiB/MiB値を入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetMathTest, RejectsByteConversionOverflowWithoutChangingOutput)
{
	const auto max_kib = std::numeric_limits<std::uint64_t>::max() / std::uint64_t{1024U};
	const auto max_uint64 = std::numeric_limits<std::uint64_t>::max();
	const auto bytes_per_mib = std::uint64_t{1024U} * std::uint64_t{1024U};
	const auto max_mib = std::numeric_limits<std::uint64_t>::max() / bytes_per_mib;

	std::uint64_t kib_out = 123U;
	std::uint64_t max_kib_out = 789U;
	std::uint64_t mib_out = 456U;
	std::uint64_t max_mib_out = 987U;

	const auto kib_ok = ket::math::TryBytesFromKiB(max_kib + 1U, kib_out);
	const auto max_kib_ok = ket::math::TryBytesFromKiB(max_uint64, max_kib_out);
	const auto mib_ok = ket::math::TryBytesFromMiB(max_mib + 1U, mib_out);
	const auto max_mib_ok = ket::math::TryBytesFromMiB(max_uint64, max_mib_out);

	EXPECT_FALSE(kib_ok);
	EXPECT_FALSE(max_kib_ok);
	EXPECT_FALSE(mib_ok);
	EXPECT_FALSE(max_mib_ok);
	EXPECT_EQ(kib_out, std::uint64_t{123U});
	EXPECT_EQ(max_kib_out, std::uint64_t{789U});
	EXPECT_EQ(mib_out, std::uint64_t{456U});
	EXPECT_EQ(max_mib_out, std::uint64_t{987U});
}

/**
 * @test
 * @brief byteからKiB/MiBへのdouble変換確認。
 * @details 代表値と端数を持つbyte数を入力し、1024基準のdouble値へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMathTest, ConvertsBytesToKiBAndMiB)
{
	const auto one_kib = ket::math::BytesToKiB(std::uint64_t{1024U});
	const auto one_point_five_kib = ket::math::BytesToKiB(std::uint64_t{1536U});
	const auto one_mib = ket::math::BytesToMiB(std::uint64_t{1048576U});
	const auto one_point_five_mib = ket::math::BytesToMiB(std::uint64_t{1572864U});
	const auto zero_kib = ket::math::BytesToKiB(std::uint64_t{0U});
	const auto zero_mib = ket::math::BytesToMiB(std::uint64_t{0U});
	const auto max_bytes = std::numeric_limits<std::uint64_t>::max();
	const auto max_bytes_as_kib = ket::math::BytesToKiB(max_bytes);
	const auto max_bytes_as_mib = ket::math::BytesToMiB(max_bytes);

	EXPECT_DOUBLE_EQ(one_kib, 1.0);
	EXPECT_DOUBLE_EQ(one_point_five_kib, 1.5);
	EXPECT_DOUBLE_EQ(one_mib, 1.0);
	EXPECT_DOUBLE_EQ(one_point_five_mib, 1.5);
	EXPECT_DOUBLE_EQ(zero_kib, 0.0);
	EXPECT_DOUBLE_EQ(zero_mib, 0.0);
	EXPECT_DOUBLE_EQ(max_bytes_as_kib, static_cast<double>(max_bytes) / 1024.0);
	EXPECT_DOUBLE_EQ(max_bytes_as_mib, static_cast<double>(max_bytes) / 1048576.0);
}
