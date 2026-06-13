#include "ket_bcd.h"

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
	std::optional<std::string> BcdToDecimalString(std::uint8_t const* data, std::size_t size)
	{
		// 入力妥当性確認
		if (data == nullptr || size == 0U)
		{
			return std::nullopt;
		}

		std::string result;

		// 出力長確認
		auto const max_result_size = result.max_size();
		auto const output_too_large = size > max_result_size / 2U;
		if (output_too_large)
		{
			return std::nullopt;
		}

		result.reserve(size * 2U);

		for (std::size_t index = 0; index < size; ++index)
		{
			auto const high = HighNibble(data[index]);
			auto const low = LowNibble(data[index]);

			// BCD桁妥当性確認
			auto const high_is_bcd = detail::IsBcdNibble(high);
			auto const low_is_bcd = detail::IsBcdNibble(low);
			auto const byte_is_bcd = high_is_bcd && low_is_bcd;
			if (!byte_is_bcd)
			{
				return std::nullopt;
			}

			result.push_back(ToDecimalChar(high));
			result.push_back(ToDecimalChar(low));
		}

		return result;
	}

	std::optional<std::vector<std::uint8_t>> DecimalStringToBcd(std::string_view text)
	{
		// 入力妥当性確認
		auto const text_is_empty = text.empty();
		if (text_is_empty)
		{
			return std::nullopt;
		}

		std::vector<std::uint8_t> result;
		auto const text_size = text.size();
		auto const output_size = (text_size / 2U) + (text_size % 2U);

		// 出力長確認
		auto const max_result_size = result.max_size();
		auto const output_too_large = output_size > max_result_size;
		if (output_too_large)
		{
			return std::nullopt;
		}

		result.reserve(output_size);

		std::size_t index = 0;
		// 奇数桁の先頭0補完
		auto const text_has_odd_length = (text_size % 2U) != 0U;
		if (text_has_odd_length)
		{
			auto const low_char = text[index];
			auto const low_is_decimal = IsDecimalChar(low_char);
			if (!low_is_decimal)
			{
				return std::nullopt;
			}

			auto const low = DecimalCharToNibble(low_char);
			result.push_back(PackBcdNibbles(0U, low));
			++index;
		}

		for (; index < text_size; index += 2U)
		{
			auto const high_char = text[index];
			auto const low_char = text[index + 1U];

			// 10進数字妥当性確認
			auto const high_is_decimal = IsDecimalChar(high_char);
			auto const low_is_decimal = IsDecimalChar(low_char);
			auto const chars_are_decimal = high_is_decimal && low_is_decimal;
			if (!chars_are_decimal)
			{
				return std::nullopt;
			}

			auto const high = DecimalCharToNibble(high_char);
			auto const low = DecimalCharToNibble(low_char);
			result.push_back(PackBcdNibbles(high, low));
		}

		return result;
	}

} // namespace ket
