#include "ket_platform.h"

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <cstring>
#include <limits>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <string.h> // NOLINT(modernize-deprecated-headers)
#endif

namespace
{
	constexpr std::size_t kErrnoMessageBufferSize = 256U;

	std::string FormatUnknownErrno(int error_number)
	{
		return std::string("Unknown error ") + std::to_string(error_number);
	}

	bool EnvironmentVariableNameIsValid(std::string_view name) noexcept
	{
		const auto name_is_empty = name.empty();
		if (name_is_empty)
		{
			return false;
		}

		const auto nul_position = name.find('\0');
		const auto name_contains_nul = nul_position != std::string_view::npos;
		return !name_contains_nul;
	}

	std::string CopyErrnoMessageOrFallback(const char* message, int error_number)
	{
		const auto message_is_null = message == nullptr;
		if (message_is_null)
		{
			return FormatUnknownErrno(error_number);
		}

		const auto message_is_empty = message[0] == '\0';
		if (message_is_empty)
		{
			return FormatUnknownErrno(error_number);
		}

		return {message};
	}

#ifdef _WIN32
	std::string FormatUnknownWindowsError(ket::platform::WindowsErrorCode code)
	{
		return std::string("Unknown Windows error ") + std::to_string(code);
	}

	std::optional<std::wstring> Utf8ToWide(std::string_view text)
	{
		const auto text_too_large =
			text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)());
		if (text_too_large)
		{
			return std::nullopt;
		}

		const auto text_size = static_cast<int>(text.size());
		const auto required_size = ::MultiByteToWideChar(
			CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), text_size, nullptr, 0);
		const auto conversion_failed = required_size <= 0;
		if (conversion_failed)
		{
			return std::nullopt;
		}

		std::wstring result(static_cast<std::size_t>(required_size), L'\0');
		const auto converted_size = ::MultiByteToWideChar(
			CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), text_size, &result[0], required_size);
		const auto converted_all = converted_size == required_size;
		if (!converted_all)
		{
			return std::nullopt;
		}

		return result;
	}

	std::optional<std::string> WideToUtf8(std::wstring_view text)
	{
		const auto text_is_empty = text.empty();
		if (text_is_empty)
		{
			return std::string();
		}

		const auto text_too_large =
			text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)());
		if (text_too_large)
		{
			return std::nullopt;
		}

		const auto text_size = static_cast<int>(text.size());
		const auto required_size = ::WideCharToMultiByte(
			CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), text_size, nullptr, 0, nullptr, nullptr);
		const auto conversion_failed = required_size <= 0;
		if (conversion_failed)
		{
			return std::nullopt;
		}

		std::string result(static_cast<std::size_t>(required_size), '\0');
		const auto converted_size = ::WideCharToMultiByte(CP_UTF8,
														  WC_ERR_INVALID_CHARS,
														  text.data(),
														  text_size,
														  &result[0],
														  required_size,
														  nullptr,
														  nullptr);
		const auto converted_all = converted_size == required_size;
		if (!converted_all)
		{
			return std::nullopt;
		}

		return result;
	}

	class LocalMemory
	{
	  public:
		explicit LocalMemory(HLOCAL handle) noexcept : handle_(handle) {}

		LocalMemory(const LocalMemory&) = delete;
		LocalMemory& operator=(const LocalMemory&) = delete;

		~LocalMemory() noexcept
		{
			const auto has_handle = handle_ != nullptr;
			if (has_handle)
			{
				static_cast<void>(::LocalFree(handle_));
			}
		}

	  private:
		HLOCAL handle_ = nullptr;
	};
#else
	[[maybe_unused]] std::string
	FormatStrerrorResult(int result, const char* buffer, int error_number)
	{
		const auto succeeded = result == 0;
		if (!succeeded)
		{
			return FormatUnknownErrno(error_number);
		}

		return CopyErrnoMessageOrFallback(buffer, error_number);
	}

	[[maybe_unused]] std::string
	FormatStrerrorResult(const char* message, const char* buffer, int error_number)
	{
		const char* const selected_message = message != nullptr ? message : buffer;
		return CopyErrnoMessageOrFallback(selected_message, error_number);
	}
#endif

} // namespace

namespace ket
{
	namespace platform
	{
		std::string FormatErrno(int error_number)
		{
			std::array<char, kErrnoMessageBufferSize> buffer{};

#ifdef _WIN32
			const auto result = strerror_s(buffer.data(), buffer.size(), error_number);
			const auto succeeded = result == 0;
			if (!succeeded)
			{
				return FormatUnknownErrno(error_number);
			}

			return CopyErrnoMessageOrFallback(buffer.data(), error_number);
#else
			return FormatStrerrorResult(strerror_r(error_number, buffer.data(), buffer.size()),
										buffer.data(),
										error_number);
#endif
		}

		std::optional<std::string> GetEnvironmentVariable(std::string_view name)
		{
			const auto name_is_valid = EnvironmentVariableNameIsValid(name);
			if (!name_is_valid)
			{
				return std::nullopt;
			}

#ifdef _WIN32
			const auto wide_name = Utf8ToWide(name);
			const auto has_wide_name = wide_name.has_value();
			if (!has_wide_name)
			{
				return std::nullopt;
			}

			DWORD capacity = ::GetEnvironmentVariableW(wide_name->c_str(), nullptr, 0U);
			const auto initial_lookup_failed = capacity == 0U;
			if (initial_lookup_failed)
			{
				return std::nullopt;
			}

			for (int attempt = 0; attempt < 3; ++attempt)
			{
				std::wstring wide_value(static_cast<std::size_t>(capacity), L'\0');
				::SetLastError(ERROR_SUCCESS);
				const auto copied_size =
					::GetEnvironmentVariableW(wide_name->c_str(), &wide_value[0], capacity);
				const auto lookup_failed = copied_size == 0U;
				if (lookup_failed)
				{
					const auto last_error = ::GetLastError();
					const auto empty_value = last_error == ERROR_SUCCESS;
					if (!empty_value)
					{
						return std::nullopt;
					}

					wide_value.clear();
					return WideToUtf8(wide_value);
				}

				const auto buffer_was_too_small = copied_size >= capacity;
				if (buffer_was_too_small)
				{
					capacity = copied_size + 1U;
					continue;
				}

				wide_value.resize(static_cast<std::size_t>(copied_size));
				return WideToUtf8(wide_value);
			}

			return std::nullopt;
#else
			const auto name_string = std::string(name);
			const char* const value = std::getenv(name_string.c_str());
			const auto value_is_missing = value == nullptr;
			if (value_is_missing)
			{
				return std::nullopt;
			}

			return std::string(value);
#endif
		}

#ifdef _WIN32
		WindowsErrorCode GetLastErrorCode() noexcept
		{
			return static_cast<WindowsErrorCode>(::GetLastError());
		}

		std::string FormatWindowsError(WindowsErrorCode code)
		{
			wchar_t* message = nullptr;
			constexpr DWORD kFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
			const auto message_length = ::FormatMessageW(kFormatFlags,
														 nullptr,
														 static_cast<DWORD>(code),
														 0U,
														 reinterpret_cast<LPWSTR>(&message),
														 0U,
														 nullptr);
			const auto has_message = message_length > 0U && message != nullptr;
			if (!has_message)
			{
				return FormatUnknownWindowsError(code);
			}

			const auto local_memory = LocalMemory(static_cast<HLOCAL>(message));
			static_cast<void>(local_memory);

			const auto converted = WideToUtf8(std::wstring_view(message, message_length));
			const auto converted_has_value = converted.has_value();
			if (!converted_has_value)
			{
				return FormatUnknownWindowsError(code);
			}

			const auto converted_is_empty = converted->empty();
			if (converted_is_empty)
			{
				return FormatUnknownWindowsError(code);
			}

			return *converted;
		}
#endif

	} // namespace platform

} // namespace ket
