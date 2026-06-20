#include "ket_testing.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
	constexpr char kHexDigits[] = "0123456789abcdef";

	constexpr bool HasInvalidBytePointer(const std::uint8_t* data, std::size_t size) noexcept
	{
		return data == nullptr && size != 0U;
	}

	constexpr char HexDigitChar(std::uint8_t value) noexcept
	{
		return kHexDigits[value & 0x0FU];
	}

	constexpr bool IsAsciiWhitespace(char value) noexcept
	{
		return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' ||
			value == '\v';
	}

	constexpr int HexDigitValue(char value) noexcept
	{
		const auto is_decimal = value >= '0' && value <= '9';
		if (is_decimal)
		{
			return value - '0';
		}

		const auto is_lower_hex = value >= 'a' && value <= 'f';
		if (is_lower_hex)
		{
			return 10 + (value - 'a');
		}

		const auto is_upper_hex = value >= 'A' && value <= 'F';
		if (is_upper_hex)
		{
			return 10 + (value - 'A');
		}

		return -1;
	}

	std::size_t CountHexDigits(std::string_view hex) noexcept
	{
		auto result = std::size_t{0U};

		for (const auto digit : hex)
		{
			const auto digit_is_whitespace = IsAsciiWhitespace(digit);
			if (digit_is_whitespace)
			{
				continue;
			}

			++result;
		}

		return result;
	}

	void AppendByteHex(std::string& output, std::uint8_t value)
	{
		const auto unsigned_value = static_cast<unsigned int>(value);
		const auto high = static_cast<std::uint8_t>((unsigned_value >> 4U) & 0x0FU);
		const auto low = static_cast<std::uint8_t>(unsigned_value & 0x0FU);
		output.push_back(HexDigitChar(high));
		output.push_back(HexDigitChar(low));
	}

	std::string ByteHex(std::uint8_t value)
	{
		std::string result;
		result.reserve(2U);
		AppendByteHex(result, value);

		return result;
	}

	std::string BytesHex(const std::uint8_t* data, std::size_t size)
	{
		const auto input_is_empty = size == 0U;
		if (input_is_empty)
		{
			return "<empty>";
		}

		std::string result;
		const auto max_result_size = result.max_size();
		const auto output_too_large = size > (max_result_size / 2U);
		if (output_too_large)
		{
			return "<too-large>";
		}

		result.reserve(size * 2U);
		for (std::size_t index = 0; index < size; ++index)
		{
			AppendByteHex(result, data[index]);
		}

		return result;
	}

	std::string NormalizeHex(std::string_view hex)
	{
		std::string result;
		result.reserve(hex.size());

		for (const auto digit : hex)
		{
			const auto digit_is_whitespace = IsAsciiWhitespace(digit);
			if (digit_is_whitespace)
			{
				continue;
			}

			const auto value = static_cast<std::uint8_t>(HexDigitValue(digit));
			result.push_back(HexDigitChar(value));
		}

		const auto input_is_empty = result.empty();
		if (input_is_empty)
		{
			return "<empty>";
		}

		return result;
	}

	constexpr std::uint8_t HexByteAt(std::string_view hex, std::size_t byte_offset) noexcept
	{
		auto byte_index = std::size_t{0U};
		auto has_high = false;
		auto high = 0;

		for (const auto digit : hex)
		{
			const auto digit_is_whitespace = IsAsciiWhitespace(digit);
			if (digit_is_whitespace)
			{
				continue;
			}

			const auto value = HexDigitValue(digit);
			if (!has_high)
			{
				high = value;
				has_high = true;
				continue;
			}

			const auto byte_matches_offset = byte_index == byte_offset;
			if (byte_matches_offset)
			{
				return static_cast<std::uint8_t>((high << 4U) | value);
			}

			++byte_index;
			has_high = false;
		}

		return 0U;
	}

	std::size_t FirstBytesMismatchOffset(const std::uint8_t* expected,
										 std::size_t expected_size,
										 const std::uint8_t* actual,
										 std::size_t actual_size) noexcept
	{
		const auto common_size = expected_size < actual_size ? expected_size : actual_size;
		for (std::size_t index = 0; index < common_size; ++index)
		{
			const auto bytes_match = expected[index] == actual[index];
			if (!bytes_match)
			{
				return index;
			}
		}

		return common_size;
	}

	::testing::AssertionResult InvalidPointerFailure(std::string_view name, std::size_t size)
	{
		return ::testing::AssertionFailure()
			<< name << " is nullptr with non-zero size " << size << ".";
	}

	::testing::AssertionResult ByteMismatchFailure(const std::uint8_t* expected,
												   std::size_t expected_size,
												   const std::uint8_t* actual,
												   std::size_t actual_size,
												   std::size_t offset)
	{
		const auto expected_hex = BytesHex(expected, expected_size);
		const auto actual_hex = BytesHex(actual, actual_size);
		const auto offset_has_expected_byte = offset < expected_size;
		const auto offset_has_actual_byte = offset < actual_size;

		auto failure = ::testing::AssertionFailure()
			<< "byte sequences differ at offset " << offset << " (expected size " << expected_size
			<< ", actual size " << actual_size << ")";

		const auto offset_has_both_bytes = offset_has_expected_byte && offset_has_actual_byte;
		if (offset_has_both_bytes)
		{
			const auto expected_byte = ByteHex(expected[offset]);
			const auto actual_byte = ByteHex(actual[offset]);
			failure << ": expected 0x" << expected_byte << ", actual 0x" << actual_byte;
		}
		else
		{
			failure << ": one sequence ended";
		}

		failure << ". expected hex: " << expected_hex << "; actual hex: " << actual_hex << ".";

		return failure;
	}

	std::string InvalidHexDigitText(char digit)
	{
		const auto unsigned_digit = static_cast<unsigned char>(digit);
		const auto digit_is_printable = unsigned_digit >= 0x20U && unsigned_digit <= 0x7EU;
		if (digit_is_printable)
		{
			return std::string("'") + digit + "'";
		}

		return std::string("0x") + ByteHex(static_cast<std::uint8_t>(unsigned_digit));
	}

	::testing::AssertionResult InvalidHexDigitFailure(std::size_t character_index, char digit)
	{
		const auto digit_text = InvalidHexDigitText(digit);

		return ::testing::AssertionFailure()
			<< "invalid expected hex at character " << character_index << ": " << digit_text << ".";
	}

	::testing::AssertionResult InvalidHexLengthFailure(std::size_t digit_count)
	{
		return ::testing::AssertionFailure()
			<< "invalid expected hex: odd hex digit count " << digit_count << ".";
	}

	::testing::AssertionResult HexMismatchFailure(std::string_view expected_hex,
												  const std::uint8_t* actual,
												  std::size_t actual_size,
												  std::size_t offset)
	{
		const auto normalized_expected_hex = NormalizeHex(expected_hex);
		const auto actual_hex = BytesHex(actual, actual_size);
		const auto expected_size = CountHexDigits(expected_hex) / 2U;
		const auto offset_has_expected_byte = offset < expected_size;
		const auto offset_has_actual_byte = offset < actual_size;

		auto failure = ::testing::AssertionFailure()
			<< "hex byte sequences differ at offset " << offset << " (expected size "
			<< expected_size << ", actual size " << actual_size << ")";

		const auto offset_has_both_bytes = offset_has_expected_byte && offset_has_actual_byte;
		if (offset_has_both_bytes)
		{
			const auto expected_byte = ByteHex(HexByteAt(expected_hex, offset));
			const auto actual_byte = ByteHex(actual[offset]);
			failure << ": expected 0x" << expected_byte << ", actual 0x" << actual_byte;
		}
		else
		{
			failure << ": one sequence ended";
		}

		failure << ". expected hex: " << normalized_expected_hex << "; actual hex: " << actual_hex
				<< ".";

		return failure;
	}

} // namespace

namespace ket
{
	namespace testing
	{
		::testing::AssertionResult BytesEqual(const std::uint8_t* expected,
											  std::size_t expected_size,
											  const std::uint8_t* actual,
											  std::size_t actual_size)
		{
			const auto expected_pointer_invalid = HasInvalidBytePointer(expected, expected_size);
			if (expected_pointer_invalid)
			{
				return InvalidPointerFailure("expected", expected_size);
			}

			const auto actual_pointer_invalid = HasInvalidBytePointer(actual, actual_size);
			if (actual_pointer_invalid)
			{
				return InvalidPointerFailure("actual", actual_size);
			}

			const auto mismatch_offset =
				FirstBytesMismatchOffset(expected, expected_size, actual, actual_size);
			const auto bytes_are_equal =
				expected_size == actual_size && mismatch_offset == expected_size;
			if (bytes_are_equal)
			{
				return ::testing::AssertionSuccess();
			}

			return ByteMismatchFailure(
				expected, expected_size, actual, actual_size, mismatch_offset);
		}

		::testing::AssertionResult
		HexEqual(std::string_view expected_hex, const std::uint8_t* actual, std::size_t actual_size)
		{
			const auto expected_hex_size = expected_hex.size();
			auto expected_hex_digit_count = std::size_t{0U};

			for (std::size_t character_index = 0U; character_index < expected_hex_size;
				 ++character_index)
			{
				const auto digit = expected_hex[character_index];
				const auto digit_is_whitespace = IsAsciiWhitespace(digit);
				if (digit_is_whitespace)
				{
					continue;
				}

				const auto digit_value = HexDigitValue(digit);
				const auto digit_is_valid = digit_value >= 0;
				if (!digit_is_valid)
				{
					return InvalidHexDigitFailure(character_index, digit);
				}

				++expected_hex_digit_count;
			}

			const auto expected_hex_has_odd_length = (expected_hex_digit_count % 2U) != 0U;
			if (expected_hex_has_odd_length)
			{
				return InvalidHexLengthFailure(expected_hex_digit_count);
			}

			const auto actual_pointer_invalid = HasInvalidBytePointer(actual, actual_size);
			if (actual_pointer_invalid)
			{
				return InvalidPointerFailure("actual", actual_size);
			}

			const auto expected_size = expected_hex_digit_count / 2U;
			const auto common_size = expected_size < actual_size ? expected_size : actual_size;
			auto offset = std::size_t{0U};
			auto has_high = false;
			auto high = 0;
			for (const auto digit : expected_hex)
			{
				const auto digit_is_whitespace = IsAsciiWhitespace(digit);
				if (digit_is_whitespace)
				{
					continue;
				}

				const auto value = HexDigitValue(digit);
				if (!has_high)
				{
					high = value;
					has_high = true;
					continue;
				}

				const auto expected_byte = static_cast<std::uint8_t>((high << 4U) | value);
				const auto offset_is_common = offset < common_size;
				if (!offset_is_common)
				{
					break;
				}

				const auto bytes_match = expected_byte == actual[offset];
				if (!bytes_match)
				{
					return HexMismatchFailure(expected_hex, actual, actual_size, offset);
				}

				++offset;
				has_high = false;
			}

			const auto sizes_match = expected_size == actual_size;
			if (!sizes_match)
			{
				return HexMismatchFailure(expected_hex, actual, actual_size, common_size);
			}

			return ::testing::AssertionSuccess();
		}

	} // namespace testing

} // namespace ket
