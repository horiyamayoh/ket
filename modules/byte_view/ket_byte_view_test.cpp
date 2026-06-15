#include "ket_byte_view.h"

#include <array>
#include <cstdint>
#include <string> // NOLINT(misc-include-cleaner)

#include <gtest/gtest.h>

/**
 * @test
 * @brief default構築と`nullptr + 0`空viewの確認。
 * @details 読み取り専用viewと書き込み可能viewを空状態で構築し、空sliceだけが成功することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetByteViewTest, CreatesDefaultAndNullEmptyViews)
{
	const ket::byte_view::View default_view;
	const ket::byte_view::View null_empty_view(nullptr, 0U);
	const ket::byte_view::MutableView default_mutable_view;
	const ket::byte_view::MutableView null_empty_mutable_view(nullptr, 0U);

	const auto default_view_is_empty = default_view.Empty();
	const auto null_empty_view_is_empty = null_empty_view.Empty();
	const auto default_mutable_view_is_empty = default_mutable_view.Empty();
	const auto null_empty_mutable_view_is_empty = null_empty_mutable_view.Empty();

	EXPECT_EQ(default_view.Data(), nullptr);
	EXPECT_EQ(default_view.Size(), 0U);
	EXPECT_TRUE(default_view_is_empty);
	EXPECT_EQ(null_empty_view.Data(), nullptr);
	EXPECT_EQ(null_empty_view.Size(), 0U);
	EXPECT_TRUE(null_empty_view_is_empty);
	EXPECT_EQ(default_mutable_view.Data(), nullptr);
	EXPECT_EQ(default_mutable_view.Size(), 0U);
	EXPECT_TRUE(default_mutable_view_is_empty);
	EXPECT_EQ(null_empty_mutable_view.Data(), nullptr);
	EXPECT_EQ(null_empty_mutable_view.Size(), 0U);
	EXPECT_TRUE(null_empty_mutable_view_is_empty);

	const std::array<std::uint8_t, 1> immutable_sentinel{{0xA5U}};
	std::array<std::uint8_t, 1> mutable_sentinel{{0x5AU}};
	std::uint8_t read_value = 0x5AU;
	ket::byte_view::View empty_slice(immutable_sentinel.data(), immutable_sentinel.size());
	ket::byte_view::MutableView empty_mutable_slice(mutable_sentinel.data(),
													mutable_sentinel.size());

	const auto read_empty = null_empty_view.TryAt(0U, read_value);
	const auto slice_empty = null_empty_view.TrySlice(0U, 0U, empty_slice);
	const auto mutable_slice_empty = null_empty_mutable_view.TrySlice(0U, 0U, empty_mutable_slice);

	EXPECT_FALSE(read_empty);
	EXPECT_EQ(read_value, std::uint8_t{0x5AU});
	EXPECT_TRUE(slice_empty);
	EXPECT_EQ(empty_slice.Data(), nullptr);
	EXPECT_EQ(empty_slice.Size(), 0U);
	EXPECT_TRUE(mutable_slice_empty);
	EXPECT_EQ(empty_mutable_slice.Data(), nullptr);
	EXPECT_EQ(empty_mutable_slice.Size(), 0U);
}

/**
 * @test
 * @brief 読み取り専用viewのnon-owning性確認。
 * @details
 * 元bufferを参照するviewを構築し、元buffer変更後のbyte取得に同じstorageが反映されることを確認。
 * @pre C++17以降。
 * @post 元bufferは試験内の期待値へ変更。viewは所有権を取得しない。
 */
TEST(KetByteViewTest, ReadsBytesFromOriginalBufferWithoutOwning)
{
	std::array<std::uint8_t, 3> bytes{{0x10U, 0x20U, 0x30U}};
	const ket::byte_view::View view(bytes.data(), bytes.size());

	const auto view_is_empty = view.Empty();
	std::uint8_t first = 0U;
	std::uint8_t second = 0U;

	const auto read_first = view.TryAt(0U, first);
	const auto read_second = view.TryAt(1U, second);

	bytes[1] = 0x7FU;
	std::uint8_t changed_second = 0U;
	const auto read_changed_second = view.TryAt(1U, changed_second);

	EXPECT_EQ(view.Data(), bytes.data());
	EXPECT_EQ(view.Size(), bytes.size());
	EXPECT_FALSE(view_is_empty);
	EXPECT_TRUE(read_first);
	EXPECT_EQ(first, std::uint8_t{0x10U});
	EXPECT_TRUE(read_second);
	EXPECT_EQ(second, std::uint8_t{0x20U});
	EXPECT_TRUE(read_changed_second);
	EXPECT_EQ(changed_second, std::uint8_t{0x7FU});
}

/**
 * @test
 * @brief invalid viewのaccess拒否確認。
 * @details `nullptr +
 * 非0`で構築したviewに対し、byte取得、slice、書き込みが失敗して出力とbufferを変えないことを確認。
 * @pre C++17以降。
 * @post 出力引数と比較用bufferは入力時の値を保持。
 */
TEST(KetByteViewTest, RejectsInvalidNullNonEmptyViews)
{
	std::array<std::uint8_t, 2> sentinel{{0xA0U, 0xB0U}};
	const ket::byte_view::View invalid_view(nullptr, 2U);
	ket::byte_view::View immutable_output(sentinel.data(), sentinel.size());
	std::uint8_t immutable_value = 0xCCU;

	const auto immutable_read = invalid_view.TryAt(0U, immutable_value);
	const auto immutable_slice = invalid_view.TrySlice(0U, 0U, immutable_output);

	ket::byte_view::MutableView invalid_mutable_view(nullptr, 2U);
	ket::byte_view::MutableView mutable_output(sentinel.data(), sentinel.size());
	std::uint8_t mutable_value = 0xDDU;

	const auto mutable_read = invalid_mutable_view.TryAt(0U, mutable_value);
	const auto mutable_write = invalid_mutable_view.TrySet(0U, 0xEEU);
	const auto mutable_slice = invalid_mutable_view.TrySlice(0U, 0U, mutable_output);

	EXPECT_FALSE(immutable_read);
	EXPECT_EQ(immutable_value, std::uint8_t{0xCCU});
	EXPECT_FALSE(immutable_slice);
	EXPECT_EQ(immutable_output.Data(), sentinel.data());
	EXPECT_EQ(immutable_output.Size(), sentinel.size());
	EXPECT_FALSE(mutable_read);
	EXPECT_EQ(mutable_value, std::uint8_t{0xDDU});
	EXPECT_FALSE(mutable_write);
	EXPECT_FALSE(mutable_slice);
	EXPECT_EQ(mutable_output.Data(), sentinel.data());
	EXPECT_EQ(mutable_output.Size(), sentinel.size());
	EXPECT_EQ(sentinel[0], std::uint8_t{0xA0U});
	EXPECT_EQ(sentinel[1], std::uint8_t{0xB0U});
}

/**
 * @test
 * @brief 読み取り専用viewの境界index確認。
 * @details 先頭、末尾、範囲外indexを指定し、範囲外accessでは出力が不変であることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetByteViewTest, ReadsOnlyWithinBounds)
{
	const std::array<std::uint8_t, 2> bytes{{0x01U, 0x02U}};
	const ket::byte_view::View view(bytes.data(), bytes.size());

	std::uint8_t first = 0U;
	std::uint8_t last = 0U;
	std::uint8_t out_of_bounds = 0xEFU;

	const auto read_first = view.TryAt(0U, first);
	const auto read_last = view.TryAt(1U, last);
	const auto read_out_of_bounds = view.TryAt(2U, out_of_bounds);

	EXPECT_TRUE(read_first);
	EXPECT_EQ(first, std::uint8_t{0x01U});
	EXPECT_TRUE(read_last);
	EXPECT_EQ(last, std::uint8_t{0x02U});
	EXPECT_FALSE(read_out_of_bounds);
	EXPECT_EQ(out_of_bounds, std::uint8_t{0xEFU});
}

/**
 * @test
 * @brief 読み取り専用viewのslice境界確認。
 * @details 全体、中央、末尾空sliceが成功し、範囲超過sliceでは出力viewが不変であることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetByteViewTest, SlicesImmutableViewWithinBounds)
{
	const std::array<std::uint8_t, 4> bytes{{0x10U, 0x20U, 0x30U, 0x40U}};
	const ket::byte_view::View view(bytes.data(), bytes.size());

	ket::byte_view::View full;
	ket::byte_view::View middle;
	ket::byte_view::View end_empty;
	ket::byte_view::View failed(bytes.data(), 1U);

	const auto slice_full = view.TrySlice(0U, bytes.size(), full);
	const auto slice_middle = view.TrySlice(1U, 2U, middle);
	const auto slice_end_empty = view.TrySlice(bytes.size(), 0U, end_empty);
	const auto slice_too_large_count = view.TrySlice(3U, 2U, failed);
	const auto slice_too_large_offset = view.TrySlice(5U, 0U, failed);

	std::uint8_t middle_first = 0U;
	const auto read_middle_first = middle.TryAt(0U, middle_first);

	EXPECT_TRUE(slice_full);
	EXPECT_EQ(full.Data(), bytes.data());
	EXPECT_EQ(full.Size(), bytes.size());
	EXPECT_TRUE(slice_middle);
	EXPECT_EQ(middle.Data(), bytes.data() + 1U);
	EXPECT_EQ(middle.Size(), 2U);
	EXPECT_TRUE(read_middle_first);
	EXPECT_EQ(middle_first, std::uint8_t{0x20U});
	EXPECT_TRUE(slice_end_empty);
	EXPECT_EQ(end_empty.Data(), bytes.data() + bytes.size());
	EXPECT_EQ(end_empty.Size(), 0U);
	EXPECT_FALSE(slice_too_large_count);
	EXPECT_FALSE(slice_too_large_offset);
	EXPECT_EQ(failed.Data(), bytes.data());
	EXPECT_EQ(failed.Size(), 1U);
}

/**
 * @test
 * @brief 書き込み可能viewの範囲内更新確認。
 * @details
 * 範囲内indexの更新だけが元bufferへ反映され、範囲外更新では元bufferが不変であることを確認。
 * @pre C++17以降。
 * @post 元bufferは範囲内更新だけを反映した値へ変化。
 */
TEST(KetByteViewTest, SetsMutableBytesOnlyWithinBounds)
{
	std::array<std::uint8_t, 3> bytes{{0x01U, 0x02U, 0x03U}};
	ket::byte_view::MutableView view(bytes.data(), bytes.size());

	const auto set_middle = view.TrySet(1U, 0xAAU);
	std::uint8_t middle = 0U;
	const auto read_middle = view.TryAt(1U, middle);
	const auto set_out_of_bounds = view.TrySet(3U, 0xFFU);

	EXPECT_TRUE(set_middle);
	EXPECT_TRUE(read_middle);
	EXPECT_EQ(middle, std::uint8_t{0xAAU});
	EXPECT_FALSE(set_out_of_bounds);
	EXPECT_EQ(bytes[0], std::uint8_t{0x01U});
	EXPECT_EQ(bytes[1], std::uint8_t{0xAAU});
	EXPECT_EQ(bytes[2], std::uint8_t{0x03U});
}

/**
 * @test
 * @brief 書き込み可能viewのslice境界確認。
 * @details
 * 範囲内slice経由の更新が元bufferへ反映され、範囲超過sliceでは出力viewが不変であることを確認。
 * @pre C++17以降。
 * @post 元bufferは範囲内slice経由の更新だけを反映した値へ変化。
 */
TEST(KetByteViewTest, SlicesMutableViewWithinBounds)
{
	std::array<std::uint8_t, 4> bytes{{0x10U, 0x20U, 0x30U, 0x40U}};
	const ket::byte_view::MutableView view(bytes.data(), bytes.size());

	ket::byte_view::MutableView middle;
	ket::byte_view::MutableView end_empty;
	ket::byte_view::MutableView failed(bytes.data(), 1U);

	const auto slice_middle = view.TrySlice(1U, 2U, middle);
	const auto set_middle_first = middle.TrySet(0U, 0xBBU);
	const auto slice_end_empty = view.TrySlice(bytes.size(), 0U, end_empty);
	const auto slice_too_large_count = view.TrySlice(3U, 2U, failed);
	const auto slice_too_large_offset = view.TrySlice(5U, 0U, failed);

	EXPECT_TRUE(slice_middle);
	EXPECT_EQ(middle.Data(), bytes.data() + 1U);
	EXPECT_EQ(middle.Size(), 2U);
	EXPECT_TRUE(set_middle_first);
	EXPECT_EQ(bytes[1], std::uint8_t{0xBBU});
	EXPECT_TRUE(slice_end_empty);
	EXPECT_EQ(end_empty.Data(), bytes.data() + bytes.size());
	EXPECT_EQ(end_empty.Size(), 0U);
	EXPECT_FALSE(slice_too_large_count);
	EXPECT_FALSE(slice_too_large_offset);
	EXPECT_EQ(failed.Data(), bytes.data());
	EXPECT_EQ(failed.Size(), 1U);
	EXPECT_EQ(bytes[0], std::uint8_t{0x10U});
	EXPECT_EQ(bytes[2], std::uint8_t{0x30U});
	EXPECT_EQ(bytes[3], std::uint8_t{0x40U});
}
