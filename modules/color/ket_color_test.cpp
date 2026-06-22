#include "ket_color.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace
{
	ket::color::Rgb MakeRgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept
	{
		return {r, g, b};
	}

} // namespace

/**
 * @test
 * @brief RGB値の既定初期化確認。
 * @details default constructed RGBがblackを表すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetColorTest, DefaultConstructsBlack)
{
	const ket::color::Rgb color;
	const auto matches_black = color == MakeRgb(0x00U, 0x00U, 0x00U);

	EXPECT_TRUE(matches_black);
}

/**
 * @test
 * @brief RGB値のchannel指定生成確認。
 * @details C++11でもchannel値をconstructorで直接渡せることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetColorTest, ConstructsFromChannelValues)
{
	const auto color = MakeRgb(0x0aU, 0xffU, 0x10U);
	const auto red_matches = color.r == 0x0aU;
	const auto green_matches = color.g == 0xffU;
	const auto blue_matches = color.b == 0x10U;

	EXPECT_TRUE(red_matches);
	EXPECT_TRUE(green_matches);
	EXPECT_TRUE(blue_matches);
}

/**
 * @test
 * @brief RGB値の等価比較確認。
 * @details 同じchannel値だけを等価とし、1 channelでも異なる値は非等価になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetColorTest, ComparesRgbValues)
{
	const auto base = MakeRgb(0x01U, 0x02U, 0x03U);
	const auto same = MakeRgb(0x01U, 0x02U, 0x03U);
	const auto red_differs = MakeRgb(0x04U, 0x02U, 0x03U);
	const auto green_differs = MakeRgb(0x01U, 0x04U, 0x03U);
	const auto blue_differs = MakeRgb(0x01U, 0x02U, 0x04U);

	const auto same_matches = base == same;
	const auto red_difference_matches = base == red_differs;
	const auto green_difference_matches = base == green_differs;
	const auto blue_difference_matches = base == blue_differs;
	const auto red_difference_detected = base != red_differs;
	const auto green_difference_detected = base != green_differs;
	const auto blue_difference_detected = base != blue_differs;
	const auto same_not_detected_as_different = base != same;

	EXPECT_TRUE(same_matches);
	EXPECT_FALSE(red_difference_matches);
	EXPECT_FALSE(green_difference_matches);
	EXPECT_FALSE(blue_difference_matches);
	EXPECT_TRUE(red_difference_detected);
	EXPECT_TRUE(green_difference_detected);
	EXPECT_TRUE(blue_difference_detected);
	EXPECT_FALSE(same_not_detected_as_different);
}

/**
 * @test
 * @brief 黒と白のRGB hex解析確認。
 * @details 境界値であるblackとwhiteを入力し、各channelが0x00と0xffへ変換されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetColorTest, ParsesBlackAndWhite)
{
	ket::color::Rgb black;
	ket::color::Rgb white;

	const auto black_parsed = ket::color::TryParse("#000000", 7U, black);
	const auto white_parsed = ket::color::TryParse("#ffffff", 7U, white);
	const auto black_matches = black == MakeRgb(0x00U, 0x00U, 0x00U);
	const auto white_matches = white == MakeRgb(0xffU, 0xffU, 0xffU);

	EXPECT_TRUE(black_parsed);
	EXPECT_TRUE(white_parsed);
	EXPECT_TRUE(black_matches);
	EXPECT_TRUE(white_matches);
}

/**
 * @test
 * @brief 大文字小文字混在hexの解析確認。
 * @details lower-case、upper-case、mixed-caseのhex digitを入力し、同じRGB値へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetColorTest, ParsesLowerUpperAndMixedCaseHex)
{
	ket::color::Rgb lower;
	ket::color::Rgb upper;
	ket::color::Rgb mixed;
	const auto expected = MakeRgb(0x0aU, 0xffU, 0x10U);

	const auto lower_parsed = ket::color::TryParse("#0aff10", 7U, lower);
	const auto upper_parsed = ket::color::TryParse("#0AFF10", 7U, upper);
	const auto mixed_parsed = ket::color::TryParse("#0aFf10", 7U, mixed);
	const auto lower_matches = lower == expected;
	const auto upper_matches = upper == expected;
	const auto mixed_matches = mixed == expected;

	EXPECT_TRUE(lower_parsed);
	EXPECT_TRUE(upper_parsed);
	EXPECT_TRUE(mixed_parsed);
	EXPECT_TRUE(lower_matches);
	EXPECT_TRUE(upper_matches);
	EXPECT_TRUE(mixed_matches);
}

/**
 * @test
 * @brief 先頭hash有無の解析確認。
 * @details 同じ6桁hexを先頭hashありとなしで入力し、同じRGB値へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetColorTest, ParsesWithAndWithoutLeadingHash)
{
	ket::color::Rgb with_hash;
	ket::color::Rgb without_hash;
	const auto expected = MakeRgb(0x12U, 0x34U, 0x56U);

	const auto with_hash_parsed = ket::color::TryParse("#123456", 7U, with_hash);
	const auto without_hash_parsed = ket::color::TryParse("123456", 6U, without_hash);
	const auto with_hash_matches = with_hash == expected;
	const auto without_hash_matches = without_hash == expected;

	EXPECT_TRUE(with_hash_parsed);
	EXPECT_TRUE(without_hash_parsed);
	EXPECT_TRUE(with_hash_matches);
	EXPECT_TRUE(without_hash_matches);
}

/**
 * @test
 * @brief std::string入力の解析確認。
 * @details std::string overloadを使い、Doxygen例と同じ入力がRGB値へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetColorTest, ParsesStringObjectInput)
{
	const std::string text = "#0aFF10";
	ket::color::Rgb color;
	const auto expected = MakeRgb(0x0aU, 0xffU, 0x10U);

	const auto parsed = ket::color::TryParse(text, color);
	const auto matches = color == expected;

	EXPECT_TRUE(parsed);
	EXPECT_TRUE(matches);
}

/**
 * @test
 * @brief 不正長RGB hexの拒否確認。
 * @details 空文字、短い入力、3桁短縮形、hashなし7桁、alpha付き8桁を入力し、falseを返すことを確認。
 * @pre C++17以降。
 * @post 失敗時の出力引数と外部状態の変更なし。
 */
TEST(KetColorTest, RejectsInvalidLengths)
{
	const auto sentinel = MakeRgb(0x12U, 0x34U, 0x56U);
	ket::color::Rgb empty = sentinel;
	ket::color::Rgb short_hash = sentinel;
	ket::color::Rgb shorthand = sentinel;
	ket::color::Rgb seven_without_hash = sentinel;
	ket::color::Rgb alpha = sentinel;

	const auto empty_parsed = ket::color::TryParse("", 0U, empty);
	const auto short_hash_parsed = ket::color::TryParse("#12345", 6U, short_hash);
	const auto shorthand_parsed = ket::color::TryParse("#abc", 4U, shorthand);
	const auto seven_without_hash_parsed = ket::color::TryParse("1234567", 7U, seven_without_hash);
	const auto alpha_parsed = ket::color::TryParse("#11223344", 9U, alpha);
	const auto empty_unchanged = empty == sentinel;
	const auto short_hash_unchanged = short_hash == sentinel;
	const auto shorthand_unchanged = shorthand == sentinel;
	const auto seven_without_hash_unchanged = seven_without_hash == sentinel;
	const auto alpha_unchanged = alpha == sentinel;

	EXPECT_FALSE(empty_parsed);
	EXPECT_FALSE(short_hash_parsed);
	EXPECT_FALSE(shorthand_parsed);
	EXPECT_FALSE(seven_without_hash_parsed);
	EXPECT_FALSE(alpha_parsed);
	EXPECT_TRUE(empty_unchanged);
	EXPECT_TRUE(short_hash_unchanged);
	EXPECT_TRUE(shorthand_unchanged);
	EXPECT_TRUE(seven_without_hash_unchanged);
	EXPECT_TRUE(alpha_unchanged);
}

/**
 * @test
 * @brief null pointer入力の拒否確認。
 * @details pointer overloadにnull pointerを渡し、falseを返して出力を変更しないことを確認。
 * @pre C++17以降。
 * @post 失敗時の出力引数と外部状態の変更なし。
 */
TEST(KetColorTest, RejectsNullInputPointer)
{
	const auto sentinel = MakeRgb(0x12U, 0x34U, 0x56U);
	ket::color::Rgb color = sentinel;

	const auto parsed = ket::color::TryParse(nullptr, 7U, color);
	const auto unchanged = color == sentinel;

	EXPECT_FALSE(parsed);
	EXPECT_TRUE(unchanged);
}

/**
 * @test
 * @brief 不正hex文字の拒否確認。
 * @details hex digitではない文字、空白、余分なhashを含む6桁入力を渡し、falseを返すことを確認。
 * @pre C++17以降。
 * @post 失敗時の出力引数と外部状態の変更なし。
 */
TEST(KetColorTest, RejectsInvalidHexCharacters)
{
	const auto sentinel = MakeRgb(0x12U, 0x34U, 0x56U);
	ket::color::Rgb alphabet = sentinel;
	ket::color::Rgb symbol = sentinel;
	ket::color::Rgb space = sentinel;
	ket::color::Rgb extra_hash = sentinel;
	ket::color::Rgb last_digit = sentinel;
	ket::color::Rgb leading_minus = sentinel;
	ket::color::Rgb leading_plus = sentinel;
	ket::color::Rgb hash_plus = sentinel;
	ket::color::Rgb high_bit = sentinel;
	std::string high_bit_text = "#12345";
	high_bit_text.push_back(static_cast<char>(0xffU));

	const auto alphabet_parsed = ket::color::TryParse("#12gg56", 7U, alphabet);
	const auto symbol_parsed = ket::color::TryParse("#12-456", 7U, symbol);
	const auto space_parsed = ket::color::TryParse("#12 456", 7U, space);
	const auto extra_hash_parsed = ket::color::TryParse("#12#456", 7U, extra_hash);
	const auto last_digit_parsed = ket::color::TryParse("#12345z", 7U, last_digit);
	const auto leading_minus_parsed = ket::color::TryParse("-12345", 6U, leading_minus);
	const auto leading_plus_parsed = ket::color::TryParse("+12345", 6U, leading_plus);
	const auto hash_plus_parsed = ket::color::TryParse("#+12345", 7U, hash_plus);
	const auto high_bit_parsed = ket::color::TryParse(high_bit_text, high_bit);
	const auto alphabet_unchanged = alphabet == sentinel;
	const auto symbol_unchanged = symbol == sentinel;
	const auto space_unchanged = space == sentinel;
	const auto extra_hash_unchanged = extra_hash == sentinel;
	const auto last_digit_unchanged = last_digit == sentinel;
	const auto leading_minus_unchanged = leading_minus == sentinel;
	const auto leading_plus_unchanged = leading_plus == sentinel;
	const auto hash_plus_unchanged = hash_plus == sentinel;
	const auto high_bit_unchanged = high_bit == sentinel;

	EXPECT_FALSE(alphabet_parsed);
	EXPECT_FALSE(symbol_parsed);
	EXPECT_FALSE(space_parsed);
	EXPECT_FALSE(extra_hash_parsed);
	EXPECT_FALSE(last_digit_parsed);
	EXPECT_FALSE(leading_minus_parsed);
	EXPECT_FALSE(leading_plus_parsed);
	EXPECT_FALSE(hash_plus_parsed);
	EXPECT_FALSE(high_bit_parsed);
	EXPECT_TRUE(alphabet_unchanged);
	EXPECT_TRUE(symbol_unchanged);
	EXPECT_TRUE(space_unchanged);
	EXPECT_TRUE(extra_hash_unchanged);
	EXPECT_TRUE(last_digit_unchanged);
	EXPECT_TRUE(leading_minus_unchanged);
	EXPECT_TRUE(leading_plus_unchanged);
	EXPECT_TRUE(hash_plus_unchanged);
	EXPECT_TRUE(high_bit_unchanged);
}

/**
 * @test
 * @brief RGB値のlower-case hex文字列化確認。
 * @details 代表値とwhiteを入力し、既定でhash付きlower-case 6桁hexへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetColorTest, FormatsWithHashByDefault)
{
	const auto sample = MakeRgb(0x0aU, 0xffU, 0x10U);
	const auto white = MakeRgb(0xffU, 0xffU, 0xffU);

	const auto sample_text = ket::color::Format(sample);
	const auto white_text = ket::color::Format(white);
	const auto black_text = ket::color::Format(ket::color::Rgb());

	EXPECT_EQ(sample_text, std::string("#0aff10"));
	EXPECT_EQ(white_text, std::string("#ffffff"));
	EXPECT_EQ(black_text, std::string("#000000"));
}

/**
 * @test
 * @brief RGB値のhashなしhex文字列化確認。
 * @details FormatOptions::with_hashをfalseにし、先頭hashなしのlower-case
 * 6桁hexへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetColorTest, FormatsWithoutHashWhenRequested)
{
	const auto black = MakeRgb(0x00U, 0x00U, 0x00U);
	const auto sample = MakeRgb(0x0aU, 0xffU, 0x10U);
	const ket::color::FormatOptions options(false);

	const auto black_text = ket::color::Format(black, options);
	const auto sample_text = ket::color::Format(sample, options);

	EXPECT_EQ(black_text, std::string("000000"));
	EXPECT_EQ(sample_text, std::string("0aff10"));
}
