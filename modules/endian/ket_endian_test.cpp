#include "ket_endian.h"

#include <array>
#include <cstddef>
#include <cstdint>
// IWYU pragma: no_include <string>

#include <gtest/gtest.h>

namespace
{
	constexpr auto kSwap16 = ket::endian::ByteSwap16(static_cast<std::uint16_t>(0x1234U));
	constexpr auto kSwap32 = ket::endian::ByteSwap32(std::uint32_t{0x12345678U});
	constexpr auto kSwap64 = ket::endian::ByteSwap64(std::uint64_t{0x0123456789ABCDEFULL});
	constexpr auto kSwapZero64 = ket::endian::ByteSwap64(std::uint64_t{0U});

	static_assert(kSwap16 == static_cast<std::uint16_t>(0x3412U), "ByteSwap16 is constexpr");
	static_assert(kSwap32 == std::uint32_t{0x78563412U}, "ByteSwap32 is constexpr");
	static_assert(kSwap64 == std::uint64_t{0xEFCDAB8967452301ULL}, "ByteSwap64 is constexpr");
	static_assert(kSwapZero64 == std::uint64_t{0U}, "ByteSwap64 keeps zero");

	template <std::size_t Size>
	bool ArrayEquals(const std::array<std::uint8_t, Size>& lhs,
					 const std::array<std::uint8_t, Size>& rhs) noexcept
	{
		return lhs == rhs;
	}

} // namespace

/**
 * @test
 * @brief byte順反転の代表値確認。
 * @details 16/32/64bitの代表値と全bitセット値を入力し、byte順だけが逆転することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEndianTest, SwapsBytes)
{
	const auto swapped16 = ket::endian::ByteSwap16(static_cast<std::uint16_t>(0x00FFU));
	const auto swapped32 = ket::endian::ByteSwap32(std::uint32_t{0xA1B2C3D4U});
	const auto swapped64 = ket::endian::ByteSwap64(std::uint64_t{0x1020304050607080ULL});
	const auto all_bits16 = ket::endian::ByteSwap16(static_cast<std::uint16_t>(0xFFFFU));

	EXPECT_EQ(swapped16, static_cast<std::uint16_t>(0xFF00U));
	EXPECT_EQ(swapped32, std::uint32_t{0xD4C3B2A1U});
	EXPECT_EQ(swapped64, std::uint64_t{0x8070605040302010ULL});
	EXPECT_EQ(all_bits16, static_cast<std::uint16_t>(0xFFFFU));
}

/**
 * @test
 * @brief big-endian byte列の読み取り確認。
 * @details unaligned address相当の位置から16/32/64bit幅を読み取り、上位byte順で値になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEndianTest, LoadsBigEndianValues)
{
	const auto bytes16 = std::array<std::uint8_t, 3>{{0xFEU, 0x12U, 0x34U}};
	const auto bytes32 = std::array<std::uint8_t, 5>{{0xFEU, 0x12U, 0x34U, 0x56U, 0x78U}};
	const auto bytes64 = std::array<std::uint8_t, 9>{
		{0xFEU, 0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU}};

	const auto value16 = ket::endian::LoadBe16(bytes16.data() + 1U);
	const auto value32 = ket::endian::LoadBe32(bytes32.data() + 1U);
	const auto value64 = ket::endian::LoadBe64(bytes64.data() + 1U);

	EXPECT_EQ(value16, static_cast<std::uint16_t>(0x1234U));
	EXPECT_EQ(value32, std::uint32_t{0x12345678U});
	EXPECT_EQ(value64, std::uint64_t{0x0123456789ABCDEFULL});
}

/**
 * @test
 * @brief little-endian byte列の読み取り確認。
 * @details unaligned address相当の位置から16/32/64bit幅を読み取り、下位byte順で値になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEndianTest, LoadsLittleEndianValues)
{
	const auto bytes16 = std::array<std::uint8_t, 3>{{0xFEU, 0x34U, 0x12U}};
	const auto bytes32 = std::array<std::uint8_t, 5>{{0xFEU, 0x78U, 0x56U, 0x34U, 0x12U}};
	const auto bytes64 = std::array<std::uint8_t, 9>{
		{0xFEU, 0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U}};

	const auto value16 = ket::endian::LoadLe16(bytes16.data() + 1U);
	const auto value32 = ket::endian::LoadLe32(bytes32.data() + 1U);
	const auto value64 = ket::endian::LoadLe64(bytes64.data() + 1U);

	EXPECT_EQ(value16, static_cast<std::uint16_t>(0x1234U));
	EXPECT_EQ(value32, std::uint32_t{0x12345678U});
	EXPECT_EQ(value64, std::uint64_t{0x0123456789ABCDEFULL});
}

/**
 * @test
 * @brief big-endian byte列への書き込み確認。
 * @details unaligned
 * address相当の位置へ16/32/64bit幅を書き込み、対象範囲だけが上位byte順で変化することを確認。
 * @pre C++17以降。
 * @post 書き込み対象bufferは期待するbyte列へ変化。その他の外部状態の変更なし。
 */
TEST(KetEndianTest, StoresBigEndianValues)
{
	std::array<std::uint8_t, 10> buffer16{{0xEEU, 0U, 0U, 0xEEU, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 10> buffer32{{0xEEU, 0U, 0U, 0U, 0U, 0xEEU, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 10> buffer64{{0xEEU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0xEEU}};

	ket::endian::StoreBe16(buffer16.data() + 1U, static_cast<std::uint16_t>(0x1234U));
	ket::endian::StoreBe32(buffer32.data() + 1U, std::uint32_t{0x12345678U});
	ket::endian::StoreBe64(buffer64.data() + 1U, std::uint64_t{0x0123456789ABCDEFULL});

	const auto expected16 =
		std::array<std::uint8_t, 10>{{0xEEU, 0x12U, 0x34U, 0xEEU, 0U, 0U, 0U, 0U, 0U, 0U}};
	const auto expected32 =
		std::array<std::uint8_t, 10>{{0xEEU, 0x12U, 0x34U, 0x56U, 0x78U, 0xEEU, 0U, 0U, 0U, 0U}};
	const auto expected64 = std::array<std::uint8_t, 10>{
		{0xEEU, 0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU, 0xEEU}};

	EXPECT_EQ(buffer16, expected16);
	EXPECT_EQ(buffer32, expected32);
	EXPECT_EQ(buffer64, expected64);
}

/**
 * @test
 * @brief little-endian byte列への書き込み確認。
 * @details unaligned
 * address相当の位置へ16/32/64bit幅を書き込み、対象範囲だけが下位byte順で変化することを確認。
 * @pre C++17以降。
 * @post 書き込み対象bufferは期待するbyte列へ変化。その他の外部状態の変更なし。
 */
TEST(KetEndianTest, StoresLittleEndianValues)
{
	std::array<std::uint8_t, 10> buffer16{{0xEEU, 0U, 0U, 0xEEU, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 10> buffer32{{0xEEU, 0U, 0U, 0U, 0U, 0xEEU, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 10> buffer64{{0xEEU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0xEEU}};

	ket::endian::StoreLe16(buffer16.data() + 1U, static_cast<std::uint16_t>(0x1234U));
	ket::endian::StoreLe32(buffer32.data() + 1U, std::uint32_t{0x12345678U});
	ket::endian::StoreLe64(buffer64.data() + 1U, std::uint64_t{0x0123456789ABCDEFULL});

	const auto expected16 =
		std::array<std::uint8_t, 10>{{0xEEU, 0x34U, 0x12U, 0xEEU, 0U, 0U, 0U, 0U, 0U, 0U}};
	const auto expected32 =
		std::array<std::uint8_t, 10>{{0xEEU, 0x78U, 0x56U, 0x34U, 0x12U, 0xEEU, 0U, 0U, 0U, 0U}};
	const auto expected64 = std::array<std::uint8_t, 10>{
		{0xEEU, 0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U, 0xEEU}};

	EXPECT_EQ(buffer16, expected16);
	EXPECT_EQ(buffer32, expected32);
	EXPECT_EQ(buffer64, expected64);
}

/**
 * @test
 * @brief TryLoadの成功確認。
 * @details BE/LEそれぞれの16/32/64bit入力を十分なsizeで渡し、trueと読み取り値を返すことを確認。
 * @pre C++17以降。
 * @post 出力変数は期待値へ変化。入力bufferと外部状態の変更なし。
 */
TEST(KetEndianTest, TryLoadsValues)
{
	const auto be =
		std::array<std::uint8_t, 8>{{0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU}};
	const auto le =
		std::array<std::uint8_t, 8>{{0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U}};
	std::uint16_t be16 = 0U;
	std::uint32_t be32 = 0U;
	std::uint64_t be64 = 0U;
	std::uint16_t le16 = 0U;
	std::uint32_t le32 = 0U;
	std::uint64_t le64 = 0U;

	const auto loaded_be16 = ket::endian::TryLoadBe16(be.data(), be.size(), be16);
	const auto loaded_be32 = ket::endian::TryLoadBe32(be.data(), be.size(), be32);
	const auto loaded_be64 = ket::endian::TryLoadBe64(be.data(), be.size(), be64);
	const auto loaded_le16 = ket::endian::TryLoadLe16(le.data(), le.size(), le16);
	const auto loaded_le32 = ket::endian::TryLoadLe32(le.data(), le.size(), le32);
	const auto loaded_le64 = ket::endian::TryLoadLe64(le.data(), le.size(), le64);

	EXPECT_TRUE(loaded_be16);
	EXPECT_TRUE(loaded_be32);
	EXPECT_TRUE(loaded_be64);
	EXPECT_TRUE(loaded_le16);
	EXPECT_TRUE(loaded_le32);
	EXPECT_TRUE(loaded_le64);
	EXPECT_EQ(be16, static_cast<std::uint16_t>(0x0123U));
	EXPECT_EQ(be32, std::uint32_t{0x01234567U});
	EXPECT_EQ(be64, std::uint64_t{0x0123456789ABCDEFULL});
	EXPECT_EQ(le16, static_cast<std::uint16_t>(0xCDEFU));
	EXPECT_EQ(le32, std::uint32_t{0x89ABCDEFU});
	EXPECT_EQ(le64, std::uint64_t{0x0123456789ABCDEFULL});
}

/**
 * @test
 * @brief TryLoadの失敗時出力不変確認。
 * @details nullptrとsize不足を渡し、falseを返して出力値を変更しないことを確認。
 * @pre C++17以降。
 * @post 出力変数は入力時の値を保持。入力bufferと外部状態の変更なし。
 */
TEST(KetEndianTest, TryLoadFailureKeepsOutputUnchanged)
{
	const auto bytes =
		std::array<std::uint8_t, 8>{{0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU}};
	std::uint16_t value16 = 0xBEEFU;
	std::uint32_t value32 = 0xDEADBEEFU;
	std::uint64_t value64 = 0x0123456789ABCDEFULL;

	const auto null_failed = ket::endian::TryLoadBe16(nullptr, 2U, value16);
	const auto short_be32_failed = ket::endian::TryLoadBe32(bytes.data(), 3U, value32);
	const auto short_le64_failed = ket::endian::TryLoadLe64(bytes.data(), 7U, value64);

	EXPECT_FALSE(null_failed);
	EXPECT_FALSE(short_be32_failed);
	EXPECT_FALSE(short_le64_failed);
	EXPECT_EQ(value16, static_cast<std::uint16_t>(0xBEEFU));
	EXPECT_EQ(value32, std::uint32_t{0xDEADBEEFU});
	EXPECT_EQ(value64, std::uint64_t{0x0123456789ABCDEFULL});
}

/**
 * @test
 * @brief TryStoreの成功確認。
 * @details BE/LEそれぞれの16/32/64bit値を十分なsizeで渡し、trueと期待するbyte列を返すことを確認。
 * @pre C++17以降。
 * @post 書き込み対象bufferは期待するbyte列へ変化。その他の外部状態の変更なし。
 */
TEST(KetEndianTest, TryStoresValues)
{
	std::array<std::uint8_t, 8> be16{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 8> be32{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 8> be64{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 8> le16{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 8> le32{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
	std::array<std::uint8_t, 8> le64{{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};

	const auto stored_be16 =
		ket::endian::TryStoreBe16(be16.data(), be16.size(), static_cast<std::uint16_t>(0x0123U));
	const auto stored_be32 = ket::endian::TryStoreBe32(be32.data(), be32.size(), 0x01234567U);
	const auto stored_be64 =
		ket::endian::TryStoreBe64(be64.data(), be64.size(), 0x0123456789ABCDEFULL);
	const auto stored_le16 =
		ket::endian::TryStoreLe16(le16.data(), le16.size(), static_cast<std::uint16_t>(0x0123U));
	const auto stored_le32 = ket::endian::TryStoreLe32(le32.data(), le32.size(), 0x01234567U);
	const auto stored_le64 =
		ket::endian::TryStoreLe64(le64.data(), le64.size(), 0x0123456789ABCDEFULL);

	const auto expected_be16 = std::array<std::uint8_t, 8>{{0x01U, 0x23U, 0U, 0U, 0U, 0U, 0U, 0U}};
	const auto expected_be32 =
		std::array<std::uint8_t, 8>{{0x01U, 0x23U, 0x45U, 0x67U, 0U, 0U, 0U, 0U}};
	const auto expected_be64 =
		std::array<std::uint8_t, 8>{{0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU}};
	const auto expected_le16 = std::array<std::uint8_t, 8>{{0x23U, 0x01U, 0U, 0U, 0U, 0U, 0U, 0U}};
	const auto expected_le32 =
		std::array<std::uint8_t, 8>{{0x67U, 0x45U, 0x23U, 0x01U, 0U, 0U, 0U, 0U}};
	const auto expected_le64 =
		std::array<std::uint8_t, 8>{{0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U}};

	EXPECT_TRUE(stored_be16);
	EXPECT_TRUE(stored_be32);
	EXPECT_TRUE(stored_be64);
	EXPECT_TRUE(stored_le16);
	EXPECT_TRUE(stored_le32);
	EXPECT_TRUE(stored_le64);
	EXPECT_EQ(be16, expected_be16);
	EXPECT_EQ(be32, expected_be32);
	EXPECT_EQ(be64, expected_be64);
	EXPECT_EQ(le16, expected_le16);
	EXPECT_EQ(le32, expected_le32);
	EXPECT_EQ(le64, expected_le64);
}

/**
 * @test
 * @brief TryStoreの失敗時buffer不変確認。
 * @details nullptrとsize不足を渡し、falseを返して既存bufferを変更しないことを確認。
 * @pre C++17以降。
 * @post 書き込み対象bufferは入力時のbyte列を保持。その他の外部状態の変更なし。
 */
TEST(KetEndianTest, TryStoreFailureKeepsBufferUnchanged)
{
	std::array<std::uint8_t, 8> buffer16{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x11U, 0x22U}};
	std::array<std::uint8_t, 8> buffer32{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x11U, 0x22U}};
	std::array<std::uint8_t, 8> buffer64{{0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x11U, 0x22U}};
	const auto expected16 = buffer16;
	const auto expected32 = buffer32;
	const auto expected64 = buffer64;

	const auto null_failed =
		ket::endian::TryStoreBe16(nullptr, 2U, static_cast<std::uint16_t>(0x1234U));
	const auto short_be32_failed = ket::endian::TryStoreBe32(buffer32.data(), 3U, 0x12345678U);
	const auto short_le64_failed =
		ket::endian::TryStoreLe64(buffer64.data(), 7U, 0x0123456789ABCDEFULL);
	const auto size_zero_failed =
		ket::endian::TryStoreLe16(buffer16.data(), 0U, static_cast<std::uint16_t>(0x1234U));

	const auto buffer16_unchanged = ArrayEquals(buffer16, expected16);
	const auto buffer32_unchanged = ArrayEquals(buffer32, expected32);
	const auto buffer64_unchanged = ArrayEquals(buffer64, expected64);

	EXPECT_FALSE(null_failed);
	EXPECT_FALSE(short_be32_failed);
	EXPECT_FALSE(short_le64_failed);
	EXPECT_FALSE(size_zero_failed);
	EXPECT_TRUE(buffer16_unchanged);
	EXPECT_TRUE(buffer32_unchanged);
	EXPECT_TRUE(buffer64_unchanged);
}
