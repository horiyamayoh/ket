#include "ket_uuid.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace
{
	constexpr std::array<std::size_t, 5U> kGroupByteCounts{{4U, 2U, 2U, 2U, 6U}};
	constexpr std::size_t kUuidByteCount = kGroupByteCounts[0] + kGroupByteCounts[1] +
		kGroupByteCounts[2] + kGroupByteCounts[3] + kGroupByteCounts[4];
	constexpr std::size_t kCanonicalTextLength =
		(kUuidByteCount * 2U) + kGroupByteCounts.size() - 1U;

	constexpr bool IsHyphenIndex(std::size_t index) noexcept
	{
		std::size_t boundary_byte_count = 0U;
		for (std::size_t group_index = 0U; group_index + 1U < kGroupByteCounts.size();
			 ++group_index)
		{
			boundary_byte_count += kGroupByteCounts[group_index];
			const auto hyphen_index = (boundary_byte_count * 2U) + group_index;
			const auto index_matches = index == hyphen_index;
			if (index_matches)
			{
				return true;
			}
		}

		return false;
	}

	constexpr bool NeedsHyphenBeforeByte(std::size_t byte_index) noexcept
	{
		std::size_t boundary_byte_count = 0U;
		for (std::size_t group_index = 0U; group_index + 1U < kGroupByteCounts.size();
			 ++group_index)
		{
			boundary_byte_count += kGroupByteCounts[group_index];
			const auto byte_index_matches = byte_index == boundary_byte_count;
			if (byte_index_matches)
			{
				return true;
			}
		}

		return false;
	}

	constexpr std::optional<std::uint8_t> HexValue(char value) noexcept
	{
		const auto is_decimal = value >= '0' && value <= '9';
		if (is_decimal)
		{
			return static_cast<std::uint8_t>(value - '0');
		}

		const auto is_lower_hex = value >= 'a' && value <= 'f';
		if (is_lower_hex)
		{
			return static_cast<std::uint8_t>((value - 'a') + 10);
		}

		const auto is_upper_hex = value >= 'A' && value <= 'F';
		if (is_upper_hex)
		{
			return static_cast<std::uint8_t>((value - 'A') + 10);
		}

		return std::nullopt;
	}

	constexpr char LowerHexChar(std::uint8_t value) noexcept
	{
		const auto value_is_decimal = value < 10U;
		if (value_is_decimal)
		{
			return static_cast<char>('0' + value);
		}

		return static_cast<char>('a' + (value - 10U));
	}

} // namespace

namespace ket
{
	namespace uuid
	{
		std::optional<Uuid> Parse(std::string_view text) noexcept
		{
			const auto text_size = text.size();
			const auto text_has_expected_length = text_size == kCanonicalTextLength;
			if (!text_has_expected_length)
			{
				return std::nullopt;
			}

			Uuid result;
			std::size_t byte_index = 0U;
			bool next_is_high_nibble = true;

			for (std::size_t text_index = 0U; text_index < text_size; ++text_index)
			{
				const auto text_char = text[text_index];
				const auto index_is_hyphen = IsHyphenIndex(text_index);
				if (index_is_hyphen)
				{
					const auto char_is_hyphen = text_char == '-';
					if (!char_is_hyphen)
					{
						return std::nullopt;
					}

					continue;
				}

				const auto char_is_hyphen = text_char == '-';
				if (char_is_hyphen)
				{
					return std::nullopt;
				}

				const auto digit = HexValue(text_char);
				const auto digit_has_value = digit.has_value();
				if (!digit_has_value)
				{
					return std::nullopt;
				}

				if (next_is_high_nibble)
				{
					result.bytes[byte_index] = static_cast<std::uint8_t>(*digit << 4U);
					next_is_high_nibble = false;
				}
				else
				{
					result.bytes[byte_index] =
						static_cast<std::uint8_t>(result.bytes[byte_index] | *digit);
					next_is_high_nibble = true;
					++byte_index;
				}
			}

			const auto consumed_all_bytes = byte_index == kUuidByteCount && next_is_high_nibble;
			if (!consumed_all_bytes)
			{
				return std::nullopt;
			}

			return result;
		}

		std::string Format(const Uuid& value)
		{
			std::string result;
			result.reserve(kCanonicalTextLength);

			for (std::size_t byte_index = 0U; byte_index < kUuidByteCount; ++byte_index)
			{
				const auto needs_hyphen = NeedsHyphenBeforeByte(byte_index);
				if (needs_hyphen)
				{
					result.push_back('-');
				}

				const auto byte = value.bytes[byte_index];
				const auto high = static_cast<std::uint8_t>((byte >> 4U) & 0x0FU);
				const auto low = static_cast<std::uint8_t>(byte & 0x0FU);
				result.push_back(LowerHexChar(high));
				result.push_back(LowerHexChar(low));
			}

			return result;
		}

	} // namespace uuid

} // namespace ket
