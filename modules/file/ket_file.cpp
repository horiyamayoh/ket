#include "ket_file.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios> // IWYU pragma: keep
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
	void ClearError(std::error_code* error) noexcept
	{
		if (error != nullptr)
		{
			error->clear();
		}
	}

	void SetError(std::error_code* error, std::error_code value) noexcept
	{
		if (error != nullptr)
		{
			*error = value;
		}
	}

	std::error_code MakeFileTooLargeError()
	{
		return std::make_error_code(std::errc::file_too_large);
	}

	std::error_code MakeInvalidArgumentError()
	{
		return std::make_error_code(std::errc::invalid_argument);
	}

	std::error_code MakeIoError()
	{
		return std::make_error_code(std::errc::io_error);
	}

	std::error_code MakeIsDirectoryError()
	{
		return std::make_error_code(std::errc::is_a_directory);
	}

	std::error_code MakeNoSuchFileError()
	{
		return std::make_error_code(std::errc::no_such_file_or_directory);
	}

	std::error_code MakeOperationNotSupportedError()
	{
		return std::make_error_code(std::errc::operation_not_supported);
	}

	std::optional<std::streamsize> TryGetStreamSize(std::uintmax_t size,
													std::error_code* error) noexcept
	{
		const auto max_stream_size =
			static_cast<std::uintmax_t>((std::numeric_limits<std::streamsize>::max)());
		const auto size_too_large = size > max_stream_size;
		if (size_too_large)
		{
			SetError(error, MakeFileTooLargeError());
			return std::nullopt;
		}

		return static_cast<std::streamsize>(size);
	}

	template <typename Buffer>
	bool FileSizeFitsBuffer(std::uintmax_t file_size,
							const Buffer& buffer,
							std::error_code* error) noexcept
	{
		const auto exceeds_buffer_size = file_size > buffer.max_size();
		if (exceeds_buffer_size)
		{
			SetError(error, MakeFileTooLargeError());
			return false;
		}

		return true;
	}

	void SetOpenError(const std::filesystem::path& path, std::error_code* error) noexcept
	{
		std::error_code status_error;
		const auto status = std::filesystem::status(path, status_error);
		const auto status_failed = static_cast<bool>(status_error);
		if (status_failed)
		{
			SetError(error, status_error);
			return;
		}

		const auto path_exists = std::filesystem::exists(status);
		if (!path_exists)
		{
			SetError(error, MakeNoSuchFileError());
			return;
		}

		const auto path_is_directory = std::filesystem::is_directory(status);
		if (path_is_directory)
		{
			SetError(error, MakeIsDirectoryError());
			return;
		}

		const auto path_is_regular_file = std::filesystem::is_regular_file(status);
		if (!path_is_regular_file)
		{
			SetError(error, MakeOperationNotSupportedError());
			return;
		}

		SetError(error, MakeIoError());
	}

	bool TryCheckReadableFilePath(const std::filesystem::path& path,
								  std::error_code* error) noexcept
	{
		std::error_code status_error;
		const auto status = std::filesystem::status(path, status_error);
		const auto status_failed = static_cast<bool>(status_error);
		if (status_failed)
		{
			SetError(error, status_error);
			return false;
		}

		const auto path_exists = std::filesystem::exists(status);
		if (!path_exists)
		{
			SetError(error, MakeNoSuchFileError());
			return false;
		}

		const auto path_is_directory = std::filesystem::is_directory(status);
		if (path_is_directory)
		{
			SetError(error, MakeIsDirectoryError());
			return false;
		}

		const auto path_is_regular_file = std::filesystem::is_regular_file(status);
		if (!path_is_regular_file)
		{
			SetError(error, MakeOperationNotSupportedError());
			return false;
		}

		return true;
	}

	bool TryCheckWritableFilePath(const std::filesystem::path& path,
								  std::error_code* error) noexcept
	{
		std::error_code status_error;
		const auto status = std::filesystem::status(path, status_error);
		const auto status_failed = static_cast<bool>(status_error);
		if (status_failed)
		{
			const auto path_is_missing = status_error == MakeNoSuchFileError();
			if (path_is_missing)
			{
				return true;
			}

			SetError(error, status_error);
			return false;
		}

		const auto path_exists = std::filesystem::exists(status);
		if (!path_exists)
		{
			return true;
		}

		const auto path_is_directory = std::filesystem::is_directory(status);
		if (path_is_directory)
		{
			SetError(error, MakeIsDirectoryError());
			return false;
		}

		const auto path_is_regular_file = std::filesystem::is_regular_file(status);
		if (!path_is_regular_file)
		{
			SetError(error, MakeOperationNotSupportedError());
			return false;
		}

		return true;
	}

	bool TryOpenInputAtEnd(const std::filesystem::path& path,
						   std::ifstream& stream,
						   std::uintmax_t& out,
						   std::error_code* error)
	{
		const auto path_can_be_read = TryCheckReadableFilePath(path, error);
		if (!path_can_be_read)
		{
			return false;
		}

		stream.open(path, std::ios::binary | std::ios::ate);
		const auto stream_is_open = stream.is_open();
		if (!stream_is_open)
		{
			SetOpenError(path, error);
			return false;
		}

		const auto end_position = stream.tellg();
		const auto tell_failed = end_position == std::ifstream::pos_type(-1);
		if (tell_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		const auto end_offset = static_cast<std::streamoff>(end_position);
		const auto negative_size = end_offset < 0;
		if (negative_size)
		{
			SetError(error, MakeIoError());
			return false;
		}

		out = static_cast<std::uintmax_t>(end_offset);

		stream.seekg(0, std::ios::beg);
		const auto seek_failed = !stream;
		if (seek_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		return true;
	}

	bool
	TryReadExact(std::ifstream& stream, char* data, std::streamsize size, std::error_code* error)
	{
		const auto input_is_empty = size == 0;
		if (input_is_empty)
		{
			return true;
		}

		stream.read(data, size);
		const auto read_failed = !stream;
		if (read_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		return true;
	}

	bool TryReadEof(std::ifstream& stream, std::error_code* error)
	{
		const auto next = stream.peek();
		const auto read_failed = stream.bad();
		if (read_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		const auto reached_end = next == std::char_traits<char>::eof();
		if (reached_end)
		{
			return true;
		}

		SetError(error, MakeIoError());
		return false;
	}

	bool TryWriteExact(std::ofstream& stream,
					   const char* data,
					   std::streamsize size,
					   std::error_code* error)
	{
		const auto input_is_empty = size == 0;
		if (input_is_empty)
		{
			return true;
		}

		stream.write(data, size);
		const auto write_failed = !stream;
		if (write_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		return true;
	}

	bool TryWriteAllRaw(const std::filesystem::path& path,
						const char* data,
						std::size_t size,
						std::error_code* error)
	{
		ClearError(error);

		const auto data_is_missing = data == nullptr && size > 0U;
		if (data_is_missing)
		{
			SetError(error, MakeInvalidArgumentError());
			return false;
		}

		const auto stream_size = TryGetStreamSize(static_cast<std::uintmax_t>(size), error);
		const auto stream_size_available = stream_size.has_value();
		if (!stream_size_available)
		{
			return false;
		}

		const auto path_can_be_written = TryCheckWritableFilePath(path, error);
		if (!path_can_be_written)
		{
			return false;
		}

		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		const auto stream_is_open = stream.is_open();
		if (!stream_is_open)
		{
			SetOpenError(path, error);
			return false;
		}

		const auto write_succeeded = TryWriteExact(stream, data, *stream_size, error);
		if (!write_succeeded)
		{
			return false;
		}

		stream.close();
		const auto close_failed = !stream;
		if (close_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		ClearError(error);
		return true;
	}

} // namespace

namespace ket
{
	namespace file
	{
		bool
		TryReadAllText(const std::filesystem::path& path, std::string& out, std::error_code* error)
		{
			ClearError(error);

			std::ifstream stream;
			std::uintmax_t file_size = 0;
			const auto file_size_available = TryOpenInputAtEnd(path, stream, file_size, error);
			if (!file_size_available)
			{
				return false;
			}

			const auto stream_size = TryGetStreamSize(file_size, error);
			const auto stream_size_available = stream_size.has_value();
			if (!stream_size_available)
			{
				return false;
			}

			std::string buffer;
			const auto file_size_fits = FileSizeFitsBuffer(file_size, buffer, error);
			if (!file_size_fits)
			{
				return false;
			}

			buffer.resize(static_cast<std::size_t>(file_size));

			const auto read_succeeded = TryReadExact(stream, buffer.data(), *stream_size, error);
			if (!read_succeeded)
			{
				return false;
			}

			const auto reached_end = TryReadEof(stream, error);
			if (!reached_end)
			{
				return false;
			}

			out = std::move(buffer);
			ClearError(error);
			return true;
		}

		bool TryReadAllBytes(const std::filesystem::path& path,
							 std::vector<std::uint8_t>& out,
							 std::error_code* error)
		{
			ClearError(error);

			std::ifstream stream;
			std::uintmax_t file_size = 0;
			const auto file_size_available = TryOpenInputAtEnd(path, stream, file_size, error);
			if (!file_size_available)
			{
				return false;
			}

			const auto stream_size = TryGetStreamSize(file_size, error);
			const auto stream_size_available = stream_size.has_value();
			if (!stream_size_available)
			{
				return false;
			}

			std::vector<std::uint8_t> buffer;
			const auto file_size_fits = FileSizeFitsBuffer(file_size, buffer, error);
			if (!file_size_fits)
			{
				return false;
			}

			buffer.resize(static_cast<std::size_t>(file_size));

			auto* const data = reinterpret_cast<char*>(buffer.data());
			const auto read_succeeded = TryReadExact(stream, data, *stream_size, error);
			if (!read_succeeded)
			{
				return false;
			}

			const auto reached_end = TryReadEof(stream, error);
			if (!reached_end)
			{
				return false;
			}

			out = std::move(buffer);
			ClearError(error);
			return true;
		}

		bool TryWriteAllText(const std::filesystem::path& path,
							 std::string_view text,
							 std::error_code* error)
		{
			return TryWriteAllRaw(path, text.data(), text.size(), error);
		}

		bool TryWriteAllBytes(const std::filesystem::path& path,
							  const std::uint8_t* data,
							  std::size_t size,
							  std::error_code* error)
		{
			const auto* const raw_data = reinterpret_cast<const char*>(data);
			return TryWriteAllRaw(path, raw_data, size, error);
		}

		bool Exists(const std::filesystem::path& path) noexcept
		{
			std::error_code error;
			const auto exists = std::filesystem::exists(path, error);
			const auto query_failed = static_cast<bool>(error);
			if (query_failed)
			{
				return false;
			}

			return exists;
		}

		bool IsDirectory(const std::filesystem::path& path) noexcept
		{
			std::error_code error;
			const auto is_directory = std::filesystem::is_directory(path, error);
			const auto query_failed = static_cast<bool>(error);
			if (query_failed)
			{
				return false;
			}

			return is_directory;
		}

		std::optional<std::uintmax_t> Size(const std::filesystem::path& path) noexcept
		{
			std::error_code error;
			const auto file_size = std::filesystem::file_size(path, error);
			const auto query_failed = static_cast<bool>(error);
			if (query_failed)
			{
				return std::nullopt;
			}

			return file_size;
		}

	} // namespace file

} // namespace ket
