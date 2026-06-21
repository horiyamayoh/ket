#include "ket_hex.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

/**
 * @test
 * @brief 整数hex文字列化の幅とcase確認。
 * @details 0、代表値、64bit最大値、指定幅と実桁の大小を入力し、最小幅zero
 * paddingと大文字小文字が固定されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, FormatsIntegerWithWidthAndCase)
{
	const auto zero = ket::hex::Format(0U);
	const auto padded_lower = ket::hex::Format(0x2aU, 4U, ket::hex::LetterCase::kLower);
	const auto default_upper = ket::hex::Format(0xdeadbeefU);
	const auto width_below_digits = ket::hex::Format(0x123U, 2U);
	const auto width_above_uint64_digits = ket::hex::Format(0x1U, 18U);
	const auto maximum = ket::hex::Format(UINT64_MAX, 16U, ket::hex::LetterCase::kLower);

	EXPECT_EQ(zero, std::string("0"));
	EXPECT_EQ(padded_lower, std::string("002a"));
	EXPECT_EQ(default_upper, std::string("DEADBEEF"));
	EXPECT_EQ(width_below_digits, std::string("123"));
	EXPECT_EQ(width_above_uint64_digits, std::string("000000000000000001"));
	EXPECT_EQ(maximum, std::string("ffffffffffffffff"));
}

/**
 * @test
 * @brief byte列hex文字列化の空入力とcase確認。
 * @details 空入力、lower、upperを入力し、空文字列または指定caseのhex文字列になることを確認。
 * @pre C++17以降。`nullptr` かつ非0 sizeはprecondition違反のため呼び出さない。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, EncodesBytesWithCase)
{
	const auto data = std::array<std::uint8_t, 4>{{0x00U, 0x0FU, 0xA5U, 0xFFU}};
	const auto lower_options = ket::hex::FormatOptions{ket::hex::LetterCase::kLower, '\0'};

	const auto empty = ket::hex::Encode(nullptr, 0U);
	const auto default_upper = ket::hex::Encode(data.data(), data.size());
	const auto lower = ket::hex::Encode(data.data(), data.size(), lower_options);

	EXPECT_EQ(empty, std::string());
	EXPECT_EQ(default_upper, std::string("000FA5FF"));
	EXPECT_EQ(lower, std::string("000fa5ff"));
}

/**
 * @test
 * @brief byte列hex文字列化のseparator確認。
 * @details separator付きoptionを入力し、byte間だけにseparatorが挿入されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, EncodesBytesWithSeparator)
{
	const auto data = std::array<std::uint8_t, 3>{{0xDEU, 0xADU, 0xBEU}};
	const auto dash_options = ket::hex::FormatOptions{ket::hex::LetterCase::kUpper, '-'};
	const auto colon_lower_options = ket::hex::FormatOptions{ket::hex::LetterCase::kLower, ':'};

	const auto dash = ket::hex::Encode(data.data(), data.size(), dash_options);
	const auto colon_lower = ket::hex::Encode(data.data(), data.size(), colon_lower_options);

	EXPECT_EQ(dash, std::string("DE-AD-BE"));
	EXPECT_EQ(colon_lower, std::string("de:ad:be"));
}

/**
 * @test
 * @brief hex文字列decodeの空入力とASCII whitespace確認。
 * @details 空入力、ASCII
 * whitespaceのみ、whitespace混在hexを入力し、偶数桁だけがbyte列へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DecodesHexWithAsciiWhitespace)
{
	const auto empty = ket::hex::Decode("");
	const auto whitespace_only = ket::hex::Decode(" \t\r\n\v\f");
	const auto mixed = ket::hex::Decode("de ad\nBE\tEF");

	const auto expected_empty =
		std::optional<std::vector<std::uint8_t>>(std::vector<std::uint8_t>{});
	const auto expected_mixed = std::optional<std::vector<std::uint8_t>>(
		std::vector<std::uint8_t>{0xDEU, 0xADU, 0xBEU, 0xEFU});

	EXPECT_EQ(empty, expected_empty);
	EXPECT_EQ(whitespace_only, expected_empty);
	EXPECT_EQ(mixed, expected_mixed);
}

/**
 * @test
 * @brief hex文字列decodeの不正入力拒否確認。
 * @details 奇数桁、ASCII
 * whitespaceを除いた奇数桁、不正文字、separator付きhexを入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, RejectsInvalidHexDuringDecode)
{
	const auto odd_digits = ket::hex::Decode("abc");
	const auto odd_digits_with_whitespace = ket::hex::Decode("a \n");
	const auto invalid_character = ket::hex::Decode("00gg");
	const auto separator = ket::hex::Decode("DE-AD");

	EXPECT_EQ(odd_digits, std::nullopt);
	EXPECT_EQ(odd_digits_with_whitespace, std::nullopt);
	EXPECT_EQ(invalid_character, std::nullopt);
	EXPECT_EQ(separator, std::nullopt);
}

/**
 * @test
 * @brief hexdumpの空入力確認。
 * @details byte列dumpと型なしmemory dumpへ空入力を渡し、空文字列を返すことを確認。
 * @pre C++17以降。`nullptr` かつ非0 sizeはprecondition違反のため呼び出さない。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DumpsEmptyInputAsEmptyString)
{
	const auto byte_dump = ket::hex::Dump(nullptr, 0U);
	const auto memory_dump = ket::hex::DumpMemory(nullptr, 0U);

	EXPECT_EQ(byte_dump, std::string());
	EXPECT_EQ(memory_dump, std::string());
}

/**
 * @test
 * @brief hexdumpの16 byte行golden確認。
 * @details 0x00から0x0fまでの16 byteを入力し、仕様カードのgolden outputと一致することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DumpsSixteenByteRow)
{
	const auto data = std::array<std::uint8_t, 16>{{0x00U,
													0x01U,
													0x02U,
													0x03U,
													0x04U,
													0x05U,
													0x06U,
													0x07U,
													0x08U,
													0x09U,
													0x0AU,
													0x0BU,
													0x0CU,
													0x0DU,
													0x0EU,
													0x0FU}};
	const auto expected = std::string("00000000  00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  "
									  "|................|");

	const auto dump = ket::hex::Dump(data.data(), data.size());

	EXPECT_EQ(dump, expected);
}

/**
 * @test
 * @brief hexdumpの短い最終行padding確認。
 * @details 8 byte splitより前で終わる2 byte入力をdumpし、ASCII preview columnとtyped memory
 * dumpのobject representation読み取りが固定されることを確認。
 * @pre C++17以降。`nullptr` かつ非0 sizeはprecondition違反のため呼び出さない。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DumpsMemoryRangeAndPadsShortRow)
{
	const auto data = std::array<unsigned char, 2>{{0x00U, 0x41U}};
	const auto expected =
		std::string("00000000  00 41") + std::string(45U, ' ') + std::string("|.A|");

	const auto dump = ket::hex::DumpMemory(data.data(), data.size());

	EXPECT_EQ(dump, expected);
}

/**
 * @test
 * @brief hexdumpの複数行golden確認。
 * @details 17 byte入力をdumpし、2行目offset、行間newline、最終行padding、末尾newlineなしを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DumpsMultipleRowsWithSecondOffset)
{
	const auto data = std::array<std::uint8_t, 17>{{0x00U,
													0x01U,
													0x02U,
													0x03U,
													0x04U,
													0x05U,
													0x06U,
													0x07U,
													0x08U,
													0x09U,
													0x0AU,
													0x0BU,
													0x0CU,
													0x0DU,
													0x0EU,
													0x0FU,
													0x10U}};
	const auto first_row = std::string(
		"00000000  00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  |................|");
	const auto second_row =
		std::string("00000010  10") + std::string(48U, ' ') + std::string("|.|");
	const auto expected = first_row + std::string("\n") + second_row;

	const auto dump = ket::hex::Dump(data.data(), data.size());
	const auto last_char = dump.back();

	EXPECT_EQ(dump, expected);
	EXPECT_NE(last_char, '\n');
}

/**
 * @test
 * @brief hexdumpのASCII previewと最終行padding確認。
 * @details printable/non-printableを混ぜた10 byteを入力し、ASCII
 * column位置と末尾newlineなしを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetHexTest, DumpsAsciiPreviewAndPadsFinalRow)
{
	const auto data = std::array<std::uint8_t, 10>{
		{0x20U, 0x21U, 0x7EU, 0x7FU, 0x1FU, 0x41U, 0x7AU, 0x00U, 0x41U, 0x42U}};
	const auto expected = std::string("00000000  20 21 7e 7f 1f 41 7a 00  41 42") +
		std::string(20U, ' ') + std::string("| !~..Az.AB|");

	const auto dump = ket::hex::Dump(data.data(), data.size());
	const auto last_char = dump.back();

	EXPECT_EQ(dump, expected);
	EXPECT_NE(last_char, '\n');
}
