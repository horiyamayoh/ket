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
 * U+0800、U+D7FF、U+E000、U+10000、U+10FFFFを境界値として固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, AcceptsUtf8BoundarySequences)
{
	const auto text = Bytes({
		0xE0U,
		0xA0U,
		0x80U,
		0xEDU,
		0x9FU,
		0xBFU,
		0xEEU,
		0x80U,
		0x80U,
		0xF0U,
		0x90U,
		0x80U,
		0x80U,
		0xF4U,
		0x8FU,
		0xBFU,
		0xBFU,
	});

	const auto validation = ket::utf8::Validate(text);
	const auto count = ket::utf8::CountCodePoints(text);

	EXPECT_TRUE(validation.valid);
	EXPECT_EQ(validation.error_offset, 0U);
	EXPECT_EQ(count, std::optional<std::size_t>(5U));
}

/**
 * @test
 * @brief overlong UTF-8 sequenceの拒否確認。
 * @details 2 byteと3 byteと4 byteのoverlong表現を入力し、invalidとして最初の不正byte
 * offsetを返すことを確認。 CountCodePointsは不正UTF-8をstd::nulloptに変換。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsOverlongSequences)
{
	const auto two_byte = Bytes({0xC0U, 0x80U});
	const auto three_byte = Bytes({0xE0U, 0x80U, 0x80U});
	const auto four_byte = Bytes({0xF0U, 0x80U, 0x80U, 0x80U});

	const auto two_byte_validation = ket::utf8::Validate(two_byte);
	const auto three_byte_validation = ket::utf8::Validate(three_byte);
	const auto four_byte_validation = ket::utf8::Validate(four_byte);
	const auto count = ket::utf8::CountCodePoints(two_byte);
	const auto count_has_value = count.has_value();

	EXPECT_FALSE(two_byte_validation.valid);
	EXPECT_EQ(two_byte_validation.error_offset, 0U);
	EXPECT_FALSE(three_byte_validation.valid);
	EXPECT_EQ(three_byte_validation.error_offset, 1U);
	EXPECT_FALSE(four_byte_validation.valid);
	EXPECT_EQ(four_byte_validation.error_offset, 1U);
	EXPECT_FALSE(count_has_value);
}

/**
 * @test
 * @brief truncated UTF-8 sequenceの拒否確認。
 * @details 2/3/4 byte
 * sequenceの途中で入力が終わるbyte列を渡し、sequence先頭offsetを返すことを確認。
 * 途中まで妥当なprefixがある場合も、不完全なsequenceの先頭を失敗位置として固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsTruncatedSequences)
{
	const auto truncated_two_byte = Bytes({0x41U, 0xC2U});
	const auto truncated_three_byte = Bytes({0xE2U, 0x82U});
	const auto truncated_four_byte = Bytes({0xF0U, 0x9FU, 0x98U});

	const auto two_byte_validation = ket::utf8::Validate(truncated_two_byte);
	const auto three_byte_validation = ket::utf8::Validate(truncated_three_byte);
	const auto four_byte_validation = ket::utf8::Validate(truncated_four_byte);

	EXPECT_FALSE(two_byte_validation.valid);
	EXPECT_EQ(two_byte_validation.error_offset, 1U);
	EXPECT_FALSE(three_byte_validation.valid);
	EXPECT_EQ(three_byte_validation.error_offset, 0U);
	EXPECT_FALSE(four_byte_validation.valid);
	EXPECT_EQ(four_byte_validation.error_offset, 0U);
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

	const auto high_validation = ket::utf8::Validate(high_surrogate);
	const auto low_validation = ket::utf8::Validate(low_surrogate);

	EXPECT_FALSE(high_validation.valid);
	EXPECT_EQ(high_validation.error_offset, 1U);
	EXPECT_FALSE(low_validation.valid);
	EXPECT_EQ(low_validation.error_offset, 1U);
}

/**
 * @test
 * @brief continuation byte不正の拒否確認。
 * @details 単独continuation byte、second byte不正、third byte不正、fourth
 * byte不正を入力し、最初の不正位置を返すことを確認。 byte列先頭のASCII
 * prefixは失敗offsetに影響しない。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUtf8Test, RejectsBadContinuationBytes)
{
	const auto lone_continuation = Bytes({0x80U});
	const auto bad_second = Bytes({0x41U, 0xE2U, 0x28U, 0xA1U});
	const auto bad_third = Bytes({0xE2U, 0x82U, 0x28U});
	const auto bad_fourth = Bytes({0xF0U, 0x9FU, 0x98U, 0x28U});

	const auto lone_validation = ket::utf8::Validate(lone_continuation);
	const auto second_validation = ket::utf8::Validate(bad_second);
	const auto third_validation = ket::utf8::Validate(bad_third);
	const auto fourth_validation = ket::utf8::Validate(bad_fourth);

	EXPECT_FALSE(lone_validation.valid);
	EXPECT_EQ(lone_validation.error_offset, 0U);
	EXPECT_FALSE(second_validation.valid);
	EXPECT_EQ(second_validation.error_offset, 2U);
	EXPECT_FALSE(third_validation.valid);
	EXPECT_EQ(third_validation.error_offset, 2U);
	EXPECT_FALSE(fourth_validation.valid);
	EXPECT_EQ(fourth_validation.error_offset, 3U);
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

	const auto above_max_validation = ket::utf8::Validate(above_max);
	const auto five_byte_validation = ket::utf8::Validate(five_byte_lead);

	EXPECT_FALSE(above_max_validation.valid);
	EXPECT_EQ(above_max_validation.error_offset, 1U);
	EXPECT_FALSE(five_byte_validation.valid);
	EXPECT_EQ(five_byte_validation.error_offset, 0U);
}
