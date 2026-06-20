#include "ket_byte_writer.h"

#include <array>
#include <cstdint>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

/**
 * @test
 * @brief 空buffer writerの境界確認。
 * @details nullptrとsize
 * 0でwriterを生成し、空のWriteBytesは成功し、非空WriteU8は失敗することを確認。
 * @pre C++17以降。
 * @post writerは空状態を保持。外部bufferの変更なし。
 */
TEST(KetByteWriterTest, HandlesEmptyBuffer)
{
	ket::byte_writer::Writer writer(nullptr, 0U);

	const auto empty_write = writer.WriteBytes(nullptr, 0U);
	const auto byte_write = writer.WriteU8(0x12U);
	const auto writer_is_full = writer.Full();

	EXPECT_TRUE(empty_write);
	EXPECT_FALSE(byte_write);
	EXPECT_EQ(writer.Size(), 0U);
	EXPECT_EQ(writer.Offset(), 0U);
	EXPECT_EQ(writer.Remaining(), 0U);
	EXPECT_TRUE(writer_is_full);
}

/**
 * @test
 * @brief fixed bufferへのぴったり書き切り確認。
 * @details U8、big-endian 16bit、little-endian
 * 32bitを順に書き込み、offsetが末尾へ到達することを確認。
 * @pre C++17以降。
 * @post bufferは期待するbyte列に変化し、writerはfull状態。
 */
TEST(KetByteWriterTest, WritesExactlyToEnd)
{
	std::array<std::uint8_t, 7> buffer = {{0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto write_u8 = writer.WriteU8(0xAAU);
	const auto write_be16 = writer.WriteBe16(0x1234U);
	const auto write_le32 = writer.WriteLe32(0x89ABCDEFU);
	const auto writer_is_full = writer.Full();

	const auto expected =
		std::array<std::uint8_t, 7>{{0xAAU, 0x12U, 0x34U, 0xEFU, 0xCDU, 0xABU, 0x89U}};

	EXPECT_TRUE(write_u8);
	EXPECT_TRUE(write_be16);
	EXPECT_TRUE(write_le32);
	EXPECT_EQ(buffer, expected);
	EXPECT_EQ(writer.Offset(), buffer.size());
	EXPECT_EQ(writer.Remaining(), 0U);
	EXPECT_TRUE(writer_is_full);
}

/**
 * @test
 * @brief endian別整数値のbyte順確認。
 * @details BE16、BE32、LE16、LE32を順に書き込み、各値が指定endian順でbufferへ入ることを確認。
 * @pre C++17以降。
 * @post bufferは期待するbyte列に変化し、writer offsetは書き込みbyte数分進む。
 */
TEST(KetByteWriterTest, WritesBigEndianAndLittleEndianValues)
{
	std::array<std::uint8_t, 12> buffer = {{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto write_be16 = writer.WriteBe16(0x0102U);
	const auto write_be32 = writer.WriteBe32(0x03040506U);
	const auto write_le16 = writer.WriteLe16(0x0708U);
	const auto write_le32 = writer.WriteLe32(0x090A0B0CU);

	const auto expected = std::array<std::uint8_t, 12>{
		{0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x08U, 0x07U, 0x0CU, 0x0BU, 0x0AU, 0x09U}};

	EXPECT_TRUE(write_be16);
	EXPECT_TRUE(write_be32);
	EXPECT_TRUE(write_le16);
	EXPECT_TRUE(write_le32);
	EXPECT_EQ(buffer, expected);
	EXPECT_EQ(writer.Offset(), buffer.size());
}

/**
 * @test
 * @brief 1byte不足時の失敗確認。
 * @details 残り3byteの状態で4byte書き込みを試み、offsetとbufferが変更されないことを確認。
 * @pre C++17以降。
 * @post 失敗したwriteはoffsetとbuffer内容を保持。
 */
TEST(KetByteWriterTest, RejectsWriteWhenOneByteShort)
{
	std::array<std::uint8_t, 4> buffer = {{0x00U, 0x11U, 0x22U, 0x33U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto prefix_write = writer.WriteU8(0xAAU);
	const auto before_failed_write = buffer;
	const auto write_too_large = writer.WriteBe32(0x01020304U);

	EXPECT_TRUE(prefix_write);
	EXPECT_FALSE(write_too_large);
	EXPECT_EQ(buffer, before_failed_write);
	EXPECT_EQ(writer.Offset(), 1U);
	EXPECT_EQ(writer.Remaining(), 3U);
}

/**
 * @test
 * @brief byte列書き込み失敗時の状態保持確認。
 * @details 残りbyte数を超える入力とnull sourceを渡し、offsetとbufferが変更されないことを確認。
 * @pre C++17以降。
 * @post 失敗したwriteはoffsetとbuffer内容を保持。
 */
TEST(KetByteWriterTest, KeepsOffsetAndBufferWhenWriteBytesFails)
{
	std::array<std::uint8_t, 3> buffer = {{0xA0U, 0xA1U, 0xA2U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());
	const auto source = std::array<std::uint8_t, 4>{{0x10U, 0x11U, 0x12U, 0x13U}};

	const auto before_oversized_write = buffer;
	const auto oversized_write = writer.WriteBytes(source.data(), source.size());
	const auto before_null_write = buffer;
	const auto null_write = writer.WriteBytes(nullptr, 1U);

	EXPECT_FALSE(oversized_write);
	EXPECT_EQ(buffer, before_oversized_write);
	EXPECT_EQ(writer.Offset(), 0U);
	EXPECT_FALSE(null_write);
	EXPECT_EQ(buffer, before_null_write);
	EXPECT_EQ(writer.Offset(), 0U);
}

/**
 * @test
 * @brief skipのoffset更新と境界確認。
 * @details skip成功時はbufferを変更せずoffsetだけ進め、残りbyte数を超えるskipは失敗することを確認。
 * @pre C++17以降。
 * @post 成功したskipはoffsetだけを変更し、失敗したskipは状態を保持。
 */
TEST(KetByteWriterTest, SkipsWithoutMutatingBuffer)
{
	std::array<std::uint8_t, 3> buffer = {{0xF0U, 0xF1U, 0xF2U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto skip_two = writer.Skip(2U);
	const auto before_failed_skip = buffer;
	const auto skip_too_far = writer.Skip(2U);

	EXPECT_TRUE(skip_two);
	EXPECT_EQ(buffer, before_failed_skip);
	EXPECT_EQ(writer.Offset(), 2U);
	EXPECT_EQ(writer.Remaining(), 1U);
	EXPECT_FALSE(skip_too_far);
	EXPECT_EQ(buffer, before_failed_skip);
	EXPECT_EQ(writer.Offset(), 2U);
}

/**
 * @test
 * @brief invalid writerの失敗確認。
 * @details nullptrと非0
 * sizeで生成したwriterへ非空writeとskipを行い、すべて失敗してoffsetを保持し、fullではないことを確認。
 * @pre C++17以降。
 * @post invalid writerはconstructor直後のoffsetを保持。
 */
TEST(KetByteWriterTest, RejectsInvalidWriterOperations)
{
	const auto source = std::array<std::uint8_t, 1>{{0x33U}};
	ket::byte_writer::Writer writer(nullptr, 4U);

	const auto empty_write = writer.WriteBytes(nullptr, 0U);
	const auto write_u8 = writer.WriteU8(0x12U);
	const auto write_bytes = writer.WriteBytes(source.data(), source.size());
	const auto skip_zero = writer.Skip(0U);
	const auto skip = writer.Skip(1U);
	const auto writer_is_full = writer.Full();

	EXPECT_FALSE(empty_write);
	EXPECT_FALSE(write_u8);
	EXPECT_FALSE(write_bytes);
	EXPECT_FALSE(skip_zero);
	EXPECT_FALSE(skip);
	EXPECT_EQ(writer.Size(), 4U);
	EXPECT_EQ(writer.Offset(), 0U);
	EXPECT_EQ(writer.Remaining(), 0U);
	EXPECT_FALSE(writer_is_full);
}

/**
 * @test
 * @brief writer copyのnon-owning状態複製確認。
 * @details 書き込み済みwriterをcopyし、copy先が同じbufferとoffsetから書き込むことを確認。
 * @pre C++17以降。
 * @post bufferはcopy先writerの書き込み結果を反映。
 */
TEST(KetByteWriterTest, CopiesNonOwningState)
{
	std::array<std::uint8_t, 2> buffer = {{0U, 0U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto first_write = writer.WriteU8(0x11U);
	ket::byte_writer::Writer copy = writer;
	const auto copied_write = copy.WriteU8(0x22U);

	const auto expected = std::array<std::uint8_t, 2>{{0x11U, 0x22U}};

	EXPECT_TRUE(first_write);
	EXPECT_TRUE(copied_write);
	EXPECT_EQ(buffer, expected);
	EXPECT_EQ(writer.Offset(), 1U);
	EXPECT_EQ(copy.Offset(), 2U);
}

/**
 * @test
 * @brief writer moveの状態移動確認。
 * @details move構築したwriterが同じbufferとoffsetから書き込み、bufferに反映されることを確認。
 * @pre C++17以降。
 * @post bufferはmove先writerの書き込み結果を反映。
 */
TEST(KetByteWriterTest, MovesNonOwningState)
{
	std::array<std::uint8_t, 2> buffer = {{0U, 0U}};
	ket::byte_writer::Writer writer(buffer.data(), buffer.size());

	const auto first_write = writer.WriteU8(0x44U);
	ket::byte_writer::Writer moved(static_cast<ket::byte_writer::Writer&&>(writer));
	const auto moved_write = moved.WriteU8(0x55U);

	const auto expected = std::array<std::uint8_t, 2>{{0x44U, 0x55U}};

	EXPECT_TRUE(first_write);
	EXPECT_TRUE(moved_write);
	EXPECT_EQ(buffer, expected);
	EXPECT_EQ(moved.Offset(), 2U);
}
