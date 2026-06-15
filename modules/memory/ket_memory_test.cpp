#include "ket_memory.h"

#include <array>
#include <cstdint>
#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

/**
 * @test
 * @brief pointer alignment判定の正常系と境界値確認。
 * @details aligned pointer、unaligned pointer、alignment 1、alignment 0、非power-of-two、
 * null pointerを入力し、仕様通りの真偽値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMemoryTest, ChecksPointerAlignment)
{
	alignas(16) unsigned char buffer[32] = {};
	const void* const aligned = static_cast<const void*>(buffer);
	const void* const unaligned = static_cast<const void*>(buffer + 1U);

	const auto aligned_to_16 = ket::memory::IsAligned(aligned, 16U);
	const auto unaligned_to_16 = ket::memory::IsAligned(unaligned, 16U);
	const auto aligned_to_1 = ket::memory::IsAligned(unaligned, 1U);
	const auto zero_alignment = ket::memory::IsAligned(aligned, 0U);
	const auto non_power_of_two_alignment = ket::memory::IsAligned(aligned, 3U);
	const auto null_pointer = ket::memory::IsAligned(nullptr, 1U);

	EXPECT_TRUE(aligned_to_16);
	EXPECT_FALSE(unaligned_to_16);
	EXPECT_TRUE(aligned_to_1);
	EXPECT_FALSE(zero_alignment);
	EXPECT_FALSE(non_power_of_two_alignment);
	EXPECT_FALSE(null_pointer);
}

/**
 * @test
 * @brief pointer alignment切り上げと切り下げの正常系確認。
 * @details 16 byte
 * alignment済みbufferの途中pointerを入力し、直前と直後のalignment境界へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力変数以外の外部状態の変更なし。
 */
TEST(KetMemoryTest, AlignsPointerUpAndDown)
{
	alignas(16) unsigned char buffer[64] = {};
	const void* const input = static_cast<const void*>(buffer + 1U);
	const void* const expected_up = static_cast<const void*>(buffer + 16U);
	const void* const expected_down = static_cast<const void*>(buffer);
	const void* aligned_up = nullptr;
	const void* aligned_down = nullptr;

	const auto align_up_succeeded = ket::memory::TryAlignUp(input, 16U, aligned_up);
	const auto align_down_succeeded = ket::memory::TryAlignDown(input, 16U, aligned_down);

	EXPECT_TRUE(align_up_succeeded);
	EXPECT_TRUE(align_down_succeeded);
	EXPECT_EQ(aligned_up, expected_up);
	EXPECT_EQ(aligned_down, expected_down);
}

/**
 * @test
 * @brief pointer alignment計算の失敗時出力保持確認。
 * @details alignment 0、非power-of-two、null
 * pointerを入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post 出力変数と外部状態の変更なし。
 */
TEST(KetMemoryTest, KeepsOutputUnchangedWhenAlignmentFails)
{
	alignas(8) unsigned char buffer[32] = {};
	const void* const input = static_cast<const void*>(buffer + 1U);
	const void* const sentinel = static_cast<const void*>(buffer + 3U);
	const void* out = sentinel;

	const auto zero_alignment = ket::memory::TryAlignUp(input, 0U, out);
	EXPECT_FALSE(zero_alignment);
	EXPECT_EQ(out, sentinel);

	out = sentinel;
	const auto non_power_of_two_alignment = ket::memory::TryAlignUp(input, 6U, out);
	EXPECT_FALSE(non_power_of_two_alignment);
	EXPECT_EQ(out, sentinel);

	out = sentinel;
	const auto null_pointer = ket::memory::TryAlignDown(nullptr, 8U, out);
	EXPECT_FALSE(null_pointer);
	EXPECT_EQ(out, sentinel);
}

/**
 * @test
 * @brief 通常zeroingのbyte変更とnull+0 no-op確認。
 * @details 非ゼロbyte列をZeroへ渡して全byteが0になり、nullptrとsize
 * 0の呼び出しがno-opで完了することを確認。
 * @pre C++17以降。
 * @post 入力bufferは全byteが0に変化。nullptr対象の外部状態変更なし。
 */
TEST(KetMemoryTest, ZeroesBytes)
{
	std::array<unsigned char, 4> bytes{{1U, 2U, 3U, 4U}};

	ket::memory::Zero(bytes.data(), bytes.size());
	ket::memory::Zero(nullptr, 0U);

	for (const auto byte : bytes)
	{
		EXPECT_EQ(byte, static_cast<unsigned char>(0U));
	}
}

/**
 * @test
 * @brief secure zeroingのbyte変更とnull+0 no-op確認。
 * @details 非ゼロbyte列をSecureZeroへ渡して全byteが0になり、nullptrとsize
 * 0の呼び出しがno-opで完了することを確認。
 * @pre C++17以降。
 * @post 入力bufferは全byteが0に変化。nullptr対象の外部状態変更なし。
 */
TEST(KetMemoryTest, SecureZeroesBytes)
{
	std::array<unsigned char, 5> bytes{{9U, 8U, 7U, 6U, 5U}};

	ket::memory::SecureZero(bytes.data(), bytes.size());
	ket::memory::SecureZero(nullptr, 0U);

	for (const auto byte : bytes)
	{
		EXPECT_EQ(byte, static_cast<unsigned char>(0U));
	}
}

/**
 * @test
 * @brief object representation pointerとbyte数取得確認。
 * @details trivially copyable objectを入力し、ObjectBytesが同一objectのbyte先頭を指し、
 * ObjectByteSizeがsizeofと一致することを確認。
 * @pre C++17以降。
 * @post 入力objectと外部状態の変更なし。
 */
TEST(KetMemoryTest, ReadsObjectBytesAndSize)
{
	const auto value = std::uint32_t{0x12345678U};
	const auto* const expected_bytes = reinterpret_cast<const unsigned char*>(&value);

	const auto* const bytes = ket::memory::ObjectBytes(value);
	const auto byte_size = ket::memory::ObjectByteSize(value);

	EXPECT_EQ(bytes, expected_bytes);
	EXPECT_EQ(byte_size, sizeof(value));
}
