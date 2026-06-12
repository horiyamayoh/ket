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

} // namespace ket
