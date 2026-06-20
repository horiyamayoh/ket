#include "ket_mac.h"

#include <optional>
#include <string>

#include <gtest/gtest.h>

/**
 * @test
 * @brief colon区切り大文字MAC addressのparse確認。
 * @details 大文字hex digitとcolon区切りの正準入力を渡し、6 byteのAddressへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ParsesUpperColonAddress)
{
	const auto parsed = ket::mac::Parse("AA:BB:CC:DD:EE:FF");
	const auto expected = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}};
	const auto parsed_has_value = parsed.has_value();
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::mac::Address{});
	EXPECT_EQ(parsed_value, expected);
}

/**
 * @test
 * @brief hyphen区切り小文字MAC addressのparse確認。
 * @details 小文字hex digitとhyphen区切りの入力を渡し、6 byteのAddressへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ParsesLowerHyphenAddress)
{
	const auto parsed = ket::mac::Parse("aa-bb-cc-dd-ee-ff");
	const auto expected = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}};
	const auto parsed_has_value = parsed.has_value();
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::mac::Address{});
	EXPECT_EQ(parsed_value, expected);
}

/**
 * @test
 * @brief 混在caseのMAC address parse確認。
 * @details 大文字と小文字が混在するhex digitを渡し、同じbyte値へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ParsesMixedHexCase)
{
	const auto parsed = ket::mac::Parse("aA:Bb:Cc:Dd:eE:Ff");
	const auto expected = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}};
	const auto parsed_has_value = parsed.has_value();
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::mac::Address{});
	EXPECT_EQ(parsed_value, expected);
}

/**
 * @test
 * @brief 数字hex digitと境界byteのparse確認。
 * @details 0から9のhex
 * digitと0x00/0x01/0x09/0x10/0xFFを含む入力を渡し、各byte値へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ParsesDecimalHexDigitsAndBoundaryBytes)
{
	const auto parsed = ket::mac::Parse("00:01:09:0a:10:ff");
	const auto expected = ket::mac::Address{{0x00U, 0x01U, 0x09U, 0x0AU, 0x10U, 0xFFU}};
	const auto parsed_has_value = parsed.has_value();
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::mac::Address{});
	EXPECT_EQ(parsed_value, expected);
}

/**
 * @test
 * @brief MAC address値の比較確認。
 * @details 同じ6 byte値と異なる6
 * byte値を比較し、等価/非等価演算子とoptional比較が同じ意味になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ComparesAddressValues)
{
	const auto lhs = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}};
	const auto same = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}};
	const auto different = ket::mac::Address{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0x00U}};
	const std::optional<ket::mac::Address> parsed = ket::mac::Parse("AA:BB:CC:DD:EE:FF");
	const std::optional<ket::mac::Address> expected = same;

	const auto values_match = lhs == same;
	const auto values_do_not_differ = lhs != same;
	const auto values_differ = lhs != different;
	const auto optional_matches = parsed == expected;

	EXPECT_TRUE(values_match);
	EXPECT_FALSE(values_do_not_differ);
	EXPECT_TRUE(values_differ);
	EXPECT_TRUE(optional_matches);
}

/**
 * @test
 * @brief MAC addressの小文字format確認。
 * @details 0、1桁値、最大byte値を含むAddressを渡し、既定で小文字colon区切りになることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, FormatsLowerColonAddressByDefault)
{
	const auto value = ket::mac::Address{{0x00U, 0x01U, 0x0AU, 0x10U, 0xABU, 0xFFU}};

	const auto formatted = ket::mac::Format(value);

	EXPECT_EQ(formatted, std::string("00:01:0a:10:ab:ff"));
}

/**
 * @test
 * @brief MAC addressの大文字format確認。
 * @details LetterCase::kUpperを指定し、hex digitが大文字のcolon区切りになることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, FormatsUpperColonAddress)
{
	const auto value = ket::mac::Address{{0x00U, 0x01U, 0x0AU, 0x10U, 0xABU, 0xFFU}};

	const auto formatted = ket::mac::Format(value, ket::mac::LetterCase::kUpper);

	EXPECT_EQ(formatted, std::string("00:01:0A:10:AB:FF"));
}

/**
 * @test
 * @brief MAC address parse後formatのroundtrip確認。
 * @details hyphen区切り入力をparseし、formatで小文字colon区切りの正準形へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, ParsesThenFormatsCanonicalLowerColonAddress)
{
	const auto parsed = ket::mac::Parse("AA-BB-CC-DD-EE-FF");
	const auto parsed_has_value = parsed.has_value();
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::mac::Address{});
	const auto formatted = ket::mac::Format(parsed_value);

	EXPECT_EQ(formatted, std::string("aa:bb:cc:dd:ee:ff"));
}

/**
 * @test
 * @brief 不正hex digitの拒否確認。
 * @details hex digit以外の文字を含む入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, RejectsInvalidHexDigit)
{
	const auto invalid_first = ket::mac::Parse("GA:BB:CC:DD:EE:FF");
	const auto invalid_second = ket::mac::Parse("AA:BG:CC:DD:EE:FF");
	const auto invalid_last = ket::mac::Parse("AA:BB:CC:DD:EE:FG");

	EXPECT_EQ(invalid_first, std::nullopt);
	EXPECT_EQ(invalid_second, std::nullopt);
	EXPECT_EQ(invalid_last, std::nullopt);
}

/**
 * @test
 * @brief 短いMAC address入力の拒否確認。
 * @details byte数不足、区切り不足、空文字列を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, RejectsShortInput)
{
	const auto five_bytes = ket::mac::Parse("AA:BB:CC:DD:EE");
	const auto missing_separator = ket::mac::Parse("AA:BB:CC:DD:EEFF");
	const auto empty = ket::mac::Parse("");

	EXPECT_EQ(five_bytes, std::nullopt);
	EXPECT_EQ(missing_separator, std::nullopt);
	EXPECT_EQ(empty, std::nullopt);
}

/**
 * @test
 * @brief 長いMAC address入力の拒否確認。
 * @details byte数過多、末尾文字追加、Cisco形式を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, RejectsLongInputAndCiscoFormat)
{
	const auto seven_bytes = ket::mac::Parse("AA:BB:CC:DD:EE:FF:00");
	const auto trailing_char = ket::mac::Parse("AA:BB:CC:DD:EE:FF0");
	const auto cisco = ket::mac::Parse("aabb.ccdd.eeff");

	EXPECT_EQ(seven_bytes, std::nullopt);
	EXPECT_EQ(trailing_char, std::nullopt);
	EXPECT_EQ(cisco, std::nullopt);
}

/**
 * @test
 * @brief 混在区切り文字の拒否確認。
 * @details colonとhyphenが混在する入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, RejectsMixedSeparators)
{
	const auto hyphen_after_colon = ket::mac::Parse("AA:BB-CC:DD:EE:FF");
	const auto colon_after_hyphen = ket::mac::Parse("AA-BB:CC-DD-EE-FF");

	EXPECT_EQ(hyphen_after_colon, std::nullopt);
	EXPECT_EQ(colon_after_hyphen, std::nullopt);
}

/**
 * @test
 * @brief 非対応区切り文字の拒否確認。
 * @details byte数と位置は正しいが区切り文字だけが異なる入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMacTest, RejectsUnsupportedSeparator)
{
	const auto dot_separator = ket::mac::Parse("AA.BB.CC.DD.EE.FF");
	const auto slash_separator = ket::mac::Parse("AA/BB/CC/DD/EE/FF");

	EXPECT_EQ(dot_separator, std::nullopt);
	EXPECT_EQ(slash_separator, std::nullopt);
}
