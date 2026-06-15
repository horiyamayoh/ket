#include "ket_ascii.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr std::size_t kNotFound = std::string_view::npos;

	bool IsAsciiWhitespace(char value) noexcept
	{
		return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' ||
			value == '\v';
	}

	bool IsAsciiUpper(char value) noexcept
	{
		return value >= 'A' && value <= 'Z';
	}

	bool IsAsciiLower(char value) noexcept
	{
		return value >= 'a' && value <= 'z';
	}

	char ToLowerAscii(char value) noexcept
	{
		const auto value_is_upper = IsAsciiUpper(value);
		if (!value_is_upper)
		{
			return value;
		}

		return static_cast<char>(value + ('a' - 'A'));
	}

	char ToUpperAscii(char value) noexcept
	{
		const auto value_is_lower = IsAsciiLower(value);
		if (!value_is_lower)
		{
			return value;
		}

		return static_cast<char>(value - ('a' - 'A'));
	}

	std::string_view MakeView(std::string_view text, std::size_t offset, std::size_t size) noexcept
	{
		const auto offset_is_zero = offset == 0U;
		if (offset_is_zero)
		{
			return {text.data(), size};
		}

		return {text.data() + offset, size};
	}

	bool EqualBytesAt(std::string_view text, std::size_t offset, std::string_view expected) noexcept
	{
		const auto expected_size = expected.size();
		for (std::size_t index = 0; index < expected_size; ++index)
		{
			const auto text_char = text[offset + index];
			const auto expected_char = expected[index];
			const auto chars_match = text_char == expected_char;
			if (!chars_match)
			{
				return false;
			}
		}

		return true;
	}

	std::size_t
	FindBytes(std::string_view text, std::string_view needle, std::size_t start) noexcept
	{
		const auto text_size = text.size();
		const auto needle_size = needle.size();
		const auto needle_is_empty = needle_size == 0U;
		if (needle_is_empty)
		{
			return start;
		}

		const auto start_is_past_end = start > text_size;
		if (start_is_past_end)
		{
			return kNotFound;
		}

		const auto needle_too_long = needle_size > (text_size - start);
		if (needle_too_long)
		{
			return kNotFound;
		}

		const auto last_position = text_size - needle_size;
		for (std::size_t position = start; position <= last_position; ++position)
		{
			const auto matches_here = EqualBytesAt(text, position, needle);
			if (matches_here)
			{
				return position;
			}
		}

		return kNotFound;
	}

	std::size_t CountSplitFields(std::string_view text, char delimiter) noexcept
	{
		std::size_t count = 1U;
		const auto text_size = text.size();

		for (std::size_t index = 0; index < text_size; ++index)
		{
			const auto char_is_delimiter = text[index] == delimiter;
			if (char_is_delimiter)
			{
				++count;
			}
		}

		return count;
	}

	void AppendView(std::string& destination, std::string_view part)
	{
		const auto part_is_empty = part.empty();
		if (part_is_empty)
		{
			return;
		}

		destination.append(part.data(), part.size());
	}

	std::string StringFromView(std::string_view part)
	{
		const auto part_is_empty = part.empty();
		if (part_is_empty)
		{
			return {};
		}

		return {part.data(), part.size()};
	}

	std::size_t AddSize(std::size_t total_size, std::size_t part_size, std::size_t max_size)
	{
		const auto size_would_exceed = part_size > (max_size - total_size);
		if (size_would_exceed)
		{
			throw std::length_error("ket ascii result exceeds std::string::max_size().");
		}

		return total_size + part_size;
	}

	std::size_t JoinedSize(const std::vector<std::string_view>& parts,
						   std::string_view delimiter,
						   std::size_t max_size)
	{
		std::size_t total_size = 0U;
		const auto part_count = parts.size();
		const auto delimiter_size = delimiter.size();

		for (std::size_t index = 0; index < part_count; ++index)
		{
			const auto part_size = parts[index].size();
			total_size = AddSize(total_size, part_size, max_size);

			const auto needs_delimiter = (index + 1U) < part_count;
			if (needs_delimiter)
			{
				total_size = AddSize(total_size, delimiter_size, max_size);
			}
		}

		return total_size;
	}

} // namespace

namespace ket
{
	namespace ascii
	{
		bool StartsWith(std::string_view text, std::string_view prefix) noexcept
		{
			const auto text_size = text.size();
			const auto prefix_size = prefix.size();
			const auto prefix_is_empty = prefix_size == 0U;
			if (prefix_is_empty)
			{
				return true;
			}

			const auto prefix_too_long = prefix_size > text_size;
			if (prefix_too_long)
			{
				return false;
			}

			return EqualBytesAt(text, 0U, prefix);
		}

		bool EndsWith(std::string_view text, std::string_view suffix) noexcept
		{
			const auto text_size = text.size();
			const auto suffix_size = suffix.size();
			const auto suffix_is_empty = suffix_size == 0U;
			if (suffix_is_empty)
			{
				return true;
			}

			const auto suffix_too_long = suffix_size > text_size;
			if (suffix_too_long)
			{
				return false;
			}

			const auto suffix_offset = text_size - suffix_size;
			return EqualBytesAt(text, suffix_offset, suffix);
		}

		bool Contains(std::string_view text, std::string_view needle) noexcept
		{
			const auto position = FindBytes(text, needle, 0U);
			return position != kNotFound;
		}

		std::string_view Trim(std::string_view text) noexcept
		{
			const auto left_trimmed = TrimLeft(text);
			return TrimRight(left_trimmed);
		}

		std::string_view TrimLeft(std::string_view text) noexcept
		{
			const auto text_size = text.size();
			std::size_t begin = 0U;

			while (begin < text_size)
			{
				const auto char_is_whitespace = IsAsciiWhitespace(text[begin]);
				if (!char_is_whitespace)
				{
					break;
				}

				++begin;
			}

			return MakeView(text, begin, text_size - begin);
		}

		std::string_view TrimRight(std::string_view text) noexcept
		{
			std::size_t end = text.size();

			while (end > 0U)
			{
				const auto char_is_whitespace = IsAsciiWhitespace(text[end - 1U]);
				if (!char_is_whitespace)
				{
					break;
				}

				--end;
			}

			return MakeView(text, 0U, end);
		}

		std::string_view StripPrefix(std::string_view text, std::string_view prefix) noexcept
		{
			const auto text_starts_with_prefix = StartsWith(text, prefix);
			if (!text_starts_with_prefix)
			{
				return text;
			}

			const auto prefix_size = prefix.size();
			const auto text_size = text.size();
			return MakeView(text, prefix_size, text_size - prefix_size);
		}

		std::string_view StripSuffix(std::string_view text, std::string_view suffix) noexcept
		{
			const auto text_ends_with_suffix = EndsWith(text, suffix);
			if (!text_ends_with_suffix)
			{
				return text;
			}

			const auto suffix_size = suffix.size();
			const auto text_size = text.size();
			return MakeView(text, 0U, text_size - suffix_size);
		}

		std::vector<std::string_view> SplitViews(std::string_view text, char delimiter)
		{
			std::vector<std::string_view> result;
			const auto field_count = CountSplitFields(text, delimiter);
			result.reserve(field_count);

			const auto text_size = text.size();
			std::size_t field_begin = 0U;

			for (std::size_t index = 0; index <= text_size; ++index)
			{
				const auto reached_end = index == text_size;
				const auto found_delimiter = !reached_end && text[index] == delimiter;
				const auto should_emit_field = reached_end || found_delimiter;
				if (should_emit_field)
				{
					const auto field_size = index - field_begin;
					result.push_back(MakeView(text, field_begin, field_size));
					field_begin = index + 1U;
				}
			}

			return result;
		}

		std::vector<std::string> Split(std::string_view text, char delimiter)
		{
			const auto views = SplitViews(text, delimiter);
			const auto view_count = views.size();

			std::vector<std::string> result;
			result.reserve(view_count);

			std::transform(views.begin(), views.end(), std::back_inserter(result), StringFromView);

			return result;
		}

		std::string Join(const std::vector<std::string_view>& parts, std::string_view delimiter)
		{
			std::string result;
			const auto max_size = result.max_size();
			const auto result_size = JoinedSize(parts, delimiter, max_size);
			result.reserve(result_size);

			const auto part_count = parts.size();
			for (std::size_t index = 0; index < part_count; ++index)
			{
				AppendView(result, parts[index]);

				const auto needs_delimiter = (index + 1U) < part_count;
				if (needs_delimiter)
				{
					AppendView(result, delimiter);
				}
			}

			return result;
		}

		std::string ReplaceAll(std::string_view text, std::string_view from, std::string_view to)
		{
			const auto from_is_empty = from.empty();
			if (from_is_empty)
			{
				return StringFromView(text);
			}

			std::string result;
			const auto text_size = text.size();
			std::size_t cursor = 0U;

			while (cursor < text_size)
			{
				const auto match_position = FindBytes(text, from, cursor);
				const auto match_found = match_position != kNotFound;
				if (!match_found)
				{
					AppendView(result, MakeView(text, cursor, text_size - cursor));
					return result;
				}

				AppendView(result, MakeView(text, cursor, match_position - cursor));
				AppendView(result, to);
				cursor = match_position + from.size();
			}

			return result;
		}

		std::string ToLower(std::string_view text)
		{
			std::string result;
			const auto text_size = text.size();
			result.reserve(text_size);

			std::transform(text.begin(), text.end(), std::back_inserter(result), ToLowerAscii);

			return result;
		}

		std::string ToUpper(std::string_view text)
		{
			std::string result;
			const auto text_size = text.size();
			result.reserve(text_size);

			std::transform(text.begin(), text.end(), std::back_inserter(result), ToUpperAscii);

			return result;
		}

		bool EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept
		{
			const auto a_size = a.size();
			const auto b_size = b.size();
			const auto sizes_match = a_size == b_size;
			if (!sizes_match)
			{
				return false;
			}

			for (std::size_t index = 0; index < a_size; ++index)
			{
				const auto a_char = ToLowerAscii(a[index]);
				const auto b_char = ToLowerAscii(b[index]);
				const auto chars_match = a_char == b_char;
				if (!chars_match)
				{
					return false;
				}
			}

			return true;
		}

	} // namespace ascii

} // namespace ket
