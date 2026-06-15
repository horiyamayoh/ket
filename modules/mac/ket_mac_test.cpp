#include "ket_mac.h"

#include <cstddef>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
	constexpr std::size_t kAddressByteCount = 6U;

	bool AddressEquals(ket::mac::Address value, ket::mac::Address expected) noexcept
	{
		for (std::size_t index = 0; index < kAddressByteCount; ++index)
		{
			const auto byte_matches = value.bytes[index] == expected.bytes[index];
			if (!byte_matches)
			{
				return false;
			}
		}

		return true;
	}

	bool OptionalAddressEquals(std::optional<ket::mac::Address> value,
							   ket::mac::Address expected) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return AddressEquals(*value, expected);
	}

} // namespace

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
	const auto parsed_matches = OptionalAddressEquals(parsed, expected);

	EXPECT_TRUE(parsed_matches);
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
	const auto parsed_matches = OptionalAddressEquals(parsed, expected);

	EXPECT_TRUE(parsed_matches);
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
	const auto parsed_matches = OptionalAddressEquals(parsed, expected);

	EXPECT_TRUE(parsed_matches);
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
