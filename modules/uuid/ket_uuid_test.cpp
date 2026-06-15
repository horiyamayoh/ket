#include "ket_uuid.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
	constexpr std::size_t kUuidByteCount = 16U;

	bool UuidBytesEqual(const ket::uuid::Uuid& value,
						const std::array<std::uint8_t, kUuidByteCount>& expected) noexcept
	{
		for (std::size_t index = 0U; index < kUuidByteCount; ++index)
		{
			if (value.bytes[index] != expected[index])
			{
				return false;
			}
		}

		return true;
	}

	bool OptionalUuidBytesEqual(const std::optional<ket::uuid::Uuid>& value,
								const std::array<std::uint8_t, kUuidByteCount>& expected) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return UuidBytesEqual(*value, expected);
	}

} // namespace

/**
 * @test
 * @brief zero UUID のparse確認。
 * @details 全桁0のcanonical UUIDを入力し、16 byteすべてが0としてparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, ParsesZeroUuid)
{
	const auto value = ket::uuid::Parse("00000000-0000-0000-0000-000000000000");
	const auto expected = std::array<std::uint8_t, kUuidByteCount>{};

	const auto bytes_match = OptionalUuidBytesEqual(value, expected);
	EXPECT_TRUE(bytes_match);
}

/**
 * @test
 * @brief 通常 UUID のparse確認。
 * @details 0からffまでの代表byte列を含むcanonical
 * UUIDを入力し、文字列順のbyte列へparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, ParsesNormalUuid)
{
	const auto value = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeeff");
	const auto expected = std::array<std::uint8_t, kUuidByteCount>{{0x00U,
																	0x11U,
																	0x22U,
																	0x33U,
																	0x44U,
																	0x55U,
																	0x66U,
																	0x77U,
																	0x88U,
																	0x99U,
																	0xaaU,
																	0xbbU,
																	0xccU,
																	0xddU,
																	0xeeU,
																	0xffU}};

	const auto bytes_match = OptionalUuidBytesEqual(value, expected);
	EXPECT_TRUE(bytes_match);
}

/**
 * @test
 * @brief upper-case UUID 入力のparse確認。
 * @details upper-case hexを含むcanonical
 * UUIDを入力し、lower-case入力と同じbyte列へparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, ParsesUppercaseUuid)
{
	const auto value = ket::uuid::Parse("ABCDEFAB-CDEF-ABCD-EFAB-CDEFABCDEFAB");
	const auto expected = std::array<std::uint8_t, kUuidByteCount>{{0xabU,
																	0xcdU,
																	0xefU,
																	0xabU,
																	0xcdU,
																	0xefU,
																	0xabU,
																	0xcdU,
																	0xefU,
																	0xabU,
																	0xcdU,
																	0xefU,
																	0xabU,
																	0xcdU,
																	0xefU,
																	0xabU}};

	const auto bytes_match = OptionalUuidBytesEqual(value, expected);
	EXPECT_TRUE(bytes_match);
}

/**
 * @test
 * @brief UUID parse の長さ不一致拒否確認。
 * @details
 * 空文字、短すぎる文字列、長すぎる文字列、brace付き形式を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, RejectsBadLengthAndBracedForm)
{
	const auto empty = ket::uuid::Parse("");
	const auto short_text = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeef");
	const auto long_text = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeeff0");
	const auto braced_text = ket::uuid::Parse("{00112233-4455-6677-8899-aabbccddeeff}");

	EXPECT_EQ(empty, std::nullopt);
	EXPECT_EQ(short_text, std::nullopt);
	EXPECT_EQ(long_text, std::nullopt);
	EXPECT_EQ(braced_text, std::nullopt);
}

/**
 * @test
 * @brief UUID parse のhyphen位置不一致拒否確認。
 * @details
 * 必須hyphen位置の欠落とhex位置の余分なhyphenを含む文字列を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, RejectsBadHyphenPlacement)
{
	std::string missing_required_hyphen = "00112233-4455-6677-8899-aabbccddeeff";
	std::string extra_hyphen = "00112233-4455-6677-8899-aabbccddeeff";
	missing_required_hyphen[8] = '0';
	extra_hyphen[9] = '-';

	const auto missing_required = ket::uuid::Parse(missing_required_hyphen);
	const auto unexpected_hyphen = ket::uuid::Parse(extra_hyphen);

	EXPECT_EQ(missing_required, std::nullopt);
	EXPECT_EQ(unexpected_hyphen, std::nullopt);
}

/**
 * @test
 * @brief UUID parse の不正hex拒否確認。
 * @details
 * canonical長とhyphen位置を満たすがhexではない文字を含む文字列を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, RejectsBadHex)
{
	std::string invalid_first = "00112233-4455-6677-8899-aabbccddeeff";
	std::string invalid_last = "00112233-4455-6677-8899-aabbccddeeff";
	invalid_first[0] = 'g';
	invalid_last[35] = '_';

	const auto bad_first = ket::uuid::Parse(invalid_first);
	const auto bad_last = ket::uuid::Parse(invalid_last);

	EXPECT_EQ(bad_first, std::nullopt);
	EXPECT_EQ(bad_last, std::nullopt);
}

/**
 * @test
 * @brief UUID のcanonical文字列化確認。
 * @details 16 byteのUUID値を入力し、lower-case固定のcanonical hyphen形式へformatされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, FormatsUuidAsLowercaseCanonicalText)
{
	const auto value = ket::uuid::Uuid{{0xabU,
										0xcdU,
										0xefU,
										0xabU,
										0xcdU,
										0xefU,
										0xabU,
										0xcdU,
										0xefU,
										0xabU,
										0xcdU,
										0xefU,
										0xabU,
										0xcdU,
										0xefU,
										0xabU}};

	const auto text = ket::uuid::Format(value);

	EXPECT_EQ(text, std::string("abcdefab-cdef-abcd-efab-cdefabcdefab"));
}
