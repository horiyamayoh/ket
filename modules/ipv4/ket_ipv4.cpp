#include "ket_ipv4.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace
{
	constexpr std::size_t kAddressOctetCount = 4U;
	constexpr std::uint32_t kMaxOctetValue = 255U;

	constexpr bool IsDecimalDigit(char value) noexcept
	{
		return value >= '0' && value <= '9';
	}

	constexpr std::uint32_t DecimalDigitValue(char value) noexcept
	{
		return static_cast<std::uint32_t>(value - '0');
	}

	bool ParseOctet(std::string_view text,
					std::size_t begin,
					std::size_t end,
					std::uint8_t& out) noexcept
	{
		const auto octet_is_empty = begin == end;
		if (octet_is_empty)
		{
			return false;
		}

		const auto octet_length = end - begin;
		const auto has_leading_zero = octet_length > 1U && text[begin] == '0';
		if (has_leading_zero)
		{
			return false;
		}

		std::uint32_t value = 0U;
		for (std::size_t index = begin; index < end; ++index)
		{
			const auto current = text[index];
			const auto current_is_digit = IsDecimalDigit(current);
			if (!current_is_digit)
			{
				return false;
			}

			value = (value * 10U) + DecimalDigitValue(current);
			const auto value_too_large = value > kMaxOctetValue;
			if (value_too_large)
			{
				return false;
			}
		}

		out = static_cast<std::uint8_t>(value);
		return true;
	}

	void AppendDecimalOctet(std::string& destination, std::uint8_t octet)
	{
		destination += std::to_string(static_cast<unsigned int>(octet));
	}

} // namespace

namespace ket
{
	namespace ipv4
	{
		std::optional<Address> Parse(std::string_view text) noexcept
		{
			Address result;
			const auto text_size = text.size();
			const auto text_is_empty = text_size == 0U;
			if (text_is_empty)
			{
				return std::nullopt;
			}

			std::size_t octet_index = 0U;
			std::size_t octet_begin = 0U;
			for (std::size_t index = 0U; index <= text_size; ++index)
			{
				const auto at_end = index == text_size;
				const auto current_is_dot = !at_end && text[index] == '.';
				const auto should_close_octet = at_end || current_is_dot;
				if (!should_close_octet)
				{
					continue;
				}

				const auto has_too_many_octets = octet_index >= kAddressOctetCount;
				if (has_too_many_octets)
				{
					return std::nullopt;
				}

				std::uint8_t octet = 0U;
				const auto octet_is_valid = ParseOctet(text, octet_begin, index, octet);
				if (!octet_is_valid)
				{
					return std::nullopt;
				}

				result.octets[octet_index] = octet;
				++octet_index;
				octet_begin = index + 1U;
			}

			const auto has_exact_octets = octet_index == kAddressOctetCount;
			if (!has_exact_octets)
			{
				return std::nullopt;
			}

			return result;
		}

		std::string Format(Address value)
		{
			std::string result;
			result.reserve(15U);

			for (std::size_t index = 0U; index < kAddressOctetCount; ++index)
			{
				const auto needs_dot = index != 0U;
				if (needs_dot)
				{
					result.push_back('.');
				}

				AppendDecimalOctet(result, value.octets[index]);
			}

			return result;
		}

		std::uint32_t ToBe32(Address value) noexcept
		{
			return (static_cast<std::uint32_t>(value.octets[0]) << 24U) |
				(static_cast<std::uint32_t>(value.octets[1]) << 16U) |
				(static_cast<std::uint32_t>(value.octets[2]) << 8U) |
				static_cast<std::uint32_t>(value.octets[3]);
		}

		Address FromBe32(std::uint32_t value) noexcept
		{
			Address result;
			result.octets[0] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
			result.octets[1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
			result.octets[2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
			result.octets[3] = static_cast<std::uint8_t>(value & 0xFFU);

			return result;
		}

	} // namespace ipv4

} // namespace ket
