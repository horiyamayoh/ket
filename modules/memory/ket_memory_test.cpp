#include "ket_memory.h"

#include <array>
#include <cstdint>
#include <limits>
#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

namespace
{
	void ZeroNullNonZero()
	{
		ket::memory::Zero(nullptr, 1U);
	}

	void SecureZeroNullNonZero()
	{
		ket::memory::SecureZero(nullptr, 1U);
	}

} // namespace

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
 * alignment済みbufferの途中pointer、既にalignedなpointer、alignment 1を入力し、
 * 直前と直後のalignment境界へ変換されることを確認。
 * @pre C++17以降。
 * @post 出力変数以外の外部状態の変更なし。
 */
TEST(KetMemoryTest, AlignsPointerUpAndDown)
{
	alignas(16) unsigned char buffer[64] = {};
	const void* const input = static_cast<const void*>(buffer + 1U);
	const void* const expected_up = static_cast<const void*>(buffer + 16U);
	const void* const expected_down = static_cast<const void*>(buffer);
	const void* const documented_up_input = static_cast<const void*>(buffer + 1U);
	const void* const documented_up = static_cast<const void*>(buffer + 8U);
	const void* const documented_down_input = static_cast<const void*>(buffer + 7U);
	const void* const already_aligned = static_cast<const void*>(buffer + 16U);
	const void* aligned_up = nullptr;
	const void* aligned_down = nullptr;
	const void* aligned_up_to_8 = nullptr;
	const void* aligned_down_to_8 = nullptr;
	const void* aligned_up_same = nullptr;
	const void* aligned_down_same = nullptr;
	const void* aligned_to_1 = nullptr;
	void* mutable_input = buffer + 1U;
	void* mutable_aligned_up = nullptr;
	void* mutable_aligned_down = nullptr;
	unsigned char* typed_aligned_up = nullptr;
	unsigned char* typed_aligned_down = nullptr;

	const auto align_up_succeeded = ket::memory::TryAlignUp(input, 16U, aligned_up);
	const auto align_down_succeeded = ket::memory::TryAlignDown(input, 16U, aligned_down);
	const auto align_up_to_8_succeeded =
		ket::memory::TryAlignUp(documented_up_input, 8U, aligned_up_to_8);
	const auto align_down_to_8_succeeded =
		ket::memory::TryAlignDown(documented_down_input, 8U, aligned_down_to_8);
	const auto align_up_same_succeeded =
		ket::memory::TryAlignUp(already_aligned, 8U, aligned_up_same);
	const auto align_down_same_succeeded =
		ket::memory::TryAlignDown(already_aligned, 8U, aligned_down_same);
	const auto align_to_1_succeeded = ket::memory::TryAlignUp(input, 1U, aligned_to_1);
	const auto mutable_align_up_succeeded =
		ket::memory::TryAlignUp(mutable_input, 16U, mutable_aligned_up);
	const auto mutable_align_down_succeeded =
		ket::memory::TryAlignDown(mutable_input, 16U, mutable_aligned_down);
	const auto typed_align_up_succeeded =
		ket::memory::TryAlignUp(buffer + 1U, 16U, typed_aligned_up);
	const auto typed_align_down_succeeded =
		ket::memory::TryAlignDown(buffer + 1U, 16U, typed_aligned_down);

	EXPECT_TRUE(align_up_succeeded);
	EXPECT_TRUE(align_down_succeeded);
	EXPECT_TRUE(align_up_to_8_succeeded);
	EXPECT_TRUE(align_down_to_8_succeeded);
	EXPECT_TRUE(align_up_same_succeeded);
	EXPECT_TRUE(align_down_same_succeeded);
	EXPECT_TRUE(align_to_1_succeeded);
	EXPECT_TRUE(mutable_align_up_succeeded);
	EXPECT_TRUE(mutable_align_down_succeeded);
	EXPECT_TRUE(typed_align_up_succeeded);
	EXPECT_TRUE(typed_align_down_succeeded);
	EXPECT_EQ(aligned_up, expected_up);
	EXPECT_EQ(aligned_down, expected_down);
	EXPECT_EQ(aligned_up_to_8, documented_up);
	EXPECT_EQ(aligned_down_to_8, expected_down);
	EXPECT_EQ(aligned_up_same, already_aligned);
	EXPECT_EQ(aligned_down_same, already_aligned);
	EXPECT_EQ(aligned_to_1, input);
	EXPECT_EQ(mutable_aligned_up, buffer + 16U);
	EXPECT_EQ(mutable_aligned_down, buffer);
	EXPECT_EQ(typed_aligned_up, buffer + 16U);
	EXPECT_EQ(typed_aligned_down, buffer);
}

/**
 * @test
 * @brief pointer alignment計算の失敗時出力保持確認。
 * @details TryAlignUpとTryAlignDownへalignment 0、非power-of-two、null
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

	out = sentinel;
	const auto down_zero_alignment = ket::memory::TryAlignDown(input, 0U, out);
	EXPECT_FALSE(down_zero_alignment);
	EXPECT_EQ(out, sentinel);

	out = sentinel;
	const auto down_non_power_of_two_alignment = ket::memory::TryAlignDown(input, 6U, out);
	EXPECT_FALSE(down_non_power_of_two_alignment);
	EXPECT_EQ(out, sentinel);

	out = sentinel;
	const auto up_null_pointer = ket::memory::TryAlignUp(nullptr, 8U, out);
	EXPECT_FALSE(up_null_pointer);
	EXPECT_EQ(out, sentinel);
}

/**
 * @test
 * @brief pointer alignment切り上げoverflow時の失敗確認。
 * @details address空間末尾近くの非dereference pointerを入力し、切り上げ加算がoverflowする場合に
 * falseを返してoutを変更しないことを確認。
 * @pre C++17以降。`std::uintptr_t`とpointerの相互変換を提供する処理系。
 * @post 出力変数と外部状態の変更なし。
 */
TEST(KetMemoryTest, KeepsOutputUnchangedWhenAlignmentWouldOverflow)
{
	alignas(8) unsigned char buffer[8] = {};
	const auto max_address = (std::numeric_limits<std::uintptr_t>::max)();
	const auto overflowing_address = max_address - std::uintptr_t{1U};
	const void* const input =
		reinterpret_cast<const void*>(overflowing_address); // NOLINT(performance-no-int-to-ptr)
	const void* const sentinel = static_cast<const void*>(buffer);
	const void* out = sentinel;

	const auto align_up_succeeded = ket::memory::TryAlignUp(input, 4U, out);

	EXPECT_FALSE(align_up_succeeded);
	EXPECT_EQ(out, sentinel);
}

/**
 * @test
 * @brief 通常zeroingのbyte変更確認。
 * @details 非ゼロbyte列の一部をZeroへ渡して、指定範囲だけが0になることを確認。
 * @pre C++17以降。
 * @post 入力bufferは指定範囲だけ0に変化。
 */
TEST(KetMemoryTest, ZeroesRequestedByteRange)
{
	std::array<unsigned char, 4> bytes{{1U, 2U, 3U, 4U}};
	const std::array<unsigned char, 4> expected{{1U, 0U, 0U, 4U}};

	ket::memory::Zero(bytes.data() + 1U, 2U);

	EXPECT_EQ(bytes, expected);
}

/**
 * @test
 * @brief secure zeroingのbyte変更確認。
 * @details 非ゼロbyte列の一部をSecureZeroへ渡して、指定範囲だけが0になることを確認。
 * @pre C++17以降。
 * @post 入力bufferは指定範囲だけ0に変化。
 */
TEST(KetMemoryTest, SecureZeroesRequestedByteRange)
{
	std::array<unsigned char, 5> bytes{{9U, 8U, 7U, 6U, 5U}};
	const std::array<unsigned char, 5> expected{{9U, 0U, 0U, 0U, 5U}};

	ket::memory::SecureZero(bytes.data() + 1U, 3U);

	EXPECT_EQ(bytes, expected);
}

/**
 * @test
 * @brief zeroing APIの空範囲no-op確認。
 * @details 非null pointerとsize 0、nullptrとsize 0をZeroとSecureZeroへ渡し、
 * 既存byte列を変更しないことを確認。
 * @pre C++17以降。
 * @post 入力bufferと外部状態の変更なし。
 */
TEST(KetMemoryTest, LeavesBytesUnchangedForEmptyZeroingRange)
{
	std::array<unsigned char, 4> bytes{{1U, 2U, 3U, 4U}};
	const std::array<unsigned char, 4> expected{{1U, 2U, 3U, 4U}};

	ket::memory::Zero(bytes.data(), 0U);
	ket::memory::SecureZero(bytes.data(), 0U);
	ket::memory::Zero(nullptr, 0U);
	ket::memory::SecureZero(nullptr, 0U);

	EXPECT_EQ(bytes, expected);
}

/**
 * @test
 * @brief zeroing APIのnull非0契約違反確認。
 * @details nullptrと非0
 * sizeをZeroとSecureZeroへ渡した場合に、契約違反としてprocessが終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetMemoryTest, TerminatesForNullNonZeroZeroingRange)
{
	EXPECT_DEATH(ZeroNullNonZero(), "");
	EXPECT_DEATH(SecureZeroNullNonZero(), "");
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

	for (decltype(+byte_size) index = 0U; index < byte_size; ++index)
	{
		const auto byte = bytes[index];
		const auto expected_byte = expected_bytes[index];
		EXPECT_EQ(byte, expected_byte);
	}
}
