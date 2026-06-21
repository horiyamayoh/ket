#include "ket_io_stream.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <ostream> // IWYU pragma: keep
#include <sstream>
#include <streambuf> // IWYU pragma: keep
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

	class ShortWriteBuffer : public std::streambuf
	{
	  public:
		explicit ShortWriteBuffer(std::streamsize write_limit) : write_limit_(write_limit) {}

		[[nodiscard]] const std::string& Written() const noexcept
		{
			return written_;
		}

	  protected:
		std::streamsize xsputn(const char* data, std::streamsize size) override
		{
			const auto accepted = size < write_limit_ ? size : write_limit_;
			written_.append(data, static_cast<std::size_t>(accepted));
			return accepted;
		}

	  private:
		std::streamsize write_limit_;
		std::string written_;
	};

	static_assert(!std::is_copy_constructible_v<ket::io_stream::FormatStateSaver>,
				  "FormatStateSaver copy construction is disabled");
	static_assert(!std::is_copy_assignable_v<ket::io_stream::FormatStateSaver>,
				  "FormatStateSaver copy assignment is disabled");
	static_assert(!std::is_move_constructible_v<ket::io_stream::FormatStateSaver>,
				  "FormatStateSaver move construction is disabled");
	static_assert(!std::is_move_assignable_v<ket::io_stream::FormatStateSaver>,
				  "FormatStateSaver move assignment is disabled");

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
 * @details 要求サイズより短い入力を渡し、読み切り失敗としてfalseを返し、読めたbyteだけ
 * bufferへ格納されることを確認。
 * @pre C++17以降。
 * @post streamは標準istreamのshort read後の状態へ進み、出力bufferの先頭だけが更新される。
 */
TEST(KetIoStreamTest, RejectsShortRead)
{
	std::istringstream stream("ab");
	auto output = std::array<std::uint8_t, 4>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}};
	const auto expected = std::array<std::uint8_t, 4>{
		{static_cast<std::uint8_t>('a'), static_cast<std::uint8_t>('b'), 0xAAU, 0xAAU}};

	const auto result = ket::io_stream::TryReadExactly(stream, output.data(), output.size());
	const auto reached_eof = stream.eof();

	EXPECT_FALSE(result);
	EXPECT_TRUE(reached_eof);
	EXPECT_EQ(output, expected);
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
	std::istringstream stream("ab");

	const auto result = ket::io_stream::TryReadExactly(stream, nullptr, 0U);
	const auto next = stream.peek();
	const auto stream_failed = stream.fail();

	EXPECT_TRUE(result);
	EXPECT_EQ(next, static_cast<int>('a'));
	EXPECT_FALSE(stream_failed);
}

/**
 * @test
 * @brief null bufferへの非0バイト読み取り失敗確認。
 * @details nullptrと非0サイズを渡し、streamへアクセスせずfalseを返すことを確認。
 * @pre C++17以降。
 * @post stream状態と読み取り位置は変更なし。
 */
TEST(KetIoStreamTest, RejectsNullReadBufferWithoutTouchingStream)
{
	std::istringstream stream("ab");

	const auto result = ket::io_stream::TryReadExactly(stream, nullptr, 1U);
	const auto next = stream.peek();
	const auto stream_failed = stream.fail();

	EXPECT_FALSE(result);
	EXPECT_EQ(next, static_cast<int>('a'));
	EXPECT_FALSE(stream_failed);
}

/**
 * @test
 * @brief 空入力からの非0バイト読み取り失敗確認。
 * @details 空streamから1バイト読み取りを試み、falseを返して出力bufferを変更しないことを確認。
 * @pre C++17以降。
 * @post streamはEOF状態へ進み、出力bufferは入力時の値を保持。
 */
TEST(KetIoStreamTest, RejectsReadFromEmptyInputForNonzeroSize)
{
	std::istringstream stream("");
	auto output = std::array<std::uint8_t, 1>{{0xAAU}};

	const auto result = ket::io_stream::TryReadExactly(stream, output.data(), output.size());
	const auto reached_eof = stream.eof();

	EXPECT_FALSE(result);
	EXPECT_TRUE(reached_eof);
	EXPECT_EQ(output[0], 0xAAU);
}

/**
 * @test
 * @brief bad streamからの読み取り失敗確認。
 * @details
 * badbit設定済みstreamから1バイト読み取りを試み、falseを返して出力bufferを変更しないことを確認。
 * @pre C++17以降。
 * @post streamはbad状態を保持し、出力bufferは入力時の値を保持。
 */
TEST(KetIoStreamTest, RejectsReadFromBadStream)
{
	std::istringstream stream("ab");
	stream.setstate(std::ios::badbit);
	auto output = std::array<std::uint8_t, 1>{{0xAAU}};

	const auto result = ket::io_stream::TryReadExactly(stream, output.data(), output.size());
	const auto stream_is_bad = stream.bad();

	EXPECT_FALSE(result);
	EXPECT_TRUE(stream_is_bad);
	EXPECT_EQ(output[0], 0xAAU);
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
	stream << "prefix";

	const auto result = ket::io_stream::TryWriteAll(stream, nullptr, 0U);
	const auto output = stream.str();

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string("prefix"));
}

/**
 * @test
 * @brief null bufferからの非0バイト書き込み失敗確認。
 * @details nullptrと非0サイズを渡し、streamへアクセスせずfalseを返すことを確認。
 * @pre C++17以降。
 * @post 出力streamと外部状態の変更なし。
 */
TEST(KetIoStreamTest, RejectsNullWriteBufferWithoutTouchingStream)
{
	std::ostringstream stream;

	const auto result = ket::io_stream::TryWriteAll(stream, nullptr, 1U);
	const auto output = stream.str();
	const auto stream_failed = stream.fail();

	EXPECT_FALSE(result);
	EXPECT_EQ(output, std::string());
	EXPECT_FALSE(stream_failed);
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
 * @brief short writeの失敗確認。
 * @details 書き込み先bufferが要求サイズより少ないbyte数だけ受理し、falseを返すことを確認。
 * @pre C++17以降。
 * @post streamは失敗状態となり、書き込めたbyteだけがbufferに残る。
 */
TEST(KetIoStreamTest, RejectsShortWrite)
{
	const auto input = std::array<std::uint8_t, 4>{{static_cast<std::uint8_t>('a'),
													static_cast<std::uint8_t>('b'),
													static_cast<std::uint8_t>('c'),
													static_cast<std::uint8_t>('d')}};
	ShortWriteBuffer buffer(2);
	std::ostream stream(&buffer);

	const auto result = ket::io_stream::TryWriteAll(stream, input.data(), input.size());
	const auto stream_failed = stream.fail();
	const auto written = buffer.Written();

	EXPECT_FALSE(result);
	EXPECT_TRUE(stream_failed);
	EXPECT_EQ(written, std::string("ab"));
}

/**
 * @test
 * @brief 読み取り例外の伝播確認。
 * @details failbit例外mask設定済みstreamでshort
 * readを起こし、istream例外が呼び出し側へ伝播することを確認。
 * @pre C++17以降。
 * @post streamは標準istreamの例外発生後の状態を保持。
 */
TEST(KetIoStreamTest, PropagatesReadExceptions)
{
	std::istringstream stream("ab");
	stream.exceptions(std::ios::failbit);
	auto output = std::array<std::uint8_t, 4>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}};

	const auto read_operation = [&stream, &output]()
	{
		static_cast<void>(ket::io_stream::TryReadExactly(stream, output.data(), output.size()));
	};

	EXPECT_THROW(read_operation(), std::ios_base::failure);
}

/**
 * @test
 * @brief 書き込み例外の伝播確認。
 * @details
 * badbit例外mask設定済みstreamで書き込み失敗を起こし、ostream例外が呼び出し側へ伝播することを確認。
 * @pre C++17以降。
 * @post streamは標準ostreamの例外発生後の状態を保持。
 */
TEST(KetIoStreamTest, PropagatesWriteExceptions)
{
	const auto input = std::array<std::uint8_t, 4>{{static_cast<std::uint8_t>('a'),
													static_cast<std::uint8_t>('b'),
													static_cast<std::uint8_t>('c'),
													static_cast<std::uint8_t>('d')}};
	ShortWriteBuffer buffer(2);
	std::ostream stream(&buffer);
	stream.exceptions(std::ios::badbit);

	const auto write_operation = [&stream, &input]()
	{
		static_cast<void>(ket::io_stream::TryWriteAll(stream, input.data(), input.size()));
	};

	EXPECT_THROW(write_operation(), std::ios_base::failure);
}

/**
 * @test
 * @brief stream書式状態の復元確認。
 * @details
 * flags、precision、fillを変更したscopeを抜け、FormatStateSaver構築前の値へ戻ることを確認。
 * @pre C++17以降。
 * @post streamのflags、precision、fillはFormatStateSaver構築前の値。
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
		const ket::io_stream::FormatStateSaver saver(stream);
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
 * @brief FormatStateSaverの保存対象外状態確認。
 * @details widthとerror stateをscope内で変更し、FormatStateSaverが復元しないことを確認。
 * @pre C++17以降。
 * @post streamのwidthとerror stateはscope内で設定した値を保持。
 */
TEST(KetIoStreamTest, DoesNotRestoreExcludedStreamState)
{
	std::ostringstream stream;
	stream.width(3);
	const auto initial_width = stream.width();

	{
		const ket::io_stream::FormatStateSaver saver(stream);
		stream.width(9);
		stream.setstate(std::ios::eofbit);
		static_cast<void>(saver);
	}

	const auto current_width = stream.width();
	const auto reached_eof = stream.eof();

	EXPECT_EQ(initial_width, 3);
	EXPECT_EQ(current_width, 9);
	EXPECT_TRUE(reached_eof);
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

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string("  value"));
}

/**
 * @test
 * @brief form feedとvertical tabを含む行末ASCII whitespace除去確認。
 * @details 行末にspace、tab、CR、form feed、vertical tabを置き、全て除去されることを確認。
 * @pre C++17以降。
 * @post outはASCII trim後の行に更新。
 */
TEST(KetIoStreamTest, TrimsAllTrailingAsciiWhitespaceBytes)
{
	std::istringstream stream("value \t\r\f\v\n");
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string("value"));
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

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, std::string());
}

/**
 * @test
 * @brief 改行なし最終行のEOF成功確認。
 * @details 改行なしでEOFへ到達する入力を渡し、読み取れた文字列を成功として扱うことを確認。
 * @pre C++17以降。
 * @post outはtrim後の最終行に更新。streamはEOF状態へ進む。
 */
TEST(KetIoStreamTest, ReadsFinalLineAtEofWithoutTrailingNewline)
{
	std::istringstream stream("value \t");
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);
	const auto reached_eof = stream.eof();

	EXPECT_TRUE(result);
	EXPECT_TRUE(reached_eof);
	EXPECT_EQ(output, std::string("value"));
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

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);

	EXPECT_FALSE(result);
	EXPECT_EQ(output, std::string("unchanged"));
}

/**
 * @test
 * @brief bad streamからの行読み失敗確認。
 * @details badbit設定済みstreamから行読みを試み、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post streamはbad状態を保持し、outは入力時の文字列を保持。
 */
TEST(KetIoStreamTest, RejectsLineReadFromBadStream)
{
	std::istringstream stream("value\n");
	stream.setstate(std::ios::badbit);
	std::string output = "unchanged";

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);
	const auto stream_is_bad = stream.bad();

	EXPECT_FALSE(result);
	EXPECT_TRUE(stream_is_bad);
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

	const auto result = ket::io_stream::TryReadLineTrimRightAscii(stream, output);

	EXPECT_TRUE(result);
	EXPECT_EQ(output, expected);
}

/**
 * @test
 * @brief 行読み例外の伝播確認。
 * @details
 * failbit例外mask設定済みstreamでEOF行読みを起こし、istream例外が呼び出し側へ伝播することを確認。
 * @pre C++17以降。
 * @post outは入力時の文字列を保持。
 */
TEST(KetIoStreamTest, PropagatesLineReadExceptions)
{
	std::istringstream stream("");
	stream.exceptions(std::ios::failbit);
	std::string output = "unchanged";

	const auto read_operation = [&stream, &output]()
	{
		static_cast<void>(ket::io_stream::TryReadLineTrimRightAscii(stream, output));
	};

	EXPECT_THROW(read_operation(), std::ios_base::failure);
	EXPECT_EQ(output, std::string("unchanged"));
}
