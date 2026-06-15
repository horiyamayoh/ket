#include "ket_hex.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr char kLowerHexDigits[] = "0123456789abcdef";
	constexpr char kUpperHexDigits[] = "0123456789ABCDEF";
	constexpr std::size_t kDumpReservedRowSize = 79U;

	constexpr const char* HexDigits(ket::hex::LetterCase letter_case) noexcept
	{
		const auto uses_lower = letter_case == ket::hex::LetterCase::kLower;
		if (uses_lower)
		{
			return kLowerHexDigits;
		}

		return kUpperHexDigits;
	}

	constexpr bool IsAsciiWhitespace(char value) noexcept
	{
		return value == ' ' || (value >= '\t' && value <= '\r');
	}

	constexpr bool IsHexDigit(char value) noexcept
	{
		return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
			(value >= 'A' && value <= 'F');
	}

	constexpr std::uint8_t HexDigitValue(char value) noexcept
	{
		const auto is_decimal = value >= '0' && value <= '9';
		if (is_decimal)
		{
			return static_cast<std::uint8_t>(value - '0');
		}

		const auto is_lower = value >= 'a' && value <= 'f';
		if (is_lower)
		{
			return static_cast<std::uint8_t>((value - 'a') + 10);
		}

		return static_cast<std::uint8_t>((value - 'A') + 10);
	}

	constexpr bool IsPrintableAscii(std::uint8_t value) noexcept
	{
		return value >= 0x20U && value <= 0x7EU;
	}

	constexpr char ToAsciiPreviewChar(std::uint8_t value) noexcept
	{
		const auto value_is_printable = IsPrintableAscii(value);
		if (value_is_printable)
		{
			return static_cast<char>(value);
		}

		return '.';
	}

	std::size_t HexDigitCount(std::uint64_t value) noexcept
	{
		std::size_t digit_count = 1U;
		auto remaining = value;

		while (remaining > 0x0FULL)
		{
			remaining >>= 4U;
			++digit_count;
		}

		return digit_count;
	}

	void AppendHexDigits(std::string& destination,
						 std::uint64_t value,
						 std::size_t min_width,
						 ket::hex::LetterCase letter_case)
	{
		const auto* const digits = HexDigits(letter_case);
		const auto digit_count = HexDigitCount(value);
		const auto output_width = std::max(digit_count, min_width);
		const auto padding = output_width - digit_count;

		destination.append(padding, '0');

		for (std::size_t digit_index = 0; digit_index < digit_count; ++digit_index)
		{
			const auto reverse_index = digit_count - digit_index - 1U;
			const auto shift = static_cast<unsigned>(reverse_index * 4U);
			const auto digit = static_cast<unsigned>((value >> shift) & 0x0FULL);
			destination.push_back(digits[digit]);
		}
	}

	void
	AppendByteHex(std::string& destination, std::uint8_t value, ket::hex::LetterCase letter_case)
	{
		const auto* const digits = HexDigits(letter_case);
		const auto high = (value >> 4U) & 0x0FU;
		const auto low = value & 0x0FU;

		destination.push_back(digits[high]);
		destination.push_back(digits[low]);
	}

	std::size_t EncodedByteStringSize(std::size_t size, char separator, std::size_t max_size)
	{
		const auto has_separator = separator != '\0';
		const auto separator_count = has_separator && size > 0U ? size - 1U : 0U;
		const auto separators_exceed_max = separator_count > max_size;
		if (separators_exceed_max)
		{
			throw std::length_error("ket hex result exceeds std::string::max_size().");
		}

		const auto remaining_size = max_size - separator_count;
		const auto bytes_exceed_max = size > (remaining_size / ket::hex::detail::kByteHexWidth);
		if (bytes_exceed_max)
		{
			throw std::length_error("ket hex result exceeds std::string::max_size().");
		}

		return (size * ket::hex::detail::kByteHexWidth) + separator_count;
	}

	void ReserveDumpString(std::string& destination, std::size_t size)
	{
		const auto full_row_count = size / ket::hex::detail::kDumpBytesPerRow;
		const auto has_partial_row = (size % ket::hex::detail::kDumpBytesPerRow) != 0U;
		const auto row_count = full_row_count + (has_partial_row ? 1U : 0U);
		const auto max_size = destination.max_size();
		const auto row_count_exceeds_max = row_count > (max_size / kDumpReservedRowSize);
		if (row_count_exceeds_max)
		{
			throw std::length_error("ket hex dump result exceeds std::string::max_size().");
		}

		destination.reserve(row_count * kDumpReservedRowSize);
	}

	void
	AppendDumpHexArea(std::string& destination, const std::uint8_t* row_data, std::size_t row_size)
	{
		for (std::size_t byte_index = 0; byte_index < ket::hex::detail::kDumpBytesPerRow;
			 ++byte_index)
		{
			const auto starts_second_half = byte_index == ket::hex::detail::kDumpHalfRowBytes;
			if (starts_second_half)
			{
				destination.push_back(' ');
			}

			const auto byte_exists = byte_index < row_size;
			if (byte_exists)
			{
				AppendByteHex(destination, row_data[byte_index], ket::hex::LetterCase::kLower);
				destination.push_back(' ');
				continue;
			}

			destination.append(3U, ' ');
		}
	}

	void AppendDumpAsciiPreview(std::string& destination,
								const std::uint8_t* row_data,
								std::size_t row_size)
	{
		destination.push_back(' ');
		destination.push_back('|');

		for (std::size_t byte_index = 0; byte_index < row_size; ++byte_index)
		{
			destination.push_back(ToAsciiPreviewChar(row_data[byte_index]));
		}

		destination.push_back('|');
	}

} // namespace

namespace ket
{
	namespace hex
	{
		std::string Encode(std::uint64_t value, unsigned min_width, LetterCase letter_case)
		{
			std::string result;
			const auto output_width =
				std::max(HexDigitCount(value), static_cast<std::size_t>(min_width));

			result.reserve(output_width);
			AppendHexDigits(result, value, output_width, letter_case);

			return result;
		}

		std::string Encode(const std::uint8_t* data, std::size_t size, FormatOptions options)
		{
			std::string result;
			const auto input_is_empty = size == 0U;
			if (input_is_empty)
			{
				return result;
			}

			const auto output_size =
				EncodedByteStringSize(size, options.separator, result.max_size());
			result.reserve(output_size);

			for (std::size_t index = 0; index < size; ++index)
			{
				const auto is_not_first_byte = index != 0U;
				const auto uses_separator = options.separator != '\0';
				if (is_not_first_byte && uses_separator)
				{
					result.push_back(options.separator);
				}

				AppendByteHex(result, data[index], options.letter_case);
			}

			return result;
		}

		std::optional<std::vector<std::uint8_t>> Decode(std::string_view text)
		{
			std::vector<std::uint8_t> result;
			result.reserve(text.size() / detail::kByteHexWidth);

			bool has_high_nibble = false;
			std::uint8_t high_nibble = 0U;

			for (const auto value : text)
			{
				const auto is_whitespace = IsAsciiWhitespace(value);
				if (is_whitespace)
				{
					continue;
				}

				const auto is_hex_digit = IsHexDigit(value);
				if (!is_hex_digit)
				{
					return std::nullopt;
				}

				const auto nibble = HexDigitValue(value);
				if (!has_high_nibble)
				{
					high_nibble = nibble;
					has_high_nibble = true;
					continue;
				}

				const auto byte = static_cast<std::uint8_t>((high_nibble << 4U) | nibble);
				result.push_back(byte);
				has_high_nibble = false;
			}

			if (has_high_nibble)
			{
				return std::nullopt;
			}

			return result;
		}

		std::string Dump(const std::uint8_t* data, std::size_t size)
		{
			std::string result;
			const auto input_is_empty = size == 0U;
			if (input_is_empty)
			{
				return result;
			}

			ReserveDumpString(result, size);

			for (std::size_t offset = 0U; offset < size;)
			{
				const auto is_not_first_row = offset != 0U;
				if (is_not_first_row)
				{
					result.push_back('\n');
				}

				const auto remaining_size = size - offset;
				const auto row_size = std::min(remaining_size, detail::kDumpBytesPerRow);
				const auto* const row_data = data + offset;

				AppendHexDigits(result,
								static_cast<std::uint64_t>(offset),
								detail::kOffsetHexWidth,
								LetterCase::kLower);
				result.append(2U, ' ');
				AppendDumpHexArea(result, row_data, row_size);
				AppendDumpAsciiPreview(result, row_data, row_size);

				offset += row_size;
			}

			return result;
		}

		std::string Dump(const void* data, std::size_t size)
		{
			const auto* const bytes = static_cast<const std::uint8_t*>(data);

			return Dump(bytes, size);
		}

	} // namespace hex

} // namespace ket
