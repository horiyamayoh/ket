#include "ket_version.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
	/**
	 * @brief 数値3要素の一致判定。
	 * @param[in] value 判定対象の数値3要素。
	 * @param[in] expected 期待する数値3要素。
	 * @retval true 3要素すべてが一致。
	 * @retval false いずれかの要素が不一致。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	constexpr bool TripletEquals(ket::version::Triplet value,
								 ket::version::Triplet expected) noexcept
	{
		return value.major == expected.major && value.minor == expected.minor &&
			value.patch == expected.patch;
	}

	/**
	 * @brief optional数値3要素の期待値一致判定。
	 * @param[in] value 判定対象のoptional数値3要素。
	 * @param[in] expected 期待する数値3要素。
	 * @retval true `value`が値を持ち、期待値と一致。
	 * @retval false 値なし、または期待値不一致。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	bool OptionalTripletEquals(std::optional<ket::version::Triplet> value,
							   ket::version::Triplet expected) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return TripletEquals(*value, expected);
	}

	/**
	 * @brief optional数値3要素の空判定。
	 * @param[in] value 判定対象のoptional数値3要素。
	 * @retval true 値なし。
	 * @retval false 値あり。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 */
	bool OptionalTripletIsEmpty(std::optional<ket::version::Triplet> value) noexcept
	{
		const auto value_has_value = value.has_value();
		return !value_has_value;
	}

} // namespace

/**
 * @test
 * @brief `0.0.0`のparse確認。
 * @details 各要素が0のnumeric version tripletを入力し、3要素すべてが0としてparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, ParsesZeroTriplet)
{
	const auto parsed = ket::version::Parse("0.0.0");
	const auto parsed_matches = OptionalTripletEquals(parsed, {0U, 0U, 0U});

	EXPECT_TRUE(parsed_matches);
}

/**
 * @test
 * @brief 通常のnumeric version triplet parse確認。
 * @details 複数桁要素を含む`1.20.300`を入力し、major/minor/patchへ分解されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, ParsesNormalTriplet)
{
	const auto parsed = ket::version::Parse("1.20.300");
	const auto parsed_matches = OptionalTripletEquals(parsed, {1U, 20U, 300U});

	EXPECT_TRUE(parsed_matches);
}

/**
 * @test
 * @brief `std::uint32_t`最大値のparse確認。
 * @details 各要素に4294967295を入力し、overflowせず最大値としてparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, ParsesUint32MaxComponents)
{
	const auto max = std::numeric_limits<std::uint32_t>::max();

	const auto parsed = ket::version::Parse("4294967295.4294967295.4294967295");
	const auto parsed_matches = OptionalTripletEquals(parsed, {max, max, max});

	EXPECT_TRUE(parsed_matches);
}

/**
 * @test
 * @brief numeric version triplet format確認。
 * @details 代表値と最大値を文字列化し、`major.minor.patch`形式で出力されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, FormatsTriplet)
{
	const auto normal = ket::version::Format({1U, 20U, 300U});
	const auto max = std::numeric_limits<std::uint32_t>::max();
	const auto maximum = ket::version::Format({max, max, max});

	EXPECT_EQ(normal, std::string("1.20.300"));
	EXPECT_EQ(maximum, std::string("4294967295.4294967295.4294967295"));
}

/**
 * @test
 * @brief major/minor/patch順の比較確認。
 * @details major、minor、patchの各差分と同値を比較し、負/0/正の符号が返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, ComparesMajorMinorPatch)
{
	const auto equal = ket::version::Compare({1U, 2U, 3U}, {1U, 2U, 3U});
	const auto lower_major = ket::version::Compare({1U, 9U, 9U}, {2U, 0U, 0U});
	const auto higher_major = ket::version::Compare({2U, 0U, 0U}, {1U, 9U, 9U});
	const auto lower_minor = ket::version::Compare({1U, 2U, 9U}, {1U, 3U, 0U});
	const auto higher_minor = ket::version::Compare({1U, 3U, 0U}, {1U, 2U, 9U});
	const auto lower_patch = ket::version::Compare({1U, 2U, 3U}, {1U, 2U, 4U});
	const auto higher_patch = ket::version::Compare({1U, 2U, 4U}, {1U, 2U, 3U});

	EXPECT_EQ(equal, 0);
	EXPECT_LT(lower_major, 0);
	EXPECT_GT(higher_major, 0);
	EXPECT_LT(lower_minor, 0);
	EXPECT_GT(higher_minor, 0);
	EXPECT_LT(lower_patch, 0);
	EXPECT_GT(higher_patch, 0);
}

/**
 * @test
 * @brief `std::uint32_t`範囲外要素の拒否確認。
 * @details
 * major/minor/patchそれぞれに4294967296を含め、overflowとしてstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, RejectsOverflowComponents)
{
	const auto overflow_major = ket::version::Parse("4294967296.0.0");
	const auto overflow_minor = ket::version::Parse("0.4294967296.0");
	const auto overflow_patch = ket::version::Parse("0.0.4294967296");

	const auto overflow_major_is_empty = OptionalTripletIsEmpty(overflow_major);
	const auto overflow_minor_is_empty = OptionalTripletIsEmpty(overflow_minor);
	const auto overflow_patch_is_empty = OptionalTripletIsEmpty(overflow_patch);

	EXPECT_TRUE(overflow_major_is_empty);
	EXPECT_TRUE(overflow_minor_is_empty);
	EXPECT_TRUE(overflow_patch_is_empty);
}

/**
 * @test
 * @brief 複数桁leading zeroの拒否確認。
 * @details 各要素に`01`、`02`、`03`を含め、単独の`0`以外のleading zeroを失敗値にすることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, RejectsLeadingZeroComponents)
{
	const auto leading_zero_major = ket::version::Parse("01.2.3");
	const auto leading_zero_minor = ket::version::Parse("1.02.3");
	const auto leading_zero_patch = ket::version::Parse("1.2.03");
	const auto all_zero_digits = ket::version::Parse("00.0.0");

	const auto leading_zero_major_is_empty = OptionalTripletIsEmpty(leading_zero_major);
	const auto leading_zero_minor_is_empty = OptionalTripletIsEmpty(leading_zero_minor);
	const auto leading_zero_patch_is_empty = OptionalTripletIsEmpty(leading_zero_patch);
	const auto all_zero_digits_is_empty = OptionalTripletIsEmpty(all_zero_digits);

	EXPECT_TRUE(leading_zero_major_is_empty);
	EXPECT_TRUE(leading_zero_minor_is_empty);
	EXPECT_TRUE(leading_zero_patch_is_empty);
	EXPECT_TRUE(all_zero_digits_is_empty);
}

/**
 * @test
 * @brief 不正なtriplet形状の拒否確認。
 * @details 空、要素不足/過多、空要素、非数字、符号、空白を含む入力がstd::nulloptになることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVersionTest, RejectsMalformedTriplets)
{
	const auto empty = ket::version::Parse("");
	const auto missing_component = ket::version::Parse("1.2");
	const auto extra_component = ket::version::Parse("1.2.3.4");
	const auto empty_component = ket::version::Parse("1..3");
	const auto trailing_separator = ket::version::Parse("1.2.");
	const auto leading_separator = ket::version::Parse(".1.2");
	const auto alphabet = ket::version::Parse("1.a.3");
	const auto sign = ket::version::Parse("1.-2.3");
	const auto space = ket::version::Parse("1. 2.3");

	const auto empty_is_empty = OptionalTripletIsEmpty(empty);
	const auto missing_component_is_empty = OptionalTripletIsEmpty(missing_component);
	const auto extra_component_is_empty = OptionalTripletIsEmpty(extra_component);
	const auto empty_component_is_empty = OptionalTripletIsEmpty(empty_component);
	const auto trailing_separator_is_empty = OptionalTripletIsEmpty(trailing_separator);
	const auto leading_separator_is_empty = OptionalTripletIsEmpty(leading_separator);
	const auto alphabet_is_empty = OptionalTripletIsEmpty(alphabet);
	const auto sign_is_empty = OptionalTripletIsEmpty(sign);
	const auto space_is_empty = OptionalTripletIsEmpty(space);

	EXPECT_TRUE(empty_is_empty);
	EXPECT_TRUE(missing_component_is_empty);
	EXPECT_TRUE(extra_component_is_empty);
	EXPECT_TRUE(empty_component_is_empty);
	EXPECT_TRUE(trailing_separator_is_empty);
	EXPECT_TRUE(leading_separator_is_empty);
	EXPECT_TRUE(alphabet_is_empty);
	EXPECT_TRUE(sign_is_empty);
	EXPECT_TRUE(space_is_empty);
}
