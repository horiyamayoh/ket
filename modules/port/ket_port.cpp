#include "ket_port.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace
{
	constexpr bool IsDecimalChar(char value) noexcept
	{
		return value >= '0' && value <= '9';
	}

	constexpr std::uint32_t DecimalCharToUInt(char value) noexcept
	{
		return static_cast<std::uint32_t>(value - '0');
	}

	constexpr bool HasLeadingZero(std::string_view text) noexcept
	{
		const auto text_size = text.size();
		const auto has_multiple_digits = text_size > 1U;
		if (!has_multiple_digits)
		{
			return false;
		}

		return text[0] == '0';
	}

	constexpr bool CanAppendDigit(std::uint32_t value, std::uint32_t digit) noexcept
	{
		const auto digit_is_decimal = digit <= 9U;
		if (!digit_is_decimal)
		{
			return false;
		}

		const auto max_before_append = (ket::port::detail::kMaxPortValue - digit) / 10U;
		return value <= max_before_append;
	}

} // namespace

namespace ket
{
	namespace port
	{
		std::optional<Port> Parse(std::string_view text) noexcept
		{
			// 入力妥当性確認
			const auto text_is_empty = text.empty();
			if (text_is_empty)
			{
				return std::nullopt;
			}

			const auto text_size = text.size();
			const auto text_too_long = text_size > 5U;
			if (text_too_long)
			{
				return std::nullopt;
			}

			const auto text_has_leading_zero = HasLeadingZero(text);
			if (text_has_leading_zero)
			{
				return std::nullopt;
			}

			std::uint32_t value = 0U;
			for (const char character : text)
			{
				const auto character_is_decimal = IsDecimalChar(character);
				if (!character_is_decimal)
				{
					return std::nullopt;
				}

				const auto digit = DecimalCharToUInt(character);
				const auto append_is_safe = CanAppendDigit(value, digit);
				if (!append_is_safe)
				{
					return std::nullopt;
				}

				value = (value * 10U) + digit;
			}

			Port result;
			const auto value_is_port = TryFromUInt(value, result);
			if (!value_is_port)
			{
				return std::nullopt;
			}

			return result;
		}

		std::string Format(Port port)
		{
			return std::to_string(static_cast<unsigned int>(port.value));
		}

	} // namespace port

} // namespace ket
