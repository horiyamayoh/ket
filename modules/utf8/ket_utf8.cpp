#include "ket_utf8.h"

#include <cstddef>
#include <optional>
#include <string_view>

namespace
{
	struct ScanResult
	{
		ket::utf8::ValidationResult validation;
		std::size_t code_point_count = 0;
	};

	constexpr unsigned char ToByte(char value) noexcept
	{
		return static_cast<unsigned char>(value);
	}

	constexpr bool IsContinuationByte(unsigned char value) noexcept
	{
		return value >= 0x80U && value <= 0xBFU;
	}

	constexpr ket::utf8::ValidationResult ValidResult() noexcept
	{
		return {true, 0U};
	}

	constexpr ket::utf8::ValidationResult InvalidResult(std::size_t offset) noexcept
	{
		return {false, offset};
	}

	constexpr ScanResult ValidScanResult(std::size_t code_point_count) noexcept
	{
		return {ValidResult(), code_point_count};
	}

	constexpr ScanResult InvalidScanResult(std::size_t error_offset,
										   std::size_t code_point_count) noexcept
	{
		return {InvalidResult(error_offset), code_point_count};
	}

	constexpr std::size_t RemainingBytes(std::size_t size, std::size_t index) noexcept
	{
		return size - index;
	}

	ScanResult ScanUtf8(std::string_view text) noexcept
	{
		const auto size = text.size();
		std::size_t index = 0;
		std::size_t code_point_count = 0;

		while (index < size)
		{
			const auto first = ToByte(text[index]);

			const auto first_is_ascii = first <= 0x7FU;
			if (first_is_ascii)
			{
				++index;
				++code_point_count;
				continue;
			}

			const auto first_is_two_byte_lead = first >= 0xC2U && first <= 0xDFU;
			if (first_is_two_byte_lead)
			{
				const auto remaining = RemainingBytes(size, index);
				const auto sequence_is_truncated = remaining < 2U;
				if (sequence_is_truncated)
				{
					return InvalidScanResult(index, code_point_count);
				}

				const auto second = ToByte(text[index + 1U]);
				const auto second_is_continuation = IsContinuationByte(second);
				if (!second_is_continuation)
				{
					return InvalidScanResult(index + 1U, code_point_count);
				}

				index += 2U;
				++code_point_count;
				continue;
			}

			const auto first_is_three_byte_lead = first >= 0xE0U && first <= 0xEFU;
			if (first_is_three_byte_lead)
			{
				const auto remaining = RemainingBytes(size, index);
				const auto sequence_is_truncated = remaining < 3U;
				if (sequence_is_truncated)
				{
					return InvalidScanResult(index, code_point_count);
				}

				const auto second = ToByte(text[index + 1U]);
				const auto third = ToByte(text[index + 2U]);
				const auto first_is_overlong_boundary = first == 0xE0U;
				const auto first_is_surrogate_boundary = first == 0xEDU;
				const auto second_matches_e0_range = second >= 0xA0U && second <= 0xBFU;
				const auto second_matches_ed_range = second >= 0x80U && second <= 0x9FU;
				const auto second_is_continuation = IsContinuationByte(second);

				if (first_is_overlong_boundary)
				{
					if (!second_matches_e0_range)
					{
						return InvalidScanResult(index + 1U, code_point_count);
					}
				}
				else if (first_is_surrogate_boundary)
				{
					if (!second_matches_ed_range)
					{
						return InvalidScanResult(index + 1U, code_point_count);
					}
				}
				else if (!second_is_continuation)
				{
					return InvalidScanResult(index + 1U, code_point_count);
				}

				const auto third_is_continuation = IsContinuationByte(third);
				if (!third_is_continuation)
				{
					return InvalidScanResult(index + 2U, code_point_count);
				}

				index += 3U;
				++code_point_count;
				continue;
			}

			const auto first_is_four_byte_lead = first >= 0xF0U && first <= 0xF4U;
			if (first_is_four_byte_lead)
			{
				const auto remaining = RemainingBytes(size, index);
				const auto sequence_is_truncated = remaining < 4U;
				if (sequence_is_truncated)
				{
					return InvalidScanResult(index, code_point_count);
				}

				const auto second = ToByte(text[index + 1U]);
				const auto third = ToByte(text[index + 2U]);
				const auto fourth = ToByte(text[index + 3U]);
				const auto first_is_overlong_boundary = first == 0xF0U;
				const auto first_is_max_boundary = first == 0xF4U;
				const auto second_matches_f0_range = second >= 0x90U && second <= 0xBFU;
				const auto second_matches_f4_range = second >= 0x80U && second <= 0x8FU;
				const auto second_is_continuation = IsContinuationByte(second);

				if (first_is_overlong_boundary)
				{
					if (!second_matches_f0_range)
					{
						return InvalidScanResult(index + 1U, code_point_count);
					}
				}
				else if (first_is_max_boundary)
				{
					if (!second_matches_f4_range)
					{
						return InvalidScanResult(index + 1U, code_point_count);
					}
				}
				else if (!second_is_continuation)
				{
					return InvalidScanResult(index + 1U, code_point_count);
				}

				const auto third_is_continuation = IsContinuationByte(third);
				if (!third_is_continuation)
				{
					return InvalidScanResult(index + 2U, code_point_count);
				}

				const auto fourth_is_continuation = IsContinuationByte(fourth);
				if (!fourth_is_continuation)
				{
					return InvalidScanResult(index + 3U, code_point_count);
				}

				index += 4U;
				++code_point_count;
				continue;
			}

			return InvalidScanResult(index, code_point_count);
		}

		return ValidScanResult(code_point_count);
	}

} // namespace

namespace ket
{
	namespace utf8
	{
		bool IsAscii(std::string_view text) noexcept
		{
			const auto size = text.size();

			for (std::size_t index = 0; index < size; ++index)
			{
				const auto byte = ToByte(text[index]);
				const auto byte_is_ascii = byte <= 0x7FU;
				if (!byte_is_ascii)
				{
					return false;
				}
			}

			return true;
		}

		ValidationResult Validate(std::string_view text) noexcept
		{
			const auto result = ScanUtf8(text);
			return result.validation;
		}

		bool IsValid(std::string_view text) noexcept
		{
			const auto result = Validate(text);
			return result.valid;
		}

		std::optional<std::size_t> CountCodePoints(std::string_view text) noexcept
		{
			const auto result = ScanUtf8(text);
			if (!result.validation.valid)
			{
				return std::nullopt;
			}

			return result.code_point_count;
		}

	} // namespace utf8

} // namespace ket
