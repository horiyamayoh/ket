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

	void ExpectUuidBytes(const ket::uuid::Uuid& value,
						 const std::array<std::uint8_t, kUuidByteCount>& expected)
	{
		EXPECT_EQ(value.bytes, expected);
	}

	void ExpectOptionalUuidBytes(const std::optional<ket::uuid::Uuid>& value,
								 const std::array<std::uint8_t, kUuidByteCount>& expected)
	{
		const auto value_has_value = value != std::nullopt;
		ASSERT_TRUE(value_has_value);
		const auto actual = value.value_or(ket::uuid::Uuid{});
		ExpectUuidBytes(actual, expected);
	}

	bool IsRequiredHyphenPosition(std::size_t position) noexcept
	{
		return position == 8U || position == 13U || position == 18U || position == 23U;
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

	ExpectOptionalUuidBytes(value, expected);
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

	ExpectOptionalUuidBytes(value, expected);
}

/**
 * @test
 * @brief all-ff UUID のparse確認。
 * @details 全byteが0xffのcanonical UUIDを入力し、16 byteすべてが0xffとしてparseされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, ParsesAllFfUuid)
{
	const auto value = ket::uuid::Parse("ffffffff-ffff-ffff-ffff-ffffffffffff");
	const auto expected = std::array<std::uint8_t, kUuidByteCount>{{0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU,
																	0xffU}};

	ExpectOptionalUuidBytes(value, expected);
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

	ExpectOptionalUuidBytes(value, expected);
}

/**
 * @test
 * @brief UUID parse の長さ不一致と非canonical形式拒否確認。
 * @details
 * 空文字、短すぎる文字列、長すぎる文字列、brace付き形式、URN形式を入力し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, RejectsBadLengthBracedAndUrnForms)
{
	const auto empty = ket::uuid::Parse("");
	const auto short_text = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeef");
	const auto long_text = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeeff0");
	const auto braced_text = ket::uuid::Parse("{00112233-4455-6677-8899-aabbccddeeff}");
	const auto urn_text = ket::uuid::Parse("urn:uuid:00112233-4455-6677-8899-aabbccddeeff");

	EXPECT_EQ(empty, std::nullopt);
	EXPECT_EQ(short_text, std::nullopt);
	EXPECT_EQ(long_text, std::nullopt);
	EXPECT_EQ(braced_text, std::nullopt);
	EXPECT_EQ(urn_text, std::nullopt);
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
	const auto required_hyphen_positions = std::array<std::size_t, 4U>{{8U, 13U, 18U, 23U}};

	for (const auto position : required_hyphen_positions)
	{
		const auto trace = std::string("required hyphen index ") + std::to_string(position);
		SCOPED_TRACE(trace);
		std::string missing_required_hyphen = "00112233-4455-6677-8899-aabbccddeeff";
		missing_required_hyphen[position] = '0';

		const auto missing_required = ket::uuid::Parse(missing_required_hyphen);

		EXPECT_EQ(missing_required, std::nullopt);
	}

	for (std::size_t position = 0U; position < 36U; ++position)
	{
		const auto position_requires_hyphen = IsRequiredHyphenPosition(position);
		if (position_requires_hyphen)
		{
			continue;
		}

		const auto trace = std::string("unexpected hyphen index ") + std::to_string(position);
		SCOPED_TRACE(trace);
		std::string extra_hyphen = "00112233-4455-6677-8899-aabbccddeeff";
		extra_hyphen[position] = '-';

		const auto unexpected_hyphen = ket::uuid::Parse(extra_hyphen);

		EXPECT_EQ(unexpected_hyphen, std::nullopt);
	}
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
	for (std::size_t position = 0U; position < 36U; ++position)
	{
		const auto position_requires_hyphen = IsRequiredHyphenPosition(position);
		if (position_requires_hyphen)
		{
			continue;
		}

		const auto trace = std::string("invalid hex index ") + std::to_string(position);
		SCOPED_TRACE(trace);
		std::string invalid_text = "00112233-4455-6677-8899-aabbccddeeff";
		invalid_text[position] = 'g';

		const auto parsed = ket::uuid::Parse(invalid_text);

		EXPECT_EQ(parsed, std::nullopt);
	}
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

/**
 * @test
 * @brief all-zero UUID の文字列化確認。
 * @details 全byteが0x00のUUID値を入力し、全桁0のcanonical hyphen形式へformatされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, FormatsZeroUuid)
{
	const auto value = ket::uuid::Uuid{};

	const auto text = ket::uuid::Format(value);

	EXPECT_EQ(text, std::string("00000000-0000-0000-0000-000000000000"));
}

/**
 * @test
 * @brief all-ff UUID の文字列化確認。
 * @details 全byteが0xffのUUID値を入力し、全桁fのcanonical hyphen形式へformatされることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, FormatsAllFfUuid)
{
	const auto value = ket::uuid::Uuid{{0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU,
										0xffU}};

	const auto text = ket::uuid::Format(value);

	EXPECT_EQ(text, std::string("ffffffff-ffff-ffff-ffff-ffffffffffff"));
}

/**
 * @test
 * @brief upper-case入力のcanonical文字列化確認。
 * @details upper-case
 * UUIDをparseしてからformatし、lower-case固定のcanonical形式へ正規化されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, FormatsParsedUppercaseAsLowercaseCanonical)
{
	const auto parsed = ket::uuid::Parse("ABCDEFAB-CDEF-ABCD-EFAB-CDEFABCDEFAB");
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
	ExpectOptionalUuidBytes(parsed, expected);
	const auto parsed_has_value = parsed != std::nullopt;
	ASSERT_TRUE(parsed_has_value);

	const auto parsed_value = parsed.value_or(ket::uuid::Uuid{});
	const auto text = ket::uuid::Format(parsed_value);

	EXPECT_EQ(text, std::string("abcdefab-cdef-abcd-efab-cdefabcdefab"));
}

/**
 * @test
 * @brief Format Doxygen例の確認。
 * @details Doxygen例と同じUUID値を入力し、canonical hyphen形式の文字列を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, FormatsDocumentedExample)
{
	const auto value = ket::uuid::Uuid{{0x00U,
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

	const auto text = ket::uuid::Format(value);

	EXPECT_EQ(text, std::string("00112233-4455-6677-8899-aabbccddeeff"));
}

/**
 * @test
 * @brief format結果のparse roundtrip確認。
 * @details UUID値をformatしてからparseし、元の16 byte表現へ戻ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetUuidTest, ParsesFormattedUuidRoundtrip)
{
	const auto value = ket::uuid::Uuid{{0x10U,
										0x32U,
										0x54U,
										0x76U,
										0x98U,
										0xbaU,
										0xdcU,
										0xfeU,
										0x01U,
										0x23U,
										0x45U,
										0x67U,
										0x89U,
										0xabU,
										0xcdU,
										0xefU}};
	const auto expected = std::array<std::uint8_t, kUuidByteCount>{{0x10U,
																	0x32U,
																	0x54U,
																	0x76U,
																	0x98U,
																	0xbaU,
																	0xdcU,
																	0xfeU,
																	0x01U,
																	0x23U,
																	0x45U,
																	0x67U,
																	0x89U,
																	0xabU,
																	0xcdU,
																	0xefU}};

	const auto text = ket::uuid::Format(value);
	const auto reparsed = ket::uuid::Parse(text);
	ExpectOptionalUuidBytes(reparsed, expected);
}
