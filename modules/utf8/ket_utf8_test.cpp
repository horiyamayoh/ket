#include "ket_utf8.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
	std::string Bytes(std::initializer_list<unsigned char> values)
	{
		std::string result(values.size(), '\0');
		std::transform(values.begin(),
					   values.end(),
					   result.begin(),
					   [](unsigned char value)
					   {
						   return static_cast<char>(value);
					   });

		return result;
	}

	void ExpectInvalidAt(const std::string& text, std::size_t expected_error_offset)
	{
		const auto validation = ket::utf8::Validate(text);
		const auto is_valid = ket::utf8::IsValid(text);
		const auto count = ket::utf8::CountCodePoints(text);
		const auto count_has_value = count.has_value();

		EXPECT_FALSE(validation.valid);
		EXPECT_EQ(validation.error_offset, expected_error_offset);
		EXPECT_FALSE(is_valid);
		EXPECT_FALSE(count_has_value);
	}

} // namespace

/**
 * @test
 * @brief ASCII byte列の検査確認。
 * @details printable ASCIIとNULを含むbyte列を入力し、ASCIIかつ妥当なUTF-8として扱うことを確認。
 * code point数はbyte数と一致することも確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, AcceptsAscii)
{
	const auto text = std::string("A\0z", 3U);

	const auto is_ascii = ket::utf8::IsAscii(text);
	const auto validation = ket::utf8::Validate(text);
	const auto is_valid = ket::utf8::IsValid(text);
	const auto count = ket::utf8::CountCodePoints(text);

	EXPECT_TRUE(is_ascii);
	EXPECT_TRUE(validation.valid);
	EXPECT_EQ(validation.error_offset, 0U);
	EXPECT_TRUE(is_valid);
	EXPECT_EQ(count, std::optional<std::size_t>(3U));
}

/**
 * @test
 * @brief ASCII判定のbyte境界確認。
 * @details `0x7F`まではASCIIとして扱い、`0x80`以上のbyteを含む入力はUTF-8妥当性と独立して
 * ASCIIではないことを確認。`0xFF`のような不正UTF-8 byteも高位byteとして拒否。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, ChecksAsciiByteBoundary)
{
	const auto ascii_boundary = Bytes({0x00U, 0x7FU});
	const auto first_high_byte = Bytes({0x80U});
	const auto max_byte = Bytes({0xFFU});
	const auto mixed = Bytes({0x41U, 0x80U});

	const auto ascii_boundary_is_ascii = ket::utf8::IsAscii(ascii_boundary);
	const auto first_high_byte_is_ascii = ket::utf8::IsAscii(first_high_byte);
	const auto max_byte_is_ascii = ket::utf8::IsAscii(max_byte);
	const auto mixed_is_ascii = ket::utf8::IsAscii(mixed);

	EXPECT_TRUE(ascii_boundary_is_ascii);
	EXPECT_FALSE(first_high_byte_is_ascii);
	EXPECT_FALSE(max_byte_is_ascii);
	EXPECT_FALSE(mixed_is_ascii);
}

/**
 * @test
 * @brief 空文字列の境界値確認。
 * @details 空文字列を入力し、ASCIIかつ妥当なUTF-8として扱い、code point数0を返すことを確認。
 * 空入力は失敗値ではなく空の妥当なbyte列として固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, AcceptsEmptyInput)
{
	const auto text = std::string();

	const auto is_ascii = ket::utf8::IsAscii(text);
	const auto validation = ket::utf8::Validate(text);
	const auto is_valid = ket::utf8::IsValid(text);
	const auto count = ket::utf8::CountCodePoints(text);

	EXPECT_TRUE(is_ascii);
	EXPECT_TRUE(validation.valid);
	EXPECT_EQ(validation.error_offset, 0U);
	EXPECT_TRUE(is_valid);
	EXPECT_EQ(count, std::optional<std::size_t>(0U));
}

/**
 * @test
 * @brief 2/3/4 byte UTF-8 sequenceの正常系確認。
 * @details U+00A2、U+20AC、U+1F600を含むbyte列を入力し、妥当なUTF-8として扱うことを確認。
 * ASCII判定はfalseとなり、code point数はsequence数として数えられる。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, AcceptsMultiByteSequences)
{
	const auto text = Bytes({
		0x41U,
		0xC2U,
		0xA2U,
		0xE2U,
		0x82U,
		0xACU,
		0xF0U,
		0x9FU,
		0x98U,
		0x80U,
	});

	const auto is_ascii = ket::utf8::IsAscii(text);
	const auto validation = ket::utf8::Validate(text);
	const auto is_valid = ket::utf8::IsValid(text);
	const auto count = ket::utf8::CountCodePoints(text);

	EXPECT_FALSE(is_ascii);
	EXPECT_TRUE(validation.valid);
	EXPECT_EQ(validation.error_offset, 0U);
	EXPECT_TRUE(is_valid);
	EXPECT_EQ(count, std::optional<std::size_t>(4U));
}

/**
 * @test
 * @brief UTF-8 sequence境界値の正常系確認。
 * @details
 * overlong、surrogate、範囲外にならない最小・最大境界のbyte列を入力し、妥当なUTF-8として扱うことを確認。
 * U+007F、U+0080、U+07FF、U+0800、U+D7FF、U+E000、U+10000、U+10FFFFを境界値として固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, AcceptsUtf8BoundarySequences)
{
	const auto text = Bytes({
		0x7FU, 0xC2U, 0x80U, 0xDFU, 0xBFU, 0xE0U, 0xA0U, 0x80U, 0xEDU, 0x9FU, 0xBFU,
		0xEEU, 0x80U, 0x80U, 0xF0U, 0x90U, 0x80U, 0x80U, 0xF4U, 0x8FU, 0xBFU, 0xBFU,
	});

	const auto validation = ket::utf8::Validate(text);
	const auto count = ket::utf8::CountCodePoints(text);

	EXPECT_TRUE(validation.valid);
	EXPECT_EQ(validation.error_offset, 0U);
	EXPECT_EQ(count, std::optional<std::size_t>(8U));
}

/**
 * @test
 * @brief overlong UTF-8 sequenceの拒否確認。
 * @details 2 byteと3 byteと4 byteのoverlong表現を入力し、invalidとして最初の不正byte
 * offsetを返すことを確認。`IsValid`はfalse、`CountCodePoints`はstd::nullopt。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsOverlongSequences)
{
	const auto two_byte = Bytes({0xC0U, 0x80U});
	const auto three_byte = Bytes({0xE0U, 0x80U, 0x80U});
	const auto four_byte = Bytes({0xF0U, 0x80U, 0x80U, 0x80U});

	ExpectInvalidAt(two_byte, 0U);
	ExpectInvalidAt(three_byte, 1U);
	ExpectInvalidAt(four_byte, 1U);
}

/**
 * @test
 * @brief truncated UTF-8 sequenceの拒否確認。
 * @details 2/3/4 byte
 * sequenceの途中で入力が終わるbyte列を渡し、sequence先頭offsetを返すことを確認。
 * 途中まで妥当なprefixがある場合だけ、不完全なsequenceの先頭を失敗位置として固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsTruncatedSequences)
{
	const auto truncated_two_byte = Bytes({0x41U, 0xC2U});
	const auto truncated_three_byte = Bytes({0xE2U, 0x82U});
	const auto truncated_four_byte = Bytes({0xF0U, 0x9FU, 0x98U});

	ExpectInvalidAt(truncated_two_byte, 1U);
	ExpectInvalidAt(truncated_three_byte, 0U);
	ExpectInvalidAt(truncated_four_byte, 0U);
}

/**
 * @test
 * @brief 短い malformed sequenceの失敗位置確認。
 * @details 必要byte数に満たない入力でも、存在するbyteがUTF-8範囲制約またはcontinuation
 * 制約に反していれば、そのbyte位置を失敗位置として返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsShortMalformedSequencesAtPresentOffendingByte)
{
	const auto three_byte_overlong_prefix = Bytes({0xE0U, 0x80U});
	const auto three_byte_bad_second = Bytes({0xE2U, 0x28U});
	const auto four_byte_overlong_prefix = Bytes({0xF0U, 0x80U});
	const auto four_byte_bad_third = Bytes({0xF0U, 0x90U, 0x28U});
	const auto four_byte_above_max_prefix = Bytes({0xF4U, 0x90U});

	ExpectInvalidAt(three_byte_overlong_prefix, 1U);
	ExpectInvalidAt(three_byte_bad_second, 1U);
	ExpectInvalidAt(four_byte_overlong_prefix, 1U);
	ExpectInvalidAt(four_byte_bad_third, 2U);
	ExpectInvalidAt(four_byte_above_max_prefix, 1U);
}

/**
 * @test
 * @brief surrogate UTF-8 sequenceの拒否確認。
 * @details U+D800とU+DFFFに相当するsurrogate範囲のbyte列を渡し、invalidとして拒否することを確認。
 * 先頭byteは3 byte sequenceの形を持つため、範囲違反のsecond byteを失敗位置として返す。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsSurrogateSequences)
{
	const auto high_surrogate = Bytes({0xEDU, 0xA0U, 0x80U});
	const auto low_surrogate = Bytes({0xEDU, 0xBFU, 0xBFU});

	ExpectInvalidAt(high_surrogate, 1U);
	ExpectInvalidAt(low_surrogate, 1U);
}

/**
 * @test
 * @brief continuation byte不正の拒否確認。
 * @details 単独continuation byte、2 byte sequenceのsecond byte不正、3/4 byte
 * sequenceのsecond/third/fourth byte不正を入力し、最初の不正位置を返すことを確認。
 * byte列先頭のASCII prefixは失敗offsetに影響しない。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsBadContinuationBytes)
{
	const auto lone_continuation = Bytes({0x80U});
	const auto bad_two_byte_second = Bytes({0xC2U, 0x20U});
	const auto bad_second = Bytes({0x41U, 0xE2U, 0x28U, 0xA1U});
	const auto bad_third = Bytes({0xE2U, 0x82U, 0x28U});
	const auto bad_fourth = Bytes({0xF0U, 0x9FU, 0x98U, 0x28U});

	ExpectInvalidAt(lone_continuation, 0U);
	ExpectInvalidAt(bad_two_byte_second, 1U);
	ExpectInvalidAt(bad_second, 2U);
	ExpectInvalidAt(bad_third, 2U);
	ExpectInvalidAt(bad_fourth, 3U);
}

/**
 * @test
 * @brief UTF-8範囲外code pointの拒否確認。
 * @details U+10FFFF超過相当と5 byte lead byteを含むbyte列を渡し、invalidとして拒否することを確認。
 * F4境界ではsecond byteを、F5以降ではlead byteを失敗位置として返す。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsOutOfRangeCodePoints)
{
	const auto above_max = Bytes({0xF4U, 0x90U, 0x80U, 0x80U});
	const auto five_byte_lead = Bytes({0xF5U, 0x80U, 0x80U, 0x80U});

	ExpectInvalidAt(above_max, 1U);
	ExpectInvalidAt(five_byte_lead, 0U);
}
