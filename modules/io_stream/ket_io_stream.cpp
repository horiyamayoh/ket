#include "ket_io_stream.h"

#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <limits>
#include <ostream> // IWYU pragma: keep
#include <string>

namespace
{
	constexpr bool IsAsciiWhitespace(char value) noexcept
	{
		return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' ||
			value == '\v';
	}

	std::streamsize NextStreamChunkSize(std::size_t remaining) noexcept
	{
		const auto max_stream_size =
			static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max());
		const auto chunk_size = remaining < max_stream_size ? remaining : max_stream_size;
		return static_cast<std::streamsize>(chunk_size);
	}

	void TrimRightAsciiWhitespace(std::string& text)
	{
		auto trim_size = text.size();
		while (trim_size > 0U)
		{
			const auto last_index = trim_size - 1U;
			const auto last_is_ascii_whitespace = IsAsciiWhitespace(text[last_index]);
			if (!last_is_ascii_whitespace)
			{
				break;
			}

			--trim_size;
		}

		text.erase(trim_size);
	}

} // namespace

namespace ket
{
	namespace io_stream
	{
		bool TryReadExactly(std::istream& stream, std::uint8_t* data, std::size_t size)
		{
			const auto input_is_empty = size == 0U;
			if (input_is_empty)
			{
				return true;
			}

			if (data == nullptr)
			{
				return false;
			}

			std::size_t total_read = 0U;
			while (total_read < size)
			{
				const auto remaining = size - total_read;
				const auto chunk_size = NextStreamChunkSize(remaining);
				stream.read(reinterpret_cast<char*>(data + total_read), chunk_size);

				const auto bytes_read = stream.gcount();
				const auto chunk_fully_read = bytes_read == chunk_size;
				if (!chunk_fully_read)
				{
					return false;
				}

				total_read += static_cast<std::size_t>(chunk_size);
			}

			return true;
		}

		bool TryWriteAll(std::ostream& stream, const std::uint8_t* data, std::size_t size)
		{
			const auto input_is_empty = size == 0U;
			if (input_is_empty)
			{
				return true;
			}

			if (data == nullptr)
			{
				return false;
			}

			std::size_t total_written = 0U;
			while (total_written < size)
			{
				const auto remaining = size - total_written;
				const auto chunk_size = NextStreamChunkSize(remaining);
				stream.write(reinterpret_cast<const char*>(data + total_written), chunk_size);

				const auto stream_is_ready = static_cast<bool>(stream);
				if (!stream_is_ready)
				{
					return false;
				}

				total_written += static_cast<std::size_t>(chunk_size);
			}

			return true;
		}

		bool TryReadLineTrimRightAscii(std::istream& stream, std::string& out)
		{
			std::string line;
			std::getline(stream, line);

			const auto line_was_read = !stream.fail();
			if (!line_was_read)
			{
				return false;
			}

			TrimRightAsciiWhitespace(line);
			out = line;
			return true;
		}

		FormatStateSaver::FormatStateSaver(std::ios& stream)
			: stream_(stream), flags_(stream.flags()), precision_(stream.precision()),
			  fill_(stream.fill())
		{
		}

		FormatStateSaver::~FormatStateSaver() noexcept
		{
			stream_.flags(flags_);
			stream_.precision(precision_);
			stream_.fill(fill_);
		}

	} // namespace io_stream

} // namespace ket
