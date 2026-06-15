#include "ket_io_stream.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	std::string BytesToString(const std::array<std::uint8_t, 4>& bytes)
	{
		std::string result(bytes.size(), '\0');

		std::transform(bytes.begin(),
					   bytes.end(),
					   result.begin(),
					   [](std::uint8_t byte)
					   {
						   return static_cast<char>(byte);
					   });

		return result;
	}

	static_assert(!std::is_copy_constructible_v<ket::io_stream::StateSaver>,
				  "StateSaver copy construction is disabled");
	static_assert(!std::is_copy_assignable_v<ket::io_stream::StateSaver>,
				  "StateSaver copy assignment is disabled");
	static_assert(!std::is_move_constructible_v<ket::io_stream::StateSaver>,
				  "StateSaver move construction is disabled");
	static_assert(!std::is_move_assignable_v<ket::io_stream::StateSaver>,
				  "StateSaver move assignment is disabled");

} // namespace

/**
 * @test
 * @brief 指定バイト数の読み切り確認。
 * @details NULと上位bitを含む4バイト入力を渡し、要求した全バイトが出力へ格納されることを確認。
 * @pre C++17以降。
 * @post 出力bufferは期待する4バイト列へ変化。streamは読み取り後の状態へ進む。
 */
TEST(KetIoStreamTest, ReadsExactlyRequestedBytes)
{
	const auto expected = std::array<std::uint8_t, 4>{{0x00U, 0x01U, 0xFEU, 0xFFU}};
	const auto input = BytesToString(expected);
	std::istringstream stream(input);
	auto output = std::array<std::uint8_t, 4>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}};

	const auto result = ket::io_stream::TryReadExactly(stream, output.data(), output.size());

	EXPECT_TRUE(result);
	EXPECT_EQ(output, expected);
}

/**
 * @test
 * @brief short readの失敗確認。
 * @details 要求サイズより短い入力を渡し、読み切り失敗としてfalseを返すことを確認。
 * @pre C++17以降。
 * @post streamは標準istreamのshort read後の状態へ進む。
 */
TEST(KetIoStreamTest, RejectsShortRead)
{
	std::istringstream stream("ab");
	auto output = std::array<std::uint8_t, 4>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}};

	const auto result = ket::io_stream::TryReadExactly(stream, output.data(), output.size());
	const auto reached_eof = stream.eof();

	EXPECT_FALSE(result);
	EXPECT_TRUE(reached_eof);
}

/**
 * @test
 * @brief 0バイト読み取りの境界確認。
 * @details nullptrと0サイズを渡し、stream状態を進めずに成功することを確認。
 * @pre C++17以降。
 * @post streamと外部状態の変更なし。
 */
TEST(KetIoStreamTest, ReadsZeroBytesAsSuccess)
{
	std::istringstream stream("");

	const auto result = ket::io_stream::TryReadExactly(stream, nullptr, 0U);
	const auto stream_failed = stream.fail();

	EXPECT_TRUE(result);
	EXPECT_FALSE(stream_failed);
}

/**
 * @test
 * @brief 指定バイト数の書き切り確認。
 * @details NULと上位bitを含む4バイト入力を渡し、streamへ同じ順序で書き込まれることを確認。
 * @pre C++17以降。
 * @post 出力streamは期待する4バイト列を保持。
 */
TEST(KetIoStreamTest, WritesAllRequestedBytes)
{
	const auto input = std::array<std::uint8_t, 4>{{0x00U, 0x01U, 0xFEU, 0xFFU}};
	const auto expected = BytesToString(input);
	std::ostringstream stream;

	const auto result = ket::io_stream::TryWriteAll(stream, input.data(), input.size());
	const auto output = stream.str();

	EXPECT_TRUE(result);
	EXPECT_EQ(output, expected);
}

/**
 * @test
 * @brief 0バイト書き込みの境界確認。
 * @details nullptrと0サイズを渡し、streamへ何も書き込まずに成功することを確認。
 * @pre C++17以降。
 * @post 出力streamと外部状態の変更なし。
 */
TEST(KetIoStreamTest, WritesZeroBytesAsSuccess)
{
	std::ostringstream stream;

	const auto result = ket::io_stream::TryWriteAll(stream, nullptr, 0U);
	const auto output = stream.str();

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string());
}

/**
 * @test
 * @brief stream error時の書き込み失敗確認。
 * @details badbit設定済みstreamへ1バイトを書き込み、falseを返すことを確認。
 * @pre C++17以降。
 * @post streamは失敗状態を保持。
 */
TEST(KetIoStreamTest, RejectsWriteToBadStream)
{
	const auto input = std::array<std::uint8_t, 1>{{0x42U}};
	std::ostringstream stream;
	stream.setstate(std::ios::badbit);

	const auto result = ket::io_stream::TryWriteAll(stream, input.data(), input.size());
	const auto stream_is_bad = stream.bad();

	EXPECT_FALSE(result);
	EXPECT_TRUE(stream_is_bad);
}

/**
 * @test
 * @brief stream書式状態の復元確認。
 * @details flags、precision、fillを変更したscopeを抜け、StateSaver構築前の値へ戻ることを確認。
 * @pre C++17以降。
 * @post streamのflags、precision、fillはStateSaver構築前の値。
 */
TEST(KetIoStreamTest, RestoresStreamState)
{
	std::ostringstream stream;
	stream.setf(std::ios::hex, std::ios::basefield);
	stream.precision(3);
	stream.fill('*');
	const auto initial_flags = stream.flags();
	const auto initial_precision = stream.precision();
	const auto initial_fill = stream.fill();

	{
		const auto saver = ket::io_stream::StateSaver(stream);
		stream.setf(std::ios::dec, std::ios::basefield);
		stream.precision(9);
		stream.fill('#');
		static_cast<void>(saver);
	}

	const auto restored_flags = stream.flags();
	const auto restored_precision = stream.precision();
	const auto restored_fill = stream.fill();

	EXPECT_EQ(restored_flags, initial_flags);
	EXPECT_EQ(restored_precision, initial_precision);
	EXPECT_EQ(restored_fill, initial_fill);
}

/**
 * @test
 * @brief 行末ASCII whitespace除去付き行読み確認。
 * @details
 * 先頭spaceと行末space、tab、CRを含む1行を読み、先頭spaceを保持して行末だけ除去することを確認。
 * @pre C++17以降。
 * @post outはtrim後の行に更新。streamは次行の前へ進む。
 */
TEST(KetIoStreamTest, ReadsLineAndTrimsTrailingAsciiWhitespace)
{
	std::istringstream stream("  value \t\r\nnext");
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimmedAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string("  value"));
}

/**
 * @test
 * @brief 空行読み取りの成功確認。
 * @details 改行だけの入力を渡し、空文字列として成功することを確認。
 * @pre C++17以降。
 * @post outは空文字列に更新。
 */
TEST(KetIoStreamTest, ReadsEmptyLineAsSuccess)
{
	std::istringstream stream("\n");
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimmedAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string());
}

/**
 * @test
 * @brief EOF行読み失敗時のout不変確認。
 * @details 空の入力streamから行読みを試み、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時の文字列を保持。
 */
TEST(KetIoStreamTest, LeavesOutputUnchangedWhenLineReadHitsEof)
{
	std::istringstream stream("");
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimmedAscii(stream, output);

	EXPECT_FALSE(result);
	EXPECT_EQ(output, std::string("unchanged"));
}

/**
 * @test
 * @brief ASCII以外の末尾byte保持確認。
 * @details UTF-8のnon-breaking space後にASCII spaceを置き、ASCII spaceだけを除去することを確認。
 * @pre C++17以降。
 * @post outはASCII trim後のbyte列に更新。
 */
TEST(KetIoStreamTest, KeepsNonAsciiTrailingBytesDuringLineTrim)
{
	std::string input;
	input.push_back(static_cast<char>(0xC2));
	input.push_back(static_cast<char>(0xA0));
	input.push_back(' ');
	input.push_back('\n');
	std::istringstream stream(input);
	std::string output = "unchanged";
	std::string expected;
	expected.push_back(static_cast<char>(0xC2));
	expected.push_back(static_cast<char>(0xA0));

	const auto result = ket::io_stream::TryReadLineTrimmedAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, expected);
}
