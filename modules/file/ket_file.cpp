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

	bool TryGetStreamSize(std::size_t size, std::streamsize& out, std::error_code* error) noexcept
	{
		const auto max_stream_size =
			static_cast<std::uintmax_t>((std::numeric_limits<std::streamsize>::max)());
		const auto size_as_uintmax = static_cast<std::uintmax_t>(size);
		const auto size_too_large = size_as_uintmax > max_stream_size;
		if (size_too_large)
		{
			SetError(error, MakeFileTooLargeError());
			return false;
		}

		out = static_cast<std::streamsize>(size);
		return true;
	}

	bool TryGetFileSize(const std::filesystem::path& path,
						std::uintmax_t& out,
						std::error_code* error) noexcept
	{
		std::error_code file_size_error;
		const auto file_size = std::filesystem::file_size(path, file_size_error);
		const auto query_failed = static_cast<bool>(file_size_error);
		if (query_failed)
		{
			SetError(error, file_size_error);
			return false;
		}

		out = file_size;
		return true;
	}

	bool FileSizeFitsString(std::uintmax_t file_size,
							const std::string& buffer,
							std::error_code* error) noexcept
	{
		const auto exceeds_string_size = file_size > buffer.max_size();
		if (exceeds_string_size)
		{
			SetError(error, MakeFileTooLargeError());
			return false;
		}

		return true;
	}

	bool FileSizeFitsVector(std::uintmax_t file_size,
							const std::vector<std::uint8_t>& buffer,
							std::error_code* error) noexcept
	{
		const auto exceeds_vector_size = file_size > buffer.max_size();
		if (exceeds_vector_size)
		{
			SetError(error, MakeFileTooLargeError());
			return false;
		}

		return true;
	}

	bool TryReadExact(std::ifstream& stream, char* data, std::size_t size, std::error_code* error)
	{
		const auto input_is_empty = size == 0U;
		if (input_is_empty)
		{
			return true;
		}

		std::streamsize stream_size = 0;
		const auto stream_size_available = TryGetStreamSize(size, stream_size, error);
		if (!stream_size_available)
		{
			return false;
		}

		stream.read(data, stream_size);
		const auto read_failed = !stream;
		if (read_failed)
		{
			SetError(error, MakeIoError());
			return false;
		}

		return true;
	}

	bool
	TryWriteExact(std::ofstream& stream, const char* data, std::size_t size, std::error_code* error)
	{
		const auto input_is_empty = size == 0U;
		if (input_is_empty)
		{
			return true;
		}

		std::streamsize stream_size = 0;
		const auto stream_size_available = TryGetStreamSize(size, stream_size, error);
		if (!stream_size_available)
		{
			return false;
		}

		stream.write(data, stream_size);
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

		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		const auto stream_is_open = stream.is_open();
		if (!stream_is_open)
		{
			SetError(error, MakeIoError());
			return false;
		}

		const auto write_succeeded = TryWriteExact(stream, data, size, error);
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

			std::uintmax_t file_size = 0;
			const auto file_size_available = TryGetFileSize(path, file_size, error);
			if (!file_size_available)
			{
				return false;
			}

			std::string buffer;
			const auto file_size_fits = FileSizeFitsString(file_size, buffer, error);
			if (!file_size_fits)
			{
				return false;
			}

			buffer.resize(static_cast<std::size_t>(file_size));

			std::ifstream stream(path, std::ios::binary);
			const auto stream_is_open = stream.is_open();
			if (!stream_is_open)
			{
				SetError(error, MakeIoError());
				return false;
			}

			const auto read_succeeded = TryReadExact(stream, buffer.data(), buffer.size(), error);
			if (!read_succeeded)
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

			std::uintmax_t file_size = 0;
			const auto file_size_available = TryGetFileSize(path, file_size, error);
			if (!file_size_available)
			{
				return false;
			}

			std::vector<std::uint8_t> buffer;
			const auto file_size_fits = FileSizeFitsVector(file_size, buffer, error);
			if (!file_size_fits)
			{
				return false;
			}

			buffer.resize(static_cast<std::size_t>(file_size));

			std::ifstream stream(path, std::ios::binary);
			const auto stream_is_open = stream.is_open();
			if (!stream_is_open)
			{
				SetError(error, MakeIoError());
				return false;
			}

			auto* const data = reinterpret_cast<char*>(buffer.data());
			const auto read_succeeded = TryReadExact(stream, data, buffer.size(), error);
			if (!read_succeeded)
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
