#include "ket_ipv4.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
	constexpr bool AddressEquals(ket::ipv4::Address value,
								 std::uint8_t octet0,
								 std::uint8_t octet1,
								 std::uint8_t octet2,
								 std::uint8_t octet3) noexcept
	{
		return value.octets[0] == octet0 && value.octets[1] == octet1 &&
			value.octets[2] == octet2 && value.octets[3] == octet3;
	}

	bool OptionalAddressEquals(std::optional<ket::ipv4::Address> value,
							   std::uint8_t octet0,
							   std::uint8_t octet1,
							   std::uint8_t octet2,
							   std::uint8_t octet3) noexcept
	{
		const auto value_has_value = value.has_value();
		if (!value_has_value)
		{
			return false;
		}

		return AddressEquals(*value, octet0, octet1, octet2, octet3);
	}

} // namespace

static_assert(AddressEquals(ket::ipv4::Address{}, 0U, 0U, 0U, 0U), "default IPv4 address is zero");
static_assert(ket::ipv4::ToBe32(ket::ipv4::Address{{1U, 2U, 3U, 4U}}) == 0x01020304U,
			  "IPv4 address to BE32 is constexpr");
static_assert(AddressEquals(ket::ipv4::FromBe32(0xC0A80001U), 192U, 168U, 0U, 1U),
			  "BE32 to IPv4 address is constexpr");

/**
 * @test
 * @brief dotted decimalの代表値parse確認。
 * @details 最小値、最大値、通常のprivate addressを入力し、4 octetへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, ParsesDottedDecimalAddresses)
{
	const auto zero = ket::ipv4::Parse("0.0.0.0");
	const auto maximum = ket::ipv4::Parse("255.255.255.255");
	const auto private_address = ket::ipv4::Parse("192.168.0.1");

	const auto zero_matches = OptionalAddressEquals(zero, 0U, 0U, 0U, 0U);
	const auto maximum_matches = OptionalAddressEquals(maximum, 255U, 255U, 255U, 255U);
	const auto private_address_matches = OptionalAddressEquals(private_address, 192U, 168U, 0U, 1U);

	EXPECT_EQ(zero_matches, true);
	EXPECT_EQ(maximum_matches, true);
	EXPECT_EQ(private_address_matches, true);
}

/**
 * @test
 * @brief IPv4 addressのdotted decimal format確認。
 * @details 通常値、最小値、最大値を入力し、leading zeroなしのdotted decimalへ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, FormatsAddressesAsDottedDecimal)
{
	const auto default_address = ket::ipv4::Address{};
	const auto zero = ket::ipv4::Address{{0U, 0U, 0U, 0U}};
	const auto maximum = ket::ipv4::Address{{255U, 255U, 255U, 255U}};
	const auto private_address = ket::ipv4::Address{{192U, 168U, 0U, 1U}};

	const auto default_text = ket::ipv4::Format(default_address);
	const auto zero_text = ket::ipv4::Format(zero);
	const auto maximum_text = ket::ipv4::Format(maximum);
	const auto private_address_text = ket::ipv4::Format(private_address);

	EXPECT_EQ(default_text, std::string("0.0.0.0"));
	EXPECT_EQ(zero_text, std::string("0.0.0.0"));
	EXPECT_EQ(maximum_text, std::string("255.255.255.255"));
	EXPECT_EQ(private_address_text, std::string("192.168.0.1"));
}

/**
 * @test
 * @brief IPv4 addressとBE 32bit整数の相互変換確認。
 * @details 代表値と境界値を入力し、network byte orderの整数値と4 octetへ相互変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, ConvertsToAndFromBe32)
{
	const auto loopback = ket::ipv4::Address{{127U, 0U, 0U, 1U}};
	const auto ordered = ket::ipv4::Address{{1U, 2U, 3U, 4U}};
	const auto documented = ket::ipv4::Address{{192U, 168U, 0U, 1U}};
	const auto maximum = ket::ipv4::Address{{255U, 255U, 255U, 255U}};
	const auto zero = ket::ipv4::Address{{0U, 0U, 0U, 0U}};

	const auto loopback_value = ket::ipv4::ToBe32(loopback);
	const auto ordered_value = ket::ipv4::ToBe32(ordered);
	const auto documented_value = ket::ipv4::ToBe32(documented);
	const auto maximum_value = ket::ipv4::ToBe32(maximum);
	const auto zero_value = ket::ipv4::ToBe32(zero);
	const auto loopback_roundtrip = ket::ipv4::FromBe32(loopback_value);
	const auto ordered_roundtrip = ket::ipv4::FromBe32(ordered_value);
	const auto documented_roundtrip = ket::ipv4::FromBe32(documented_value);
	const auto maximum_roundtrip = ket::ipv4::FromBe32(maximum_value);
	const auto zero_roundtrip = ket::ipv4::FromBe32(zero_value);

	const auto loopback_matches = AddressEquals(loopback_roundtrip, 127U, 0U, 0U, 1U);
	const auto ordered_matches = AddressEquals(ordered_roundtrip, 1U, 2U, 3U, 4U);
	const auto documented_matches = AddressEquals(documented_roundtrip, 192U, 168U, 0U, 1U);
	const auto maximum_matches = AddressEquals(maximum_roundtrip, 255U, 255U, 255U, 255U);
	const auto zero_matches = AddressEquals(zero_roundtrip, 0U, 0U, 0U, 0U);

	EXPECT_EQ(loopback_value, 0x7F000001U);
	EXPECT_EQ(ordered_value, 0x01020304U);
	EXPECT_EQ(documented_value, 0xC0A80001U);
	EXPECT_EQ(maximum_value, 0xFFFFFFFFU);
	EXPECT_EQ(zero_value, 0x00000000U);
	EXPECT_EQ(loopback_matches, true);
	EXPECT_EQ(ordered_matches, true);
	EXPECT_EQ(documented_matches, true);
	EXPECT_EQ(maximum_matches, true);
	EXPECT_EQ(zero_matches, true);
}

/**
 * @test
 * @brief dotted decimalの不正octet拒否確認。
 * @details 範囲外octet、負値、非数字を含む入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, RejectsInvalidOctetValues)
{
	const auto too_large_first = ket::ipv4::Parse("256.0.0.1");
	const auto too_large_last = ket::ipv4::Parse("1.2.3.999");
	const auto negative = ket::ipv4::Parse("-1.0.0.0");
	const auto alphabet = ket::ipv4::Parse("1.2.a.4");

	EXPECT_EQ(too_large_first, std::nullopt);
	EXPECT_EQ(too_large_last, std::nullopt);
	EXPECT_EQ(negative, std::nullopt);
	EXPECT_EQ(alphabet, std::nullopt);
}

/**
 * @test
 * @brief dotted decimalの要素数と空要素の拒否確認。
 * @details 空入力、要素不足、要素過多、空要素を含む入力を渡し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, RejectsWrongElementCountAndEmptyElements)
{
	const auto empty = ket::ipv4::Parse("");
	const auto empty_first = ket::ipv4::Parse(".1.2.3");
	const auto missing_last = ket::ipv4::Parse("1.2.3");
	const auto too_many = ket::ipv4::Parse("1.2.3.4.5");
	const auto empty_middle = ket::ipv4::Parse("1..2.3");
	const auto empty_last = ket::ipv4::Parse("1.2.3.");

	EXPECT_EQ(empty, std::nullopt);
	EXPECT_EQ(empty_first, std::nullopt);
	EXPECT_EQ(missing_last, std::nullopt);
	EXPECT_EQ(too_many, std::nullopt);
	EXPECT_EQ(empty_middle, std::nullopt);
	EXPECT_EQ(empty_last, std::nullopt);
}

/**
 * @test
 * @brief dotted decimalの空白とleading zero拒否確認。
 * @details 先頭または末尾の空白、octet内空白、制御文字、NUL、
 * 複数桁octetのleading zeroを拒否することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetIpv4Test, RejectsWhitespaceAndLeadingZero)
{
	const auto leading_space = ket::ipv4::Parse(" 1.2.3.4");
	const auto trailing_space = ket::ipv4::Parse("1.2.3.4 ");
	const auto inner_space = ket::ipv4::Parse("1.2. 3.4");
	const auto tab = ket::ipv4::Parse("\t1.2.3.4");
	const auto newline = ket::ipv4::Parse("1.2.3.4\n");
	const auto trailing_nul = ket::ipv4::Parse(std::string_view("1.2.3.4\0", 8U));
	const auto inner_nul = ket::ipv4::Parse(std::string_view("1.2.\0.4", 7U));
	const auto leading_zero_first = ket::ipv4::Parse("01.2.3.4");
	const auto leading_zero_zero = ket::ipv4::Parse("0.00.0.0");

	EXPECT_EQ(leading_space, std::nullopt);
	EXPECT_EQ(trailing_space, std::nullopt);
	EXPECT_EQ(inner_space, std::nullopt);
	EXPECT_EQ(tab, std::nullopt);
	EXPECT_EQ(newline, std::nullopt);
	EXPECT_EQ(trailing_nul, std::nullopt);
	EXPECT_EQ(inner_nul, std::nullopt);
	EXPECT_EQ(leading_zero_first, std::nullopt);
	EXPECT_EQ(leading_zero_zero, std::nullopt);
}
