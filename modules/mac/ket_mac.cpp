#include "ket_mac.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace
{
	constexpr std::size_t kAddressByteCount = 6U;
	constexpr std::size_t kFormattedLength = 17U;
	constexpr std::size_t kHexDigitsPerByte = 2U;
	constexpr std::size_t kTextStride = 3U;
	constexpr char kColonSeparator = ':';
	constexpr char kHyphenSeparator = '-';

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

	constexpr bool IsAllowedSeparator(char value) noexcept
	{
		return value == kColonSeparator || value == kHyphenSeparator;
	}

	bool TryDecodeByte(char high_char, char low_char, std::uint8_t& out) noexcept
	{
		const auto high_is_hex = IsHexDigit(high_char);
		const auto low_is_hex = IsHexDigit(low_char);
		const auto chars_are_hex = high_is_hex && low_is_hex;
		if (!chars_are_hex)
		{
			return false;
		}

		const auto high = HexDigitValue(high_char);
		const auto low = HexDigitValue(low_char);
		out = static_cast<std::uint8_t>((high << 4U) | low);
		return true;
	}

	constexpr char HexChar(std::uint8_t value, ket::mac::LetterCase letter_case) noexcept
	{
		constexpr char kLowerDigits[] = "0123456789abcdef";
		constexpr char kUpperDigits[] = "0123456789ABCDEF";
		const auto use_upper = letter_case == ket::mac::LetterCase::kUpper;
		const char* const digits = use_upper ? kUpperDigits : kLowerDigits;

		return digits[static_cast<std::size_t>(value)];
	}

} // namespace

namespace ket
{
	namespace mac
	{
		bool operator==(Address lhs, Address rhs) noexcept
		{
			for (std::size_t index = 0; index < kAddressByteCount; ++index)
			{
				const auto byte_matches = lhs.bytes[index] == rhs.bytes[index];
				if (!byte_matches)
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(Address lhs, Address rhs) noexcept
		{
			const auto values_are_equal = lhs == rhs;
			return !values_are_equal;
		}

		std::optional<Address> Parse(std::string_view text) noexcept
		{
			const auto has_expected_length = text.size() == kFormattedLength;
			if (!has_expected_length)
			{
				return std::nullopt;
			}

			const auto separator = text[kHexDigitsPerByte];
			const auto separator_is_allowed = IsAllowedSeparator(separator);
			if (!separator_is_allowed)
			{
				return std::nullopt;
			}

			Address result{};

			for (std::size_t index = 0; index < kAddressByteCount; ++index)
			{
				const auto text_index = index * kTextStride;
				std::uint8_t byte = 0U;

				const auto byte_is_decoded =
					TryDecodeByte(text[text_index], text[text_index + 1U], byte);
				if (!byte_is_decoded)
				{
					return std::nullopt;
				}

				result.bytes[index] = byte;

				const auto is_last_byte = index + 1U == kAddressByteCount;
				if (is_last_byte)
				{
					continue;
				}

				const auto actual_separator = text[text_index + kHexDigitsPerByte];
				const auto separator_matches = actual_separator == separator;
				if (!separator_matches)
				{
					return std::nullopt;
				}
			}

			return result;
		}

		std::string Format(Address value, LetterCase letter_case)
		{
			std::string result;
			result.reserve(kFormattedLength);

			for (std::size_t index = 0; index < kAddressByteCount; ++index)
			{
				const auto needs_separator = index != 0U;
				if (needs_separator)
				{
					result.push_back(kColonSeparator);
				}

				const auto byte = value.bytes[index];
				const auto high = static_cast<std::uint8_t>((byte >> 4U) & 0x0FU);
				const auto low = static_cast<std::uint8_t>(byte & 0x0FU);
				result.push_back(HexChar(high, letter_case));
				result.push_back(HexChar(low, letter_case));
			}

			return result;
		}

	} // namespace mac

} // namespace ket
