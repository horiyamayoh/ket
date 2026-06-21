#include "ket_wire.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string> // NOLINT(misc-include-cleaner): IWYU expects gtest string support.
#include <vector>

#include <gtest/gtest.h>

#include "ket_byte_view.h"

// NOLINTBEGIN(bugprone-unchecked-optional-access)

namespace
{
	struct EmptyMessage
	{
		int marker = 7;
	};

	struct Frame
	{
		std::uint16_t command = 0U;
		std::uint16_t count = 0U;
		std::uint8_t mode = 0U;
	};

	struct BcdFrame
	{
		int value = 0;
	};

	struct BitsFrame
	{
		std::uint8_t mode = 0U;
		std::uint8_t flags = 0U;
	};

	struct PayloadFrame
	{
		std::array<std::uint8_t, 2U> copy{};
		ket::byte_view::View view;
	};

	struct RawBcdFrame
	{
		std::array<std::uint8_t, 2U> digits{};
	};

	struct ValidatedFrame
	{
		std::uint8_t length = 0U;
		std::array<std::uint8_t, 2U> payload{};
		std::uint8_t checksum = 0U;
	};

	struct CallbackFrame
	{
		std::uint8_t value = 0U;
	};

	auto MakeFrameSchema()
	{
		return ket::wire::MakeSchema<Frame>("Frame",
											std::array<ket::wire::Field<Frame>, 3U>{
												ket::wire::U16Be<Frame, &Frame::command>("command"),
												ket::wire::U16Le<Frame, &Frame::count>("count"),
												ket::wire::U8<Frame, &Frame::mode>("mode")});
	}

	auto MakeFrameSchemaFromLocalFields()
	{
		auto fields = std::array<ket::wire::Field<Frame>, 3U>{
			ket::wire::U16Be<Frame, &Frame::command>("command"),
			ket::wire::U16Le<Frame, &Frame::count>("count"),
			ket::wire::U8<Frame, &Frame::mode>("mode")};
		const auto schema = ket::wire::MakeSchema<Frame>("Frame", fields);
		return schema;
	}

	auto MakeBitsSchema()
	{
		return ket::wire::MakeSchema<BitsFrame>(
			"BitsFrame",
			std::array<ket::wire::Field<BitsFrame>, 1U>{ket::wire::BitsU8<BitsFrame>(
				"flags",
				std::array<ket::wire::BitMember<BitsFrame>, 3U>{
					ket::wire::Bit<BitsFrame, &BitsFrame::mode, 0U, 3U>("mode"),
					ket::wire::ReservedBits<BitsFrame, 3U, 1U>("reserved", 0U),
					ket::wire::Bit<BitsFrame, &BitsFrame::flags, 4U, 2U>("flags")})});
	}

	bool LengthMatchesPayload(const ValidatedFrame& value, ket::wire::Status& status) noexcept
	{
		const auto payload_size = value.payload.size();
		const auto matches = value.length == payload_size;
		if (!matches)
		{
			status.error = ket::wire::Error::kLengthMismatch;
			status.field = "length";
			status.required_size = payload_size;
			status.available_size = value.length;
			return false;
		}

		return true;
	}

	bool ChecksumMatchesPayload(const ValidatedFrame& value, ket::wire::Status& status) noexcept
	{
		const auto sum = static_cast<std::uint8_t>(value.payload[0] + value.payload[1]);
		const auto matches = value.checksum == sum;
		if (!matches)
		{
			status.error = ket::wire::Error::kChecksumMismatch;
			status.field = "checksum";
			status.expected = sum;
			status.actual = value.checksum;
			status.has_expected = true;
			status.has_actual = true;
			return false;
		}

		return true;
	}

	// cppcheck-suppress constParameterCallback
	bool AlwaysFails(const CallbackFrame& value, ket::wire::Status& status) noexcept
	{
		(void)value;
		(void)status;
		return false;
	}

} // namespace

/**
 * @test
 * @brief empty schema and view境界確認。
 * @details valid empty input/outputを成功扱いし、nullptr+非0のinput/output viewをdiagnostics付きで
 * 失敗扱いにすることを確認。
 * @pre C++17以降。
 * @post empty schemaはencoded size 0を保持。invalid viewではresult valueやoutputを更新しない。
 */
TEST(KetWireTest, HandlesEmptySchemaAndViewValidity)
{
	const auto schema = ket::wire::MakeSchema<EmptyMessage>(
		"Empty", std::array<ket::wire::Field<EmptyMessage>, 0U>{});

	const auto fixed_size = ket::wire::EncodedSize(schema);
	ASSERT_NE(fixed_size, std::nullopt);
	EXPECT_EQ(fixed_size.value(), 0U);

	const auto decoded = ket::wire::DecodeExact(ket::byte_view::View(nullptr, 0U), schema);
	const auto decoded_ok = decoded.status.Ok();
	ASSERT_NE(decoded.value, std::nullopt);
	EXPECT_TRUE(decoded_ok);
	EXPECT_EQ(decoded.consumed, 0U);

	const auto invalid_input = ket::wire::DecodeExact(ket::byte_view::View(nullptr, 1U), schema);
	const auto invalid_input_has_value = invalid_input.value.has_value();
	EXPECT_FALSE(invalid_input_has_value);
	EXPECT_EQ(invalid_input.status.error, ket::wire::Error::kInvalidInputView);

	const EmptyMessage value{};
	const auto encoded_empty =
		ket::wire::EncodeTo(value, schema, ket::byte_view::MutableView(nullptr, 0U));
	const auto encoded_empty_ok = encoded_empty.status.Ok();
	EXPECT_TRUE(encoded_empty_ok);
	EXPECT_EQ(encoded_empty.encoded_size, 0U);

	const auto invalid_output =
		ket::wire::EncodeTo(value, schema, ket::byte_view::MutableView(nullptr, 1U));
	EXPECT_EQ(invalid_output.status.error, ket::wire::Error::kInvalidOutputView);
	EXPECT_EQ(invalid_output.encoded_size, 0U);
}

/**
 * @test
 * @brief exact decodeとowning encodeのgolden bytes確認。
 * @details mixed endian fieldをschema順にdecode/encodeし、size APIとowning
 * vectorが同じbyte列を返すことを確認。
 * @pre C++17以降。
 * @post input objectとschemaは変更されず、encode resultだけが所有byte列を保持。
 */
TEST(KetWireTest, DecodesExactFrameAndEncodesOwningBytes)
{
	const auto schema = MakeFrameSchema();
	const auto data = std::array<std::uint8_t, 5U>{{0x12U, 0x34U, 0x78U, 0x56U, 0x9AU}};

	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	const auto decoded_ok = decoded.status.Ok();

	ASSERT_NE(decoded.value, std::nullopt);
	const auto& decoded_value = decoded.value.value();
	EXPECT_TRUE(decoded_ok);
	EXPECT_EQ(decoded_value.command, 0x1234U);
	EXPECT_EQ(decoded_value.count, 0x5678U);
	EXPECT_EQ(decoded_value.mode, 0x9AU);
	EXPECT_EQ(decoded.consumed, data.size());

	const auto measured = ket::wire::MeasureEncodedSize(decoded_value, schema);
	const auto measured_ok = measured.status.Ok();
	ASSERT_NE(measured.value, std::nullopt);
	EXPECT_TRUE(measured_ok);
	EXPECT_EQ(measured.value.value(), data.size());

	const auto encoded = ket::wire::Encode(decoded_value, schema);
	const auto encoded_ok = encoded.status.Ok();
	ASSERT_NE(encoded.bytes, std::nullopt);
	EXPECT_TRUE(encoded_ok);
	EXPECT_EQ(encoded.bytes.value(),
			  (std::vector<std::uint8_t>{0x12U, 0x34U, 0x78U, 0x56U, 0x9AU}));
}

/**
 * @test
 * @brief prefix decodeとtrailing bytes diagnostics確認。
 * @details exact decodeはtrailing bytesを拒否し、prefix
 * decodeは同じ入力からschema分だけ消費することを確認。
 * @pre C++17以降。
 * @post exact失敗resultはvalueなし、prefix成功resultはconsumedへ5を保持。
 */
TEST(KetWireTest, DistinguishesExactAndPrefixDecode)
{
	const auto schema = MakeFrameSchema();
	const auto data =
		std::array<std::uint8_t, 7U>{{0x12U, 0x34U, 0x78U, 0x56U, 0x9AU, 0xEEU, 0xFFU}};

	const auto exact =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	const auto exact_has_value = exact.value.has_value();
	EXPECT_FALSE(exact_has_value);
	EXPECT_EQ(exact.status.error, ket::wire::Error::kTrailingBytes);
	EXPECT_EQ(exact.status.offset, 5U);
	EXPECT_EQ(exact.consumed, 0U);

	const auto prefix =
		ket::wire::DecodePrefix(ket::byte_view::View(data.data(), data.size()), schema);
	const auto prefix_ok = prefix.status.Ok();
	ASSERT_NE(prefix.value, std::nullopt);
	const auto& prefix_value = prefix.value.value();
	EXPECT_TRUE(prefix_ok);
	EXPECT_EQ(prefix_value.command, 0x1234U);
	EXPECT_EQ(prefix.consumed, 5U);
}

/**
 * @test
 * @brief schema field array ownership確認。
 * @details local field arrayから作ったschemaを戻した後も、schema内のfield
 * copyだけでdecodeできることを確認。
 * @pre C++17以降。
 * @post caller-held field array lifetimeに依存せずdecode成功。
 */
TEST(KetWireTest, OwnsFieldArrayInsideSchema)
{
	const auto schema = MakeFrameSchemaFromLocalFields();
	const auto data = std::array<std::uint8_t, 5U>{{0x00U, 0x01U, 0x03U, 0x02U, 0x04U}};

	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	ASSERT_NE(decoded.value, std::nullopt);
	const auto& decoded_value = decoded.value.value();
	EXPECT_EQ(schema.fields[0].name, "command");
	EXPECT_EQ(decoded_value.command, 1U);
	EXPECT_EQ(decoded_value.count, 0x0203U);
	EXPECT_EQ(decoded_value.mode, 4U);
}

/**
 * @test
 * @brief short input diagnostics確認。
 * @details 先頭field不足と後続field不足を別々に発生させ、field名、offset、available sizeを確認。
 * @pre C++17以降。
 * @post short input失敗時はvalueなし、consumedは0。
 */
TEST(KetWireTest, ReportsShortInputAtFieldOffset)
{
	const auto schema = MakeFrameSchema();

	const auto first_short = ket::wire::DecodeExact(ket::byte_view::View(nullptr, 0U), schema);
	const auto first_short_has_value = first_short.value.has_value();
	EXPECT_FALSE(first_short_has_value);
	EXPECT_EQ(first_short.status.error, ket::wire::Error::kShortInput);
	EXPECT_EQ(first_short.status.field, "command");
	EXPECT_EQ(first_short.status.offset, 0U);
	EXPECT_EQ(first_short.status.available_size, 0U);

	const auto data = std::array<std::uint8_t, 3U>{{0x12U, 0x34U, 0x56U}};
	const auto second_short =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	const auto second_short_has_value = second_short.value.has_value();
	EXPECT_FALSE(second_short_has_value);
	EXPECT_EQ(second_short.status.error, ket::wire::Error::kShortInput);
	EXPECT_EQ(second_short.status.field, "count");
	EXPECT_EQ(second_short.status.offset, 2U);
	EXPECT_EQ(second_short.status.available_size, 1U);
	EXPECT_EQ(second_short.consumed, 0U);
}

/**
 * @test
 * @brief fixed-buffer encodeの不変性確認。
 * @details output不足ではbufferを変更せず、extra
 * capacityがある成功時はencoded範囲だけ更新することを確認。
 * @pre C++17以降。
 * @post short outputではencoded_size 0、成功時はextra capacity sentinelを保持。
 */
TEST(KetWireTest, EncodesToFixedBufferWithoutTouchingExtraCapacity)
{
	const auto schema = MakeFrameSchema();
	const Frame value{0x1234U, 0x5678U, 0x9AU};

	auto short_output = std::array<std::uint8_t, 4U>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}};
	const auto short_result = ket::wire::EncodeTo(
		value, schema, ket::byte_view::MutableView(short_output.data(), short_output.size()));
	EXPECT_EQ(short_result.status.error, ket::wire::Error::kShortOutput);
	EXPECT_EQ(short_result.encoded_size, 0U);
	EXPECT_EQ(short_output, (std::array<std::uint8_t, 4U>{{0xAAU, 0xAAU, 0xAAU, 0xAAU}}));

	auto output = std::array<std::uint8_t, 7U>{{0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xEEU, 0xFFU}};
	const auto result = ket::wire::EncodeTo(
		value, schema, ket::byte_view::MutableView(output.data(), output.size()));
	const auto result_ok = result.status.Ok();
	EXPECT_TRUE(result_ok);
	EXPECT_EQ(result.encoded_size, 5U);
	EXPECT_EQ(output,
			  (std::array<std::uint8_t, 7U>{{0x12U, 0x34U, 0x78U, 0x56U, 0x9AU, 0xEEU, 0xFFU}}));
}

/**
 * @test
 * @brief BCD fieldのdecode/encode境界確認。
 * @details valid packed BCDはintegerへdecodeし、invalid
 * nibbleと桁数超過はStatus付きで失敗することを確認。
 * @pre C++17以降。
 * @post BCD失敗resultはvalue/bytesを持たない。
 */
TEST(KetWireTest, HandlesBcdIntegerFields)
{
	const auto schema =
		ket::wire::MakeSchema<BcdFrame>("BcdFrame",
										std::array<ket::wire::Field<BcdFrame>, 1U>{
											ket::wire::BcdU8<BcdFrame, &BcdFrame::value>("value")});

	const auto good_data = std::array<std::uint8_t, 1U>{{0x42U}};
	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(good_data.data(), good_data.size()), schema);
	ASSERT_NE(decoded.value, std::nullopt);
	const auto& decoded_value = decoded.value.value();
	EXPECT_EQ(decoded_value.value, 42);

	const auto bad_data = std::array<std::uint8_t, 1U>{{0x1AU}};
	const auto invalid =
		ket::wire::DecodeExact(ket::byte_view::View(bad_data.data(), bad_data.size()), schema);
	const auto invalid_has_value = invalid.value.has_value();
	EXPECT_FALSE(invalid_has_value);
	EXPECT_EQ(invalid.status.error, ket::wire::Error::kInvalidBcd);
	EXPECT_EQ(invalid.status.actual, 0x0AU);

	const BcdFrame too_large{100};
	const auto encoded = ket::wire::Encode(too_large, schema);
	const auto encoded_has_bytes = encoded.bytes.has_value();
	EXPECT_FALSE(encoded_has_bytes);
	EXPECT_EQ(encoded.status.error, ket::wire::Error::kValueOutOfRange);
}

/**
 * @test
 * @brief grouped bit field roundtrip確認。
 * @details 1byte storageをlogical fieldsへ展開し、reserved bitを検査し、range
 * overflow時にoutputを保つことを確認。
 * @pre C++17以降。
 * @post bit groupはdecode/encodeでstorage unitを1回だけ消費。
 */
TEST(KetWireTest, HandlesGroupedBitsAndReservedBits)
{
	const auto schema = MakeBitsSchema();
	const auto data = std::array<std::uint8_t, 1U>{{0x25U}};

	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	ASSERT_NE(decoded.value, std::nullopt);
	const auto& decoded_value = decoded.value.value();
	EXPECT_EQ(decoded_value.mode, 5U);
	EXPECT_EQ(decoded_value.flags, 2U);

	const auto encoded = ket::wire::Encode(decoded_value, schema);
	ASSERT_NE(encoded.bytes, std::nullopt);
	EXPECT_EQ(encoded.bytes.value(), (std::vector<std::uint8_t>{0x25U}));

	const auto reserved_bad = std::array<std::uint8_t, 1U>{{0x08U}};
	const auto reserved_result = ket::wire::DecodeExact(
		ket::byte_view::View(reserved_bad.data(), reserved_bad.size()), schema);
	const auto reserved_has_value = reserved_result.value.has_value();
	EXPECT_FALSE(reserved_has_value);
	EXPECT_EQ(reserved_result.status.error, ket::wire::Error::kReservedMismatch);
	EXPECT_EQ(reserved_result.status.field, "reserved");
	EXPECT_EQ(reserved_result.status.group, "flags");

	const BitsFrame overflow{5U, 4U};
	auto output = std::array<std::uint8_t, 1U>{{0xAAU}};
	const auto overflow_result = ket::wire::EncodeTo(
		overflow, schema, ket::byte_view::MutableView(output.data(), output.size()));
	EXPECT_EQ(overflow_result.status.error, ket::wire::Error::kValueOutOfRange);
	EXPECT_EQ(output[0], 0xAAU);
}

/**
 * @test
 * @brief fixed byte copyとview field lifetime contract確認。
 * @details copy fieldはsource pointerを保持せず、view fieldだけがdecode元buffer
 * pointerを保持することを確認。
 * @pre C++17以降。
 * @post copy memberはinput変更後も元値を保持し、view memberは意図通り元bufferを参照。
 */
TEST(KetWireTest, CopiesFixedBytesAndPreservesViewPointerIntentionally)
{
	const auto schema = ket::wire::MakeSchema<PayloadFrame>(
		"PayloadFrame",
		std::array<ket::wire::Field<PayloadFrame>, 2U>{
			ket::wire::Bytes<PayloadFrame, &PayloadFrame::copy>("copy"),
			ket::wire::ViewBytes<PayloadFrame, &PayloadFrame::view>("view", 2U)});

	auto data = std::array<std::uint8_t, 4U>{{0xAAU, 0xBBU, 0xCCU, 0xDDU}};
	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	ASSERT_NE(decoded.value, std::nullopt);

	const auto value = decoded.value.value();
	data[0] = 0x11U;
	data[2] = 0x22U;

	EXPECT_EQ(value.copy[0], 0xAAU);
	EXPECT_EQ(value.copy[1], 0xBBU);
	EXPECT_EQ(value.view.Data(), data.data() + 2U);
	EXPECT_EQ(value.view.Data()[0], 0x22U);

	const auto encoded = ket::wire::Encode(value, schema);
	ASSERT_NE(encoded.bytes, std::nullopt);
	EXPECT_EQ(encoded.bytes.value(), (std::vector<std::uint8_t>{0xAAU, 0xBBU, 0x22U, 0xDDU}));
}

/**
 * @test
 * @brief raw BCD bytes descriptor確認。
 * @details leading zeroを持つraw BCD bytesをcopyし、invalid nibbleではdecoded
 * objectを作らないことを確認。
 * @pre C++17以降。
 * @post raw descriptorはinteger化せずbyte列を保持。
 */
TEST(KetWireTest, HandlesRawBcdBytesWithoutDroppingLeadingZero)
{
	const auto schema = ket::wire::MakeSchema<RawBcdFrame>(
		"RawBcdFrame",
		std::array<ket::wire::Field<RawBcdFrame>, 1U>{
			ket::wire::RawBcdBytes<RawBcdFrame, &RawBcdFrame::digits>("digits")});

	const auto data = std::array<std::uint8_t, 2U>{{0x00U, 0x42U}};
	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(data.data(), data.size()), schema);
	ASSERT_NE(decoded.value, std::nullopt);
	const auto& decoded_value = decoded.value.value();
	EXPECT_EQ(decoded_value.digits, (std::array<std::uint8_t, 2U>{{0x00U, 0x42U}}));

	const auto invalid_data = std::array<std::uint8_t, 2U>{{0x00U, 0x4AU}};
	const auto invalid = ket::wire::DecodeExact(
		ket::byte_view::View(invalid_data.data(), invalid_data.size()), schema);
	const auto invalid_has_value = invalid.value.has_value();
	EXPECT_FALSE(invalid_has_value);
	EXPECT_EQ(invalid.status.error, ket::wire::Error::kInvalidBcd);
}

/**
 * @test
 * @brief reserved bytes diagnostics確認。
 * @details decodeはreserved byte mismatchをoffset、expected、actual付きで返し、encodeはreserved
 * bytesを書き込むことを確認。
 * @pre C++17以降。
 * @post mismatch時はvalueなし、encode成功時はexpected byte列を生成。
 */
TEST(KetWireTest, HandlesReservedBytes)
{
	const auto schema = ket::wire::MakeSchema<EmptyMessage>(
		"ReservedFrame",
		std::array<ket::wire::Field<EmptyMessage>, 1U>{
			ket::wire::ReservedBytes<EmptyMessage>("reserved", 2U, 0U)});

	const auto bad_data = std::array<std::uint8_t, 2U>{{0x00U, 0x01U}};
	const auto decoded =
		ket::wire::DecodeExact(ket::byte_view::View(bad_data.data(), bad_data.size()), schema);
	const auto decoded_has_value = decoded.value.has_value();
	EXPECT_FALSE(decoded_has_value);
	EXPECT_EQ(decoded.status.error, ket::wire::Error::kReservedMismatch);
	EXPECT_EQ(decoded.status.offset, 1U);
	EXPECT_EQ(decoded.status.expected, 0U);
	EXPECT_EQ(decoded.status.actual, 1U);

	const auto encoded = ket::wire::Encode(EmptyMessage{}, schema);
	ASSERT_NE(encoded.bytes, std::nullopt);
	EXPECT_EQ(encoded.bytes.value(), (std::vector<std::uint8_t>{0x00U, 0x00U}));
}

/**
 * @test
 * @brief validation hook diagnostics確認。
 * @details length、checksum、default callback
 * failureをoperation固有resultのStatusで返すことを確認。
 * @pre C++17以降。
 * @post callback失敗時はvalue/bytesなし、Statusにhookが設定したdiagnosticsを保持。
 */
TEST(KetWireTest, RunsValidationHooksForLengthChecksumAndCallbackFailure)
{
	const auto validation_schema = ket::wire::MakeSchema<ValidatedFrame>(
		"ValidatedFrame",
		std::array<ket::wire::Field<ValidatedFrame>, 5U>{
			ket::wire::U8<ValidatedFrame, &ValidatedFrame::length>("length"),
			ket::wire::Bytes<ValidatedFrame, &ValidatedFrame::payload>("payload"),
			ket::wire::U8<ValidatedFrame, &ValidatedFrame::checksum>("checksum"),
			ket::wire::LengthValidation<ValidatedFrame>("length_check", &LengthMatchesPayload),
			ket::wire::ChecksumValidation<ValidatedFrame>("checksum_check",
														  &ChecksumMatchesPayload)});

	const auto length_bad = std::array<std::uint8_t, 4U>{{0x03U, 0x01U, 0x02U, 0x03U}};
	const auto length_result = ket::wire::DecodeExact(
		ket::byte_view::View(length_bad.data(), length_bad.size()), validation_schema);
	const auto length_has_value = length_result.value.has_value();
	EXPECT_FALSE(length_has_value);
	EXPECT_EQ(length_result.status.error, ket::wire::Error::kLengthMismatch);
	EXPECT_EQ(length_result.status.field, "length");

	const auto checksum_bad = std::array<std::uint8_t, 4U>{{0x02U, 0x01U, 0x02U, 0x04U}};
	const auto checksum_result = ket::wire::DecodeExact(
		ket::byte_view::View(checksum_bad.data(), checksum_bad.size()), validation_schema);
	const auto checksum_has_value = checksum_result.value.has_value();
	EXPECT_FALSE(checksum_has_value);
	EXPECT_EQ(checksum_result.status.error, ket::wire::Error::kChecksumMismatch);
	EXPECT_EQ(checksum_result.status.expected, 3U);
	EXPECT_EQ(checksum_result.status.actual, 4U);

	const auto callback_schema = ket::wire::MakeSchema<CallbackFrame>(
		"CallbackFrame",
		std::array<ket::wire::Field<CallbackFrame>, 2U>{
			ket::wire::U8<CallbackFrame, &CallbackFrame::value>("value"),
			ket::wire::Validate<CallbackFrame>("always_fails", &AlwaysFails)});
	const auto callback_data = std::array<std::uint8_t, 1U>{{0x01U}};
	const auto callback_result = ket::wire::DecodeExact(
		ket::byte_view::View(callback_data.data(), callback_data.size()), callback_schema);
	const auto callback_has_value = callback_result.value.has_value();
	EXPECT_FALSE(callback_has_value);
	EXPECT_EQ(callback_result.status.error, ket::wire::Error::kCallbackFailed);
	EXPECT_EQ(callback_result.status.field, "always_fails");
}

/**
 * @test
 * @brief size overflow diagnostics確認。
 * @details schema construction時のsize_t
 * overflowをEncodedSizeとMeasureEncodedSizeの失敗値として保持することを確認。
 * @pre C++17以降。
 * @post overflow schemaはsize resultを持たず、Status::errorにkSizeOverflowを保持。
 */
TEST(KetWireTest, ReportsSizeOverflow)
{
	const auto schema = ket::wire::MakeSchema<EmptyMessage>(
		"Huge",
		std::array<ket::wire::Field<EmptyMessage>, 2U>{
			ket::wire::ReservedBytes<EmptyMessage>(
				"huge", std::numeric_limits<std::size_t>::max(), 0U),
			ket::wire::ConstU8<EmptyMessage>("one", 0U)});

	const auto schema_ok = schema.status.Ok();
	EXPECT_FALSE(schema_ok);
	EXPECT_EQ(schema.status.error, ket::wire::Error::kSizeOverflow);

	const auto fixed_size = ket::wire::EncodedSize(schema);
	const auto fixed_size_has_value = fixed_size.has_value();
	EXPECT_FALSE(fixed_size_has_value);

	const auto measured = ket::wire::MeasureEncodedSize(EmptyMessage{}, schema);
	const auto measured_has_value = measured.value.has_value();
	EXPECT_FALSE(measured_has_value);
	EXPECT_EQ(measured.status.error, ket::wire::Error::kSizeOverflow);
}

// NOLINTEND(bugprone-unchecked-optional-access)
