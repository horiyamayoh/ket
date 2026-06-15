#include "ket_color.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace
{
	ket::color::Rgb MakeRgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept
	{
		ket::color::Rgb color;
		color.r = r;
		color.g = g;
		color.b = b;
		return color;
	}

	bool RgbEquals(ket::color::Rgb lhs, ket::color::Rgb rhs) noexcept
	{
		return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
	}

} // namespace

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

	const auto black_parsed = ket::color::TryParse("#000000", black);
	const auto white_parsed = ket::color::TryParse("#ffffff", white);
	const auto black_matches = RgbEquals(black, MakeRgb(0x00U, 0x00U, 0x00U));
	const auto white_matches = RgbEquals(white, MakeRgb(0xffU, 0xffU, 0xffU));

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

	const auto lower_parsed = ket::color::TryParse("#0aff10", lower);
	const auto upper_parsed = ket::color::TryParse("#0AFF10", upper);
	const auto mixed_parsed = ket::color::TryParse("#0aFf10", mixed);
	const auto lower_matches = RgbEquals(lower, expected);
	const auto upper_matches = RgbEquals(upper, expected);
	const auto mixed_matches = RgbEquals(mixed, expected);

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

	const auto with_hash_parsed = ket::color::TryParse("#123456", with_hash);
	const auto without_hash_parsed = ket::color::TryParse("123456", without_hash);
	const auto with_hash_matches = RgbEquals(with_hash, expected);
	const auto without_hash_matches = RgbEquals(without_hash, expected);

	EXPECT_TRUE(with_hash_parsed);
	EXPECT_TRUE(without_hash_parsed);
	EXPECT_TRUE(with_hash_matches);
	EXPECT_TRUE(without_hash_matches);
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

	const auto empty_parsed = ket::color::TryParse("", empty);
	const auto short_hash_parsed = ket::color::TryParse("#12345", short_hash);
	const auto shorthand_parsed = ket::color::TryParse("#abc", shorthand);
	const auto seven_without_hash_parsed = ket::color::TryParse("1234567", seven_without_hash);
	const auto alpha_parsed = ket::color::TryParse("#11223344", alpha);
	const auto empty_unchanged = RgbEquals(empty, sentinel);
	const auto short_hash_unchanged = RgbEquals(short_hash, sentinel);
	const auto shorthand_unchanged = RgbEquals(shorthand, sentinel);
	const auto seven_without_hash_unchanged = RgbEquals(seven_without_hash, sentinel);
	const auto alpha_unchanged = RgbEquals(alpha, sentinel);

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

	const auto alphabet_parsed = ket::color::TryParse("#12gg56", alphabet);
	const auto symbol_parsed = ket::color::TryParse("#12-456", symbol);
	const auto space_parsed = ket::color::TryParse("#12 456", space);
	const auto extra_hash_parsed = ket::color::TryParse("##1234", extra_hash);
	const auto alphabet_unchanged = RgbEquals(alphabet, sentinel);
	const auto symbol_unchanged = RgbEquals(symbol, sentinel);
	const auto space_unchanged = RgbEquals(space, sentinel);
	const auto extra_hash_unchanged = RgbEquals(extra_hash, sentinel);

	EXPECT_FALSE(alphabet_parsed);
	EXPECT_FALSE(symbol_parsed);
	EXPECT_FALSE(space_parsed);
	EXPECT_FALSE(extra_hash_parsed);
	EXPECT_TRUE(alphabet_unchanged);
	EXPECT_TRUE(symbol_unchanged);
	EXPECT_TRUE(space_unchanged);
	EXPECT_TRUE(extra_hash_unchanged);
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

	EXPECT_EQ(sample_text, std::string("#0aff10"));
	EXPECT_EQ(white_text, std::string("#ffffff"));
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
	ket::color::FormatOptions options;
	options.with_hash = false;

	const auto text = ket::color::Format(black, options);

	EXPECT_EQ(text, std::string("000000"));
}
