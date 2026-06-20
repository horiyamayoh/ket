#include "ket_tlv.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	constexpr std::size_t kHeaderSize = 6U;

	const std::uint8_t kSentinelValue[] = {std::uint8_t{0xEEU}};

	ket::tlv::DecodeResult MakeSentinelResult() noexcept
	{
		return ket::tlv::DecodeResult{{std::uint16_t{0xBEEFU}, kSentinelValue, 1U}, 99U};
	}

	void ExpectSentinelResult(const ket::tlv::DecodeResult& value)
	{
		const auto trace = std::string("sentinel DecodeResult");
		SCOPED_TRACE(trace);

		EXPECT_EQ(value.view.type, std::uint16_t{0xBEEFU});
		EXPECT_EQ(value.view.value, kSentinelValue);
		EXPECT_EQ(value.view.value_size, 1U);
		EXPECT_EQ(value.consumed, 99U);
	}

} // namespace

/**
 * @test
 * @brief decode resultのdefault初期値確認。
 * @details ViewとDecodeResultをvalue-initializeし、安全な空状態になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTlvTest, DecodeResultValueInitializesEmpty)
{
	const ket::tlv::DecodeResult decoded{};

	EXPECT_EQ(decoded.view.type, std::uint16_t{0U});
	EXPECT_EQ(decoded.view.value, nullptr);
	EXPECT_EQ(decoded.view.value_size, 0U);
	EXPECT_EQ(decoded.consumed, 0U);
}

/**
 * @test
 * @brief 空valueのencode確認。
 * @details nullptrとsize 0を入力し、6 byte headerだけのTLV recordを生成することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTlvTest, EncodesEmptyValue)
{
	const auto record = ket::tlv::Encode(0x1234U, nullptr, 0U);
	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x12U},
		std::uint8_t{0x34U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
	};

	EXPECT_EQ(record, expected);
}

/**
 * @test
 * @brief 1 byte valueのencode確認。
 * @details 1 byte valueを入力し、big-endian headerとvalue byteが順に格納されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTlvTest, EncodesOneByteValue)
{
	const auto value = std::array<std::uint8_t, 1>{{std::uint8_t{0xAAU}}};

	const auto record = ket::tlv::Encode(0x1234U, value.data(), 1U);
	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x12U},
		std::uint8_t{0x34U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x01U},
		std::uint8_t{0xAAU},
	};

	EXPECT_EQ(record, expected);
}

/**
 * @test
 * @brief big-endian golden bytesのencode確認。
 * @details 複数byte valueを入力し、typeとlengthがbig-endianで固定されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTlvTest, EncodesBigEndianGoldenBytes)
{
	const auto value = std::array<std::uint8_t, 3>{
		{std::uint8_t{0xAAU}, std::uint8_t{0xBBU}, std::uint8_t{0xCCU}}};

	const auto record = ket::tlv::Encode(0x1234U, value.data(), 3U);
	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x12U},
		std::uint8_t{0x34U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x03U},
		std::uint8_t{0xAAU},
		std::uint8_t{0xBBU},
		std::uint8_t{0xCCU},
	};

	EXPECT_EQ(record, expected);
}

/**
 * @test
 * @brief 256 byte valueのlength encode/decode確認。
 * @details 256 byte valueを入力し、lengthが00 00 01
 * 00として格納されdecode結果と一致することを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, EncodesAndDecodesLength256)
{
	std::vector<std::uint8_t> value(256U, std::uint8_t{0x5AU});
	const auto record = ket::tlv::Encode(0x0102U, value.data(), 256U);
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(record.data(), record.size(), decoded);

	ASSERT_TRUE(succeeded);
	ASSERT_EQ(record.size(), kHeaderSize + 256U);
	EXPECT_EQ(record[2], std::uint8_t{0x00U});
	EXPECT_EQ(record[3], std::uint8_t{0x00U});
	EXPECT_EQ(record[4], std::uint8_t{0x01U});
	EXPECT_EQ(record[5], std::uint8_t{0x00U});
	EXPECT_EQ(decoded.view.value, record.data() + kHeaderSize);
	EXPECT_EQ(decoded.view.value_size, 256U);
	EXPECT_EQ(decoded.consumed, kHeaderSize + 256U);
	EXPECT_EQ(decoded.view.value[255], std::uint8_t{0x5AU});
}

/**
 * @test
 * @brief type境界値のencode/decode確認。
 * @details type 0x0000と0xFFFFをencode/decodeし、16 bit境界値を保持することを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, EncodesAndDecodesTypeBoundaries)
{
	const auto zero_type_record = ket::tlv::Encode(0x0000U, nullptr, 0U);
	const auto max_type_record = ket::tlv::Encode(0xFFFFU, nullptr, 0U);
	auto zero_decoded = MakeSentinelResult();
	auto max_decoded = MakeSentinelResult();

	const auto zero_succeeded =
		ket::tlv::TryDecode(zero_type_record.data(), zero_type_record.size(), zero_decoded);
	const auto max_succeeded =
		ket::tlv::TryDecode(max_type_record.data(), max_type_record.size(), max_decoded);

	ASSERT_TRUE(zero_succeeded);
	ASSERT_TRUE(max_succeeded);
	EXPECT_EQ(zero_type_record[0], std::uint8_t{0x00U});
	EXPECT_EQ(zero_type_record[1], std::uint8_t{0x00U});
	EXPECT_EQ(max_type_record[0], std::uint8_t{0xFFU});
	EXPECT_EQ(max_type_record[1], std::uint8_t{0xFFU});
	EXPECT_EQ(zero_decoded.view.type, std::uint16_t{0x0000U});
	EXPECT_EQ(max_decoded.view.type, std::uint16_t{0xFFFFU});
}

/**
 * @test
 * @brief 既存byte列へのappend確認。
 * @details prefixを持つvectorへTLV recordを追加し、既存内容を保持して末尾に追加されることを確認。
 * @pre C++17以降。
 * @post destinationは期待するbyte列に変化。destination以外の外部状態の変更なし。
 */
TEST(KetTlvTest, AppendsRecordToExistingBytes)
{
	const auto value = std::array<std::uint8_t, 2>{{std::uint8_t{0xAAU}, std::uint8_t{0xBBU}}};
	std::vector<std::uint8_t> destination{std::uint8_t{0x99U}};

	ket::tlv::Append(destination, 0x0102U, value.data(), 2U);
	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x99U},
		std::uint8_t{0x01U},
		std::uint8_t{0x02U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x02U},
		std::uint8_t{0xAAU},
		std::uint8_t{0xBBU},
	};

	EXPECT_EQ(destination, expected);
}

/**
 * @test
 * @brief Append Doxygen例の確認。
 * @details Doxygen例と同じ入力でAppendし、headerと末尾value byteを確認。
 * @pre C++17以降。
 * @post outputはDoxygen例と同じTLV recordを保持。
 */
TEST(KetTlvTest, AppendsDocumentedExample)
{
	std::vector<std::uint8_t> output;
	const std::uint8_t value[] = {0xAAU};

	ket::tlv::Append(output, 0x1234U, value, 1U);

	ASSERT_EQ(output.size(), 7U);
	EXPECT_EQ(output[0], std::uint8_t{0x12U});
	EXPECT_EQ(output[6], std::uint8_t{0xAAU});
}

/**
 * @test
 * @brief dst内部storage由来valueのappend確認。
 * @details
 * valueがdst内部storageを指す場合でも、元valueを一時recordへ退避してから追加することを確認。
 * @pre C++17以降。
 * @post destinationは元byte列と、その元byte列をvalueに持つTLV recordを保持。
 */
TEST(KetTlvTest, AppendsValueAliasedWithDestinationStorage)
{
	std::vector<std::uint8_t> destination{std::uint8_t{0xAAU}, std::uint8_t{0xBBU}};
	const std::uint8_t* const source = destination.data();
	const auto source_size = static_cast<std::uint32_t>(destination.size());

	ket::tlv::Append(destination, 0x0102U, source, source_size);

	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0xAAU},
		std::uint8_t{0xBBU},
		std::uint8_t{0x01U},
		std::uint8_t{0x02U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x00U},
		std::uint8_t{0x02U},
		std::uint8_t{0xAAU},
		std::uint8_t{0xBBU},
	};

	EXPECT_EQ(destination, expected);
}

/**
 * @test
 * @brief encode/decode roundtrip確認。
 * @details encode結果をdecodeし、type、value view、value size、consumedが一致することを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, DecodesEncodedRecord)
{
	const auto value = std::array<std::uint8_t, 2>{{std::uint8_t{0x10U}, std::uint8_t{0x20U}}};
	const auto record = ket::tlv::Encode(0xCAFEU, value.data(), 2U);
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(record.data(), record.size(), decoded);

	ASSERT_TRUE(succeeded);
	EXPECT_EQ(decoded.view.type, std::uint16_t{0xCAFEU});
	EXPECT_EQ(decoded.view.value, record.data() + kHeaderSize);
	EXPECT_EQ(decoded.view.value_size, 2U);
	EXPECT_EQ(decoded.consumed, 8U);
	EXPECT_EQ(decoded.view.value[0], std::uint8_t{0x10U});
	EXPECT_EQ(decoded.view.value[1], std::uint8_t{0x20U});
}

/**
 * @test
 * @brief 複数record入力の先頭decode確認。
 * @details 2 recordを連結した入力を渡し、先頭recordだけをdecodeしてconsumedで境界を返すことを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, DecodesOnlyFirstRecord)
{
	const auto first_value = std::array<std::uint8_t, 1>{{std::uint8_t{0xA1U}}};
	const auto second_value = std::array<std::uint8_t, 1>{{std::uint8_t{0xB2U}}};
	std::vector<std::uint8_t> records;
	ket::tlv::Append(records, 0x1001U, first_value.data(), 1U);
	ket::tlv::Append(records, 0x1002U, second_value.data(), 1U);
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(records.data(), records.size(), decoded);

	ASSERT_TRUE(succeeded);
	EXPECT_EQ(decoded.view.type, std::uint16_t{0x1001U});
	EXPECT_EQ(decoded.view.value_size, 1U);
	EXPECT_EQ(decoded.view.value[0], std::uint8_t{0xA1U});
	EXPECT_EQ(decoded.consumed, 7U);
}

/**
 * @test
 * @brief nullptrかつsize 0のdecode失敗確認。
 * @details nullptrとsize 0を入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsNullEmptyDecodeInput)
{
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(nullptr, 0U, decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief nullptrかつ非0 sizeのdecode失敗確認。
 * @details nullptrと非0 sizeを入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsNullNonEmptyDecodeInput)
{
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(nullptr, 7U, decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief 非nullかつsize 0のdecode失敗確認。
 * @details 非null pointerとsize 0を入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsNonNullEmptyDecodeInput)
{
	const auto data = std::array<std::uint8_t, 1>{{std::uint8_t{0x12U}}};
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(data.data(), 0U, decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief 6 bytes未満のheader拒否確認。
 * @details 5 bytesの入力を渡し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsShortHeader)
{
	const auto data = std::array<std::uint8_t, 5>{{std::uint8_t{0x12U},
												   std::uint8_t{0x34U},
												   std::uint8_t{0x00U},
												   std::uint8_t{0x00U},
												   std::uint8_t{0x00U}}};
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(data.data(), data.size(), decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief declared lengthに対する短いvalue拒否確認。
 * @details length 2に対してvalue 1 byteだけの入力を渡し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsShortValue)
{
	const auto data = std::array<std::uint8_t, 7>{{std::uint8_t{0x12U},
												   std::uint8_t{0x34U},
												   std::uint8_t{0x00U},
												   std::uint8_t{0x00U},
												   std::uint8_t{0x00U},
												   std::uint8_t{0x02U},
												   std::uint8_t{0xAAU}}};
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(data.data(), data.size(), decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief 過大なdeclared length拒否確認。
 * @details uint32最大lengthを持つheaderだけの入力を渡し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時のsentinel値を保持。外部状態の変更なし。
 */
TEST(KetTlvTest, RejectsMaxUint32LengthHeaderWithoutValue)
{
	const auto data = std::array<std::uint8_t, 6>{{std::uint8_t{0x12U},
												   std::uint8_t{0x34U},
												   std::uint8_t{0xFFU},
												   std::uint8_t{0xFFU},
												   std::uint8_t{0xFFU},
												   std::uint8_t{0xFFU}}};
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(data.data(), data.size(), decoded);

	EXPECT_FALSE(succeeded);
	ExpectSentinelResult(decoded);
}

/**
 * @test
 * @brief 空value recordのdecode確認。
 * @details length 0のrecordをdecodeし、value_size 0とheader分のconsumedを返すことを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, DecodesEmptyValue)
{
	const auto record = ket::tlv::Encode(0x1234U, nullptr, 0U);
	auto decoded = MakeSentinelResult();

	const auto succeeded = ket::tlv::TryDecode(record.data(), record.size(), decoded);

	ASSERT_TRUE(succeeded);
	EXPECT_EQ(decoded.view.type, std::uint16_t{0x1234U});
	EXPECT_EQ(decoded.view.value, record.data() + kHeaderSize);
	EXPECT_EQ(decoded.view.value_size, 0U);
	EXPECT_EQ(decoded.consumed, kHeaderSize);
}

/**
 * @test
 * @brief TryDecode Doxygen例の確認。
 * @details Doxygen例と同じwire bytesをdecodeし、type、value size、value byteを確認。
 * @pre C++17以降。
 * @post decode結果は入力bufferへのnon-owning viewを保持。入力buffer自体の変更なし。
 */
TEST(KetTlvTest, DecodesDocumentedWireExample)
{
	const std::uint8_t record[] = {0x12U, 0x34U, 0x00U, 0x00U, 0x00U, 0x01U, 0xAAU};
	ket::tlv::DecodeResult result{};

	const auto ok = ket::tlv::TryDecode(record, sizeof(record), result);

	ASSERT_TRUE(ok);
	EXPECT_EQ(result.view.type, std::uint16_t{0x1234U});
	EXPECT_EQ(result.view.value_size, 1U);
	EXPECT_EQ(result.view.value[0], std::uint8_t{0xAAU});
	EXPECT_EQ(result.consumed, sizeof(record));
}
