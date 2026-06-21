#include "ket_version.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
	constexpr bool IsDecimalDigit(char value) noexcept
	{
		return value >= '0' && value <= '9';
	}

	constexpr std::uint32_t DecimalDigitValue(char value) noexcept
	{
		return static_cast<std::uint32_t>(value - '0');
	}

	std::optional<std::uint32_t> ParseComponent(std::string_view text) noexcept
	{
		// 空要素確認
		const auto text_is_empty = text.empty();
		if (text_is_empty)
		{
			return std::nullopt;
		}

		// leading zero確認
		const auto has_multiple_digits = text.size() > 1U;
		const auto starts_with_zero = text.front() == '0';
		const auto has_leading_zero = has_multiple_digits && starts_with_zero;
		if (has_leading_zero)
		{
			return std::nullopt;
		}

		std::uint32_t result = 0U;
		const auto max_value = std::numeric_limits<std::uint32_t>::max();

		for (const char character : text)
		{
			const auto character_is_digit = IsDecimalDigit(character);
			if (!character_is_digit)
			{
				return std::nullopt;
			}

			const auto digit = DecimalDigitValue(character);
			const auto max_before_append = (max_value - digit) / 10U;
			const auto would_overflow = result > max_before_append;
			if (would_overflow)
			{
				return std::nullopt;
			}

			result = (result * 10U) + digit;
		}

		return result;
	}

	struct ComponentText
	{
		std::array<char,
				   static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::digits10) + 1U>
			data{};
		std::size_t size = 0U;
	};

	ComponentText FormatComponent(std::uint32_t value)
	{
		ComponentText result;
		auto* const begin = result.data.data();
		auto* const end = begin + result.data.size();

		const auto converted = std::to_chars(begin, end, value);
		const auto conversion_failed = converted.ec != std::errc();
		if (conversion_failed)
		{
			std::terminate();
		}

		result.size = static_cast<std::size_t>(converted.ptr - begin);
		return result;
	}

	void AppendComponent(std::string& result, const ComponentText& component)
	{
		result.append(component.data.data(), component.size);
	}

	int CompareComponent(std::uint32_t a, std::uint32_t b) noexcept
	{
		const auto less = a < b;
		if (less)
		{
			return -1;
		}

		const auto greater = a > b;
		if (greater)
		{
			return 1;
		}

		return 0;
	}

} // namespace

namespace ket
{
	namespace version
	{
		std::optional<Triplet> Parse(std::string_view text) noexcept
		{
			// 入力妥当性確認
			const auto text_is_empty = text.empty();
			if (text_is_empty)
			{
				return std::nullopt;
			}

			std::array<std::uint32_t, 3> components{};
			const auto text_size = text.size();
			std::size_t component_start = 0U;
			std::size_t component_count = 0U;

			for (std::size_t index = 0U; index <= text_size; ++index)
			{
				const auto at_end = index == text_size;
				const auto at_separator = !at_end && text[index] == '.';
				const auto component_ended = at_end || at_separator;
				if (!component_ended)
				{
					continue;
				}

				const auto component_count_too_many = component_count >= components.size();
				if (component_count_too_many)
				{
					return std::nullopt;
				}

				const auto component_length = index - component_start;
				const auto component_text =
					std::string_view(text.data() + component_start, component_length);
				const auto component = ParseComponent(component_text);
				const auto component_has_value = component.has_value();
				if (!component_has_value)
				{
					return std::nullopt;
				}

				components[component_count] = *component;
				++component_count;
				component_start = index + 1U;
			}

			const auto component_count_is_valid = component_count == components.size();
			if (!component_count_is_valid)
			{
				return std::nullopt;
			}

			return Triplet{components[0], components[1], components[2]};
		}

		std::string Format(Triplet version)
		{
			const auto major = FormatComponent(version.major);
			const auto minor = FormatComponent(version.minor);
			const auto patch = FormatComponent(version.patch);

			const auto output_size = major.size + minor.size + patch.size + 2U;
			std::string result;
			result.reserve(output_size);

			AppendComponent(result, major);
			result.push_back('.');
			AppendComponent(result, minor);
			result.push_back('.');
			AppendComponent(result, patch);

			return result;
		}

		int Compare(Triplet a, Triplet b) noexcept
		{
			const auto major_order = CompareComponent(a.major, b.major);
			const auto major_differs = major_order != 0;
			if (major_differs)
			{
				return major_order;
			}

			const auto minor_order = CompareComponent(a.minor, b.minor);
			const auto minor_differs = minor_order != 0;
			if (minor_differs)
			{
				return minor_order;
			}

			return CompareComponent(a.patch, b.patch);
		}

	} // namespace version

} // namespace ket
