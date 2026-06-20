#include "ket_byte_reader.h"

#include <array>
#include <cstdint>
#include <string> // NOLINT(misc-include-cleaner): IWYU expects gtest string support.

#include <gtest/gtest.h>

/**
 * @test
 * @brief 空buffer readerの境界確認。
 * @details nullptrかつsize
 * 0のreaderを有効な空readerとして扱い、0byte読み取りだけ成功することを確認。
 * @pre C++17以降。
 * @post readerは空状態を保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, HandlesEmptyReader)
{
	ket::byte_reader::Reader reader(nullptr, 0U);

	const auto size = reader.Size();
	const auto offset = reader.Offset();
	const auto remaining = reader.Remaining();
	const auto is_empty = reader.Empty();

	EXPECT_EQ(size, 0U);
	EXPECT_EQ(offset, 0U);
	EXPECT_EQ(remaining, 0U);
	EXPECT_TRUE(is_empty);

	const auto sentinel = std::array<std::uint8_t, 1>{{0xFFU}};
	const std::uint8_t* bytes = sentinel.data();
	const auto zero_read = reader.ReadBytes(0U, bytes);
	const auto zero_offset = reader.Offset();

	EXPECT_TRUE(zero_read);
	EXPECT_EQ(bytes, nullptr);
	EXPECT_EQ(zero_offset, 0U);

	std::uint8_t value = 0xAAU;
	const auto one_read = reader.ReadU8(value);
	const auto failed_offset = reader.Offset();

	EXPECT_FALSE(one_read);
	EXPECT_EQ(value, 0xAAU);
	EXPECT_EQ(failed_offset, 0U);
}

/**
 * @test
 * @brief ぴったり読み切りのoffset更新確認。
 * @details 2byte bufferを1byteずつ読み、成功時だけoffsetとremainingが進むことを確認。
 * @pre C++17以降。
 * @post readerは末尾offsetを保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, ReadsExactSizeBuffer)
{
	const auto data = std::array<std::uint8_t, 2>{{0x12U, 0x34U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	std::uint8_t first = 0U;
	const auto first_read = reader.ReadU8(first);
	const auto first_offset = reader.Offset();
	const auto first_remaining = reader.Remaining();

	std::uint8_t second = 0U;
	const auto second_read = reader.ReadU8(second);
	const auto second_offset = reader.Offset();
	const auto second_remaining = reader.Remaining();
	const auto is_empty = reader.Empty();

	EXPECT_TRUE(first_read);
	EXPECT_EQ(first, 0x12U);
	EXPECT_EQ(first_offset, 1U);
	EXPECT_EQ(first_remaining, 1U);
	EXPECT_TRUE(second_read);
	EXPECT_EQ(second, 0x34U);
	EXPECT_EQ(second_offset, 2U);
	EXPECT_EQ(second_remaining, 0U);
	EXPECT_TRUE(is_empty);
}

/**
 * @test
 * @brief 1byte不足時の失敗確認。
 * @details 3byte bufferから4byte値を読み、失敗時にoffsetと出力が変化しないことを確認。
 * @pre C++17以降。
 * @post reader offsetと出力変数は入力時の値を保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, RejectsReadWhenOneByteShort)
{
	const auto data = std::array<std::uint8_t, 3>{{0x12U, 0x34U, 0x56U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	std::uint32_t value = 0xAABBCCDDU;
	const auto read_succeeded = reader.ReadBe32(value);
	const auto offset = reader.Offset();
	const auto remaining = reader.Remaining();

	EXPECT_FALSE(read_succeeded);
	EXPECT_EQ(value, 0xAABBCCDDU);
	EXPECT_EQ(offset, 0U);
	EXPECT_EQ(remaining, 3U);
}

/**
 * @test
 * @brief size不足時のSkip失敗確認。
 * @details 残りbyte数を超えるSkipを行い、失敗時にoffsetとremainingが変化しないことを確認。
 * @pre C++17以降。
 * @post reader offsetとremainingは入力時の値を保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, RejectsOversizedSkipWithoutAdvancing)
{
	const auto data = std::array<std::uint8_t, 2>{{0x12U, 0x34U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	const auto skipped = reader.Skip(3U);
	const auto offset = reader.Offset();
	const auto remaining = reader.Remaining();

	EXPECT_FALSE(skipped);
	EXPECT_EQ(offset, 0U);
	EXPECT_EQ(remaining, 2U);
}

/**
 * @test
 * @brief 失敗時offset保持確認。
 * @details
 * 1byte読んだ後に残りbyte数を超えるReadBytesを行い、失敗時にoffsetと出力pointerが変化しないことを確認。
 * @pre C++17以降。
 * @post reader offsetと出力pointerは失敗前の値を保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, KeepsOffsetAndOutputOnFailure)
{
	const auto data = std::array<std::uint8_t, 2>{{0x12U, 0x34U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	std::uint8_t value = 0U;
	const auto first_read = reader.ReadU8(value);
	const auto offset_before_failure = reader.Offset();

	const std::uint8_t* bytes = data.data();
	const auto bytes_read = reader.ReadBytes(2U, bytes);
	const auto offset_after_failure = reader.Offset();
	const auto remaining_after_failure = reader.Remaining();

	EXPECT_TRUE(first_read);
	EXPECT_EQ(value, 0x12U);
	EXPECT_EQ(offset_before_failure, 1U);
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(bytes, data.data());
	EXPECT_EQ(offset_after_failure, 1U);
	EXPECT_EQ(remaining_after_failure, 1U);
}

/**
 * @test
 * @brief BE/LE整数読み取り確認。
 * @details
 * 同じbyte列からbig-endianとlittle-endianの16bit/32bit値を読み、期待する整数値になることを確認。
 * @pre C++17以降。
 * @post 各readerは読み取った幅だけoffsetを進める。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, ReadsBigAndLittleEndianValues)
{
	const auto data = std::array<std::uint8_t, 4>{{0x12U, 0x34U, 0x56U, 0x78U}};

	ket::byte_reader::Reader be16_reader(data.data(), data.size());
	std::uint16_t be16 = 0U;
	const auto be16_read = be16_reader.ReadBe16(be16);
	const auto be16_offset = be16_reader.Offset();

	ket::byte_reader::Reader le16_reader(data.data(), data.size());
	std::uint16_t le16 = 0U;
	const auto le16_read = le16_reader.ReadLe16(le16);
	const auto le16_offset = le16_reader.Offset();

	ket::byte_reader::Reader be32_reader(data.data(), data.size());
	std::uint32_t be32 = 0U;
	const auto be32_read = be32_reader.ReadBe32(be32);
	const auto be32_offset = be32_reader.Offset();

	ket::byte_reader::Reader le32_reader(data.data(), data.size());
	std::uint32_t le32 = 0U;
	const auto le32_read = le32_reader.ReadLe32(le32);
	const auto le32_offset = le32_reader.Offset();

	EXPECT_TRUE(be16_read);
	EXPECT_EQ(be16, 0x1234U);
	EXPECT_EQ(be16_offset, 2U);
	EXPECT_TRUE(le16_read);
	EXPECT_EQ(le16, 0x3412U);
	EXPECT_EQ(le16_offset, 2U);
	EXPECT_TRUE(be32_read);
	EXPECT_EQ(be32, 0x12345678U);
	EXPECT_EQ(be32_offset, 4U);
	EXPECT_TRUE(le32_read);
	EXPECT_EQ(le32, 0x78563412U);
	EXPECT_EQ(le32_offset, 4U);
}

/**
 * @test
 * @brief ReadBytesのnon-owning範囲確認。
 * @details Skip後にReadBytesを行い、元buffer内の現在offset位置を指すpointerを返すことを確認。
 * @pre C++17以降。
 * @post reader offsetはskipとreadの合計byte数まで進む。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, ReturnsNonOwningBytesAtCurrentOffset)
{
	const auto data = std::array<std::uint8_t, 4>{{0x10U, 0x20U, 0x30U, 0x40U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	const auto skipped = reader.Skip(1U);
	const std::uint8_t* bytes = nullptr;
	const auto bytes_read = reader.ReadBytes(2U, bytes);
	const auto offset = reader.Offset();
	const auto remaining = reader.Remaining();

	EXPECT_TRUE(skipped);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(bytes, data.data() + 1U);
	EXPECT_EQ(bytes[0], 0x20U);
	EXPECT_EQ(bytes[1], 0x30U);
	EXPECT_EQ(offset, 3U);
	EXPECT_EQ(remaining, 1U);
}

/**
 * @test
 * @brief 非空readerの0byte範囲読み取り確認。
 * @details Skip後に0byteのReadBytesを行い、現在offsetのpointerを返し、offsetを進めないことを確認。
 * @pre C++17以降。
 * @post reader offsetは0byte読み取り前の値を保持。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, ReadsZeroBytesAtCurrentOffsetWithoutAdvancing)
{
	const auto data = std::array<std::uint8_t, 3>{{0x10U, 0x20U, 0x30U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	const auto skipped = reader.Skip(1U);
	const auto offset_before_read = reader.Offset();

	const std::uint8_t* bytes = nullptr;
	const auto bytes_read = reader.ReadBytes(0U, bytes);
	const auto offset_after_read = reader.Offset();
	const auto remaining_after_read = reader.Remaining();

	EXPECT_TRUE(skipped);
	EXPECT_EQ(offset_before_read, 1U);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(bytes, data.data() + 1U);
	EXPECT_EQ(offset_after_read, 1U);
	EXPECT_EQ(remaining_after_read, 2U);
}

/**
 * @test
 * @brief invalid readerの失敗確認。
 * @details nullptrかつ非0 sizeのreaderをinvalid
 * readerとして扱い、読み取りとskipが失敗することを確認。
 * @pre C++17以降。
 * @post reader offsetと出力値は入力時の値を保持。
 */
TEST(KetByteReaderTest, RejectsInvalidReader)
{
	ket::byte_reader::Reader reader(nullptr, 1U);

	const auto size = reader.Size();
	const auto remaining = reader.Remaining();
	const auto is_empty = reader.Empty();

	std::uint8_t value = 0xAAU;
	const auto value_read = reader.ReadU8(value);

	const auto sentinel = std::array<std::uint8_t, 1>{{0xFFU}};
	const std::uint8_t* bytes = sentinel.data();
	const auto bytes_read = reader.ReadBytes(0U, bytes);

	const auto skipped = reader.Skip(0U);
	const auto offset = reader.Offset();

	EXPECT_EQ(size, 1U);
	EXPECT_EQ(remaining, 0U);
	EXPECT_FALSE(is_empty);
	EXPECT_FALSE(value_read);
	EXPECT_EQ(value, 0xAAU);
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(bytes, sentinel.data());
	EXPECT_FALSE(skipped);
	EXPECT_EQ(offset, 0U);
}

/**
 * @test
 * @brief copy/move時のreader状態複製確認。
 * @details offsetを進めたreaderをcopy/moveし、non-owning
 * pointer、size、offsetに基づく後続読み取りが一致することを確認。
 * @pre C++17以降。
 * @post copy/move後のreaderは同じ次byteを読み取る。外部bufferの変更なし。
 */
TEST(KetByteReaderTest, CopiesAndMovesReaderState)
{
	const auto data = std::array<std::uint8_t, 3>{{0x10U, 0x20U, 0x30U}};
	ket::byte_reader::Reader reader(data.data(), data.size());

	const auto skipped = reader.Skip(1U);
	ket::byte_reader::Reader copied(reader);
	ket::byte_reader::Reader moved(static_cast<ket::byte_reader::Reader&&>(reader));

	std::uint8_t copied_value = 0U;
	const auto copied_read = copied.ReadU8(copied_value);
	const auto copied_offset = copied.Offset();

	std::uint8_t moved_value = 0U;
	const auto moved_read = moved.ReadU8(moved_value);
	const auto moved_offset = moved.Offset();

	EXPECT_TRUE(skipped);
	EXPECT_TRUE(copied_read);
	EXPECT_EQ(copied_value, 0x20U);
	EXPECT_EQ(copied_offset, 2U);
	EXPECT_TRUE(moved_read);
	EXPECT_EQ(moved_value, 0x20U);
	EXPECT_EQ(moved_offset, 2U);
}
