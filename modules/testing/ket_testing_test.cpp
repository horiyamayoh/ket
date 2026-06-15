#include "ket_testing.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
	void ExpectAssertionSucceeded(const ::testing::AssertionResult& result)
	{
		const auto succeeded = static_cast<bool>(result);

		EXPECT_TRUE(succeeded);
	}

	void ExpectAssertionFailed(const ::testing::AssertionResult& result)
	{
		const auto failed = !static_cast<bool>(result);

		EXPECT_TRUE(failed);
	}

	void ExpectMessageContains(const ::testing::AssertionResult& result,
							   std::string_view expected_part)
	{
		const auto message = std::string(result.message());
		const auto message_contains_part = message.find(expected_part) != std::string::npos;

		EXPECT_TRUE(message_contains_part) << message;
	}

} // namespace

/**
 * @test
 * @brief 同一byte列の比較成功確認。
 * @details 同じ長さと内容のbyte列を渡し、BytesEqualがsuccessを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, BytesEqualAcceptsEqualBytes)
{
	const auto expected = std::array<std::uint8_t, 3>{{0x01U, 0x02U, 0x03U}};
	const auto actual = std::array<std::uint8_t, 3>{{0x01U, 0x02U, 0x03U}};

	const auto result =
		ket::testing::BytesEqual(expected.data(), expected.size(), actual.data(), actual.size());

	ExpectAssertionSucceeded(result);
}

/**
 * @test
 * @brief byte列長さ差のfailure message確認。
 * @details 共通prefixが一致し、期待値だけが長いbyte列を渡す。failure messageに差分offset、
 * expected/actual size、expected/actual hexが含まれることを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, BytesEqualReportsDifferentSize)
{
	const auto expected = std::array<std::uint8_t, 3>{{0x01U, 0x02U, 0x03U}};
	const auto actual = std::array<std::uint8_t, 2>{{0x01U, 0x02U}};

	const auto result =
		ket::testing::BytesEqual(expected.data(), expected.size(), actual.data(), actual.size());

	ExpectAssertionFailed(result);
	ExpectMessageContains(result, "offset 2");
	ExpectMessageContains(result, "expected size 3");
	ExpectMessageContains(result, "actual size 2");
	ExpectMessageContains(result, "expected hex: 010203");
	ExpectMessageContains(result, "actual hex: 0102");
}

/**
 * @test
 * @brief byte列内容差のfailure message確認。
 * @details 同じ長さで1バイトだけ異なるbyte列を渡す。failure messageに差分offset、
 * offset位置のexpected/actual byte、expected/actual hexが含まれることを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, BytesEqualReportsDifferentByte)
{
	const auto expected = std::array<std::uint8_t, 3>{{0x01U, 0x02U, 0x03U}};
	const auto actual = std::array<std::uint8_t, 3>{{0x01U, 0xFFU, 0x03U}};

	const auto result =
		ket::testing::BytesEqual(expected.data(), expected.size(), actual.data(), actual.size());

	ExpectAssertionFailed(result);
	ExpectMessageContains(result, "offset 1");
	ExpectMessageContains(result, "expected 0x02");
	ExpectMessageContains(result, "actual 0xff");
	ExpectMessageContains(result, "expected hex: 010203");
	ExpectMessageContains(result, "actual hex: 01ff03");
}

/**
 * @test
 * @brief nullptrと空byte列の同一視確認。
 * @details expected/actualの両方にnullptr+0を渡し、空byte列としてsuccessを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, BytesEqualAcceptsNullEmptyBytes)
{
	const auto result = ket::testing::BytesEqual(nullptr, 0U, nullptr, 0U);

	ExpectAssertionSucceeded(result);
}

/**
 * @test
 * @brief nullptrと非0サイズの拒否確認。
 * @details expectedまたはactualにnullptr+非0サイズを渡し、どちらもfailureを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, BytesEqualRejectsNullNonEmptyBytes)
{
	const auto value = std::array<std::uint8_t, 1>{{0x01U}};

	const auto invalid_expected = ket::testing::BytesEqual(nullptr, 1U, value.data(), value.size());
	const auto invalid_actual = ket::testing::BytesEqual(value.data(), value.size(), nullptr, 1U);

	ExpectAssertionFailed(invalid_expected);
	ExpectAssertionFailed(invalid_actual);
	ExpectMessageContains(invalid_expected, "expected is nullptr with non-zero size 1");
	ExpectMessageContains(invalid_actual, "actual is nullptr with non-zero size 1");
}

/**
 * @test
 * @brief hex文字列とbyte列の比較成功確認。
 * @details
 * 大文字小文字を混ぜたhex文字列と同じ内容のbyte列を渡し、HexEqualがsuccessを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualAcceptsMatchingHex)
{
	const auto actual = std::array<std::uint8_t, 3>{{0x0AU, 0x1BU, 0xFFU}};

	const auto result = ket::testing::HexEqual("0a1BfF", actual.data(), actual.size());

	ExpectAssertionSucceeded(result);
}

/**
 * @test
 * @brief 空hex文字列と空byte列の比較成功確認。
 * @details 空hex文字列とnullptr+0のactualを渡し、空byte列としてsuccessを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualAcceptsEmptyHexAndNullEmptyActual)
{
	const auto result = ket::testing::HexEqual("", nullptr, 0U);

	ExpectAssertionSucceeded(result);
}

/**
 * @test
 * @brief hex文字列とbyte列の長さ差failure message確認。
 * @details 共通prefixが一致し、hex文字列だけが長い入力を渡す。failure messageに差分offset、
 * expected/actual size、expected/actual hexが含まれることを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualReportsDifferentSize)
{
	const auto actual = std::array<std::uint8_t, 1>{{0x12U}};

	const auto result = ket::testing::HexEqual("1234", actual.data(), actual.size());

	ExpectAssertionFailed(result);
	ExpectMessageContains(result, "offset 1");
	ExpectMessageContains(result, "expected size 2");
	ExpectMessageContains(result, "actual size 1");
	ExpectMessageContains(result, "expected hex: 1234");
	ExpectMessageContains(result, "actual hex: 12");
}

/**
 * @test
 * @brief hex文字列とbyte列の内容差failure message確認。
 * @details 同じ長さで1バイトだけ異なる入力を渡す。failure messageに差分offset、
 * offset位置のexpected/actual byte、expected/actual hexが含まれることを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualReportsDifferentByte)
{
	const auto actual = std::array<std::uint8_t, 2>{{0x12U, 0xFFU}};

	const auto result = ket::testing::HexEqual("1234", actual.data(), actual.size());

	ExpectAssertionFailed(result);
	ExpectMessageContains(result, "offset 1");
	ExpectMessageContains(result, "expected 0x34");
	ExpectMessageContains(result, "actual 0xff");
	ExpectMessageContains(result, "expected hex: 1234");
	ExpectMessageContains(result, "actual hex: 12ff");
}

/**
 * @test
 * @brief HexEqualのnullptr+非0サイズ拒否確認。
 * @details 妥当なhex文字列とnullptr+非0サイズのactualを渡し、failureを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualRejectsNullNonEmptyActual)
{
	const auto result = ket::testing::HexEqual("12", nullptr, 1U);

	ExpectAssertionFailed(result);
	ExpectMessageContains(result, "actual is nullptr with non-zero size 1");
}

/**
 * @test
 * @brief HexEqualの不正hex拒否確認。
 * @details 奇数桁とhex以外の文字を渡し、どちらもfailureを返すことを確認。
 * @pre C++17以降、GoogleTest利用可能。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTestingTest, HexEqualRejectsInvalidHex)
{
	const auto actual = std::array<std::uint8_t, 1>{{0x12U}};

	const auto odd_length = ket::testing::HexEqual("123", actual.data(), actual.size());
	const auto invalid_digit = ket::testing::HexEqual("1g", actual.data(), actual.size());

	ExpectAssertionFailed(odd_length);
	ExpectAssertionFailed(invalid_digit);
	ExpectMessageContains(odd_length, "odd digit count 3");
	ExpectMessageContains(invalid_digit, "invalid expected hex at digit 1");
}
