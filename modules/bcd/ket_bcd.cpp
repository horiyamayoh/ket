#include "ket_bcd.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr char ToDecimalChar(std::uint8_t value) noexcept
	{
		return static_cast<char>('0' + value);
	}

	constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept
	{
		return static_cast<std::uint8_t>((value >> 4U) & 0x0FU);
	}

	constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept
	{
		return static_cast<std::uint8_t>(value & 0x0FU);
	}

	constexpr bool IsDecimalChar(char value) noexcept
	{
		return value >= '0' && value <= '9';
	}

	constexpr std::uint8_t DecimalCharToNibble(char value) noexcept
	{
		return static_cast<std::uint8_t>(value - '0');
	}

	constexpr std::uint8_t PackBcdNibbles(std::uint8_t high, std::uint8_t low) noexcept
	{
		return static_cast<std::uint8_t>((high << 4U) | low);
	}

} // namespace

namespace ket
{
	namespace bcd
	{
		std::optional<std::string> ToDecimalString(const std::uint8_t* data, std::size_t size)
		{
			// 入力妥当性確認
			if (data == nullptr || size == 0U)
			{
				return std::nullopt;
			}

			std::string result;

			// 出力長確認
			const auto max_result_size = result.max_size();
			const auto output_too_large = size > max_result_size / 2U;
			if (output_too_large)
			{
				return std::nullopt;
			}

			result.reserve(size * 2U);

			for (std::size_t index = 0; index < size; ++index)
			{
				const auto high = HighNibble(data[index]);
				const auto low = LowNibble(data[index]);

				// BCD桁妥当性確認
				const auto high_is_bcd = detail::IsBcdNibble(high);
				const auto low_is_bcd = detail::IsBcdNibble(low);
				const auto byte_is_bcd = high_is_bcd && low_is_bcd;
				if (!byte_is_bcd)
				{
					return std::nullopt;
				}

				result.push_back(ToDecimalChar(high));
				result.push_back(ToDecimalChar(low));
			}

			return result;
		}

		std::optional<std::vector<std::uint8_t>> FromDecimalString(std::string_view text)
		{
			// 入力妥当性確認
			const auto text_is_empty = text.empty();
			if (text_is_empty)
			{
				return std::nullopt;
			}

			std::vector<std::uint8_t> result;
			const auto text_size = text.size();
			const auto output_size = (text_size / 2U) + (text_size % 2U);

			// 出力長確認
			const auto max_result_size = result.max_size();
			const auto output_too_large = output_size > max_result_size;
			if (output_too_large)
			{
				return std::nullopt;
			}

			result.reserve(output_size);

			std::size_t index = 0;
			// 奇数桁の先頭0補完
			const auto text_has_odd_length = (text_size % 2U) != 0U;
			if (text_has_odd_length)
			{
				const auto low_char = text[index];
				const auto low_is_decimal = IsDecimalChar(low_char);
				if (!low_is_decimal)
				{
					return std::nullopt;
				}

				const auto low = DecimalCharToNibble(low_char);
				result.push_back(PackBcdNibbles(0U, low));
				++index;
			}

			for (; index < text_size; index += 2U)
			{
				const auto high_char = text[index];
				const auto low_char = text[index + 1U];

				// 10進数字妥当性確認
				const auto high_is_decimal = IsDecimalChar(high_char);
				const auto low_is_decimal = IsDecimalChar(low_char);
				const auto chars_are_decimal = high_is_decimal && low_is_decimal;
				if (!chars_are_decimal)
				{
					return std::nullopt;
				}

				const auto high = DecimalCharToNibble(high_char);
				const auto low = DecimalCharToNibble(low_char);
				result.push_back(PackBcdNibbles(high, low));
			}

			return result;
		}

	} // namespace bcd

} // namespace ket
