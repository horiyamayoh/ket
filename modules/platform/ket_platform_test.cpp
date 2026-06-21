#include "ket_platform.h"

#include <cerrno>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)
#endif

namespace
{
	bool SetEnvironmentVariableForTest(const char* name, const std::string& value)
	{
#ifdef _WIN32
		const auto succeeded = ::SetEnvironmentVariableA(name, value.c_str()) != 0;
		return succeeded;
#else
		const auto result = ::setenv(name, value.c_str(), 1);
		return result == 0;
#endif
	}

	bool UnsetEnvironmentVariableForTest(const char* name)
	{
#ifdef _WIN32
		const auto succeeded = ::SetEnvironmentVariableA(name, nullptr) != 0;
		return succeeded;
#else
		const auto result = ::unsetenv(name);
		return result == 0;
#endif
	}

	class EnvironmentVariableRestorer
	{
	  public:
		explicit EnvironmentVariableRestorer(const char* name)
			: name_(name), original_(ket::platform::ReadEnvironmentVariable(name))
		{
		}

		EnvironmentVariableRestorer(const EnvironmentVariableRestorer&) = delete;
		EnvironmentVariableRestorer& operator=(const EnvironmentVariableRestorer&) = delete;

		~EnvironmentVariableRestorer()
		{
			const auto original_has_value = original_.has_value();
			if (original_has_value)
			{
				const auto restored = SetEnvironmentVariableForTest(name_, original_.value());
				static_cast<void>(restored);
				return;
			}

			const auto removed = UnsetEnvironmentVariableForTest(name_);
			static_cast<void>(removed);
		}

	  private:
		const char* name_ = nullptr;
		std::optional<std::string> original_;
	};

	std::string UnknownErrnoMessage(int error_number)
	{
		return std::string("Unknown error ") + std::to_string(error_number);
	}

#ifdef _WIN32
	std::string UnknownWindowsErrorMessage(ket::platform::WindowsErrorCode code)
	{
		return std::string("Unknown Windows error ") + std::to_string(code);
	}

	std::optional<std::wstring> ReadWideEnvironmentVariableForTest(const wchar_t* name)
	{
		::SetLastError(ERROR_SUCCESS);
		const auto capacity = ::GetEnvironmentVariableW(name, nullptr, 0U);
		const auto initial_lookup_failed = capacity == 0U;
		if (initial_lookup_failed)
		{
			const auto last_error = ::GetLastError();
			const auto empty_value = last_error == ERROR_SUCCESS;
			if (empty_value)
			{
				return std::wstring();
			}

			return std::nullopt;
		}

		std::wstring value(static_cast<std::size_t>(capacity), L'\0');
		::SetLastError(ERROR_SUCCESS);
		const auto copied_size = ::GetEnvironmentVariableW(name, &value[0], capacity);
		const auto lookup_failed = copied_size == 0U;
		if (lookup_failed)
		{
			const auto last_error = ::GetLastError();
			const auto empty_value = last_error == ERROR_SUCCESS;
			if (empty_value)
			{
				return std::wstring();
			}

			return std::nullopt;
		}

		value.resize(static_cast<std::size_t>(copied_size));
		return value;
	}

	class WideEnvironmentVariableRestorer
	{
	  public:
		explicit WideEnvironmentVariableRestorer(const wchar_t* name)
			: name_(name), original_(ReadWideEnvironmentVariableForTest(name))
		{
		}

		WideEnvironmentVariableRestorer(const WideEnvironmentVariableRestorer&) = delete;
		WideEnvironmentVariableRestorer& operator=(const WideEnvironmentVariableRestorer&) = delete;

		~WideEnvironmentVariableRestorer()
		{
			const auto original_has_value = original_.has_value();
			if (original_has_value)
			{
				const auto restored =
					::SetEnvironmentVariableW(name_, original_.value().c_str()) != 0;
				static_cast<void>(restored);
				return;
			}

			const auto removed = ::SetEnvironmentVariableW(name_, nullptr) != 0;
			static_cast<void>(removed);
		}

	  private:
		const wchar_t* name_ = nullptr;
		std::optional<std::wstring> original_;
	};
#endif

} // namespace

/**
 * @test
 * @brief 既知errno番号のmessage取得確認。
 * @details EINVALを入力し、platformから非空かつfallbackでないmessageが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPlatformTest, FormatsKnownErrno)
{
	const auto message = ket::platform::FormatErrno(EINVAL);
	const auto message_is_empty = message.empty();
	const auto fallback = UnknownErrnoMessage(EINVAL);

	EXPECT_FALSE(message_is_empty);
	EXPECT_NE(message, fallback);
}

/**
 * @test
 * @brief 未知errno番号のfallback確認。
 * @details 大きいerrno番号を入力し、ASCII fallback形式のmessageが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPlatformTest, FormatsUnknownErrnoWithFallback)
{
	constexpr int kUnknownError = 123456789;

	const auto message = ket::platform::FormatErrno(kUnknownError);
	const auto message_is_empty = message.empty();
	const auto fallback = UnknownErrnoMessage(kUnknownError);
	const auto message_uses_fallback_prefix = message.rfind("Unknown error ", 0U) == 0U;

	EXPECT_FALSE(message_is_empty);
	EXPECT_EQ(message, fallback);
	EXPECT_TRUE(message_uses_fallback_prefix);
}

/**
 * @test
 * @brief errno番号のmessage取得時のerrno保存確認。
 * @details FormatErrno呼び出し前のerrno値が呼び出し後も保持されることを確認。
 * @pre C++17以降。
 * @post errnoはテスト中に設定した値を保持。
 */
TEST(KetPlatformTest, FormatErrnoPreservesErrno)
{
	errno = ERANGE;

	const auto message = ket::platform::FormatErrno(EINVAL);
	const auto message_is_empty = message.empty();
	const auto current_errno = errno;

	EXPECT_FALSE(message_is_empty);
	EXPECT_EQ(current_errno, ERANGE);
}

/**
 * @test
 * @brief missing environment variableの取得失敗確認。
 * @details 一意なKET_ prefixの変数名を削除してから取得し、std::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post 対象environment variableはテスト前の状態へ復元。
 */
TEST(KetPlatformTest, ReturnsNulloptForMissingEnvironmentVariable)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_MISSING_VALUE";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto removed = UnsetEnvironmentVariableForTest(kName);
	static_cast<void>(removed);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

/**
 * @test
 * @brief present environment variableの値取得確認。
 * @details 一意なKET_ prefixの変数へ値を設定し、同じ値がstd::stringで返ることを確認。
 * @pre C++17以降。
 * @post 対象environment variableはテスト前の状態へ復元。
 */
TEST(KetPlatformTest, ReadsPresentEnvironmentVariable)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_PRESENT_VALUE";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto set_succeeded = SetEnvironmentVariableForTest(kName, "platform-value");

	ASSERT_TRUE(set_succeeded);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto expected = std::optional<std::string>("platform-value");

	EXPECT_EQ(value, expected);
}

/**
 * @test
 * @brief 空値を持つpresent environment variableの取得確認。
 * @details 一意なKET_
 * prefixの変数へ空文字列を設定し、missingではなく空のstd::stringを返すことを確認。
 * @pre C++17以降。
 * @post 対象environment variableはテスト前の状態へ復元。
 */
TEST(KetPlatformTest, ReadsPresentEmptyEnvironmentVariable)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_EMPTY_VALUE";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto set_succeeded = SetEnvironmentVariableForTest(kName, "");

	ASSERT_TRUE(set_succeeded);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto expected = std::optional<std::string>("");

	EXPECT_EQ(value, expected);
}

/**
 * @test
 * @brief restorerがpresent environment variableを復元することの確認。
 * @details 元値を持つ一意なKET_ prefixの変数をscope内で変更し、scope終了後に元値へ戻ることを確認。
 * @pre C++17以降。
 * @post 対象environment variableはテスト開始時と同じ値。
 */
TEST(KetPlatformTest, RestoresPresentEnvironmentVariableAfterScope)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_RESTORE_PRESENT";
	const auto outer_restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(outer_restorer);
	const auto set_succeeded = SetEnvironmentVariableForTest(kName, "original-value");

	ASSERT_TRUE(set_succeeded);

	{
		const auto inner_restorer = EnvironmentVariableRestorer(kName);
		static_cast<void>(inner_restorer);
		const auto changed = SetEnvironmentVariableForTest(kName, "changed-value");

		ASSERT_TRUE(changed);
	}

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto expected = std::optional<std::string>("original-value");

	EXPECT_EQ(value, expected);
}

/**
 * @test
 * @brief restorerがmissing environment variableを削除状態へ戻すことの確認。
 * @details 元々missingの一意なKET_
 * prefixの変数をscope内で作成し、scope終了後にstd::nulloptへ戻ることを確認。
 * @pre C++17以降。
 * @post 対象environment variableはテスト開始時の状態へ復元。
 */
TEST(KetPlatformTest, RestoresMissingEnvironmentVariableAfterScope)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_RESTORE_MISSING";
	const auto outer_restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(outer_restorer);
	const auto removed_before_test = UnsetEnvironmentVariableForTest(kName);
	static_cast<void>(removed_before_test);

	{
		const auto restorer = EnvironmentVariableRestorer(kName);
		static_cast<void>(restorer);
		const auto set_succeeded = SetEnvironmentVariableForTest(kName, "temporary-value");

		ASSERT_TRUE(set_succeeded);
	}

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

/**
 * @test
 * @brief 空environment variable名の拒否確認。
 * @details 空nameを入力し、platform APIを呼ばずstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post process environmentの変更なし。
 */
TEST(KetPlatformTest, RejectsEmptyEnvironmentVariableName)
{
	const auto value = ket::platform::ReadEnvironmentVariable(std::string_view());
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

/**
 * @test
 * @brief NULを含むenvironment variable名の拒否確認。
 * @details NULを含むnameを入力し、platform APIへ渡さずstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post process environmentの変更なし。
 */
TEST(KetPlatformTest, RejectsEnvironmentVariableNameContainingNul)
{
	const auto name =
		std::string("KET_PLATFORM_TEST_NUL\0NAME", sizeof("KET_PLATFORM_TEST_NUL\0NAME") - 1U);

	const auto value =
		ket::platform::ReadEnvironmentVariable(std::string_view(name.data(), name.size()));
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

#ifndef _WIN32
/**
 * @test
 * @brief 非Windows環境のconditional compile確認。
 * @details 非Windows環境でplatform moduleをincludeし、Windows専用APIなしでcompileされることを確認。
 * @pre C++17以降かつ非Windows環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPlatformTest, CompilesWithoutWindowsOnlyApiOnNonWindows)
{
	SUCCEED();
}
#endif

#ifdef _WIN32
/**
 * @test
 * @brief Windows last-error codeとmessage取得確認。
 * @details 既知のWindows error codeをlast-errorに設定し、code取得と非空messageへの変換を確認。
 * @pre C++17以降かつWindows環境。
 * @post 現在threadのlast-error code以外の外部状態の変更なし。
 */
TEST(KetPlatformTest, ReadsAndFormatsWindowsLastError)
{
	::SetLastError(ERROR_FILE_NOT_FOUND);

	const auto code = ket::platform::GetLastErrorCode();
	const auto expected = static_cast<ket::platform::WindowsErrorCode>(ERROR_FILE_NOT_FOUND);
	const auto message = ket::platform::FormatWindowsError(code);
	const auto message_is_empty = message.empty();
	const auto fallback = UnknownWindowsErrorMessage(code);

	EXPECT_EQ(code, expected);
	EXPECT_FALSE(message_is_empty);
	EXPECT_NE(message, fallback);
}

/**
 * @test
 * @brief Windows UTF-8 environment variable取得確認。
 * @details Windows wide
 * APIで非ASCIIのname/valueを設定し、UTF-8文字列で同じ値を取得できることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 対象environment variableはテスト前の状態へ復元。
 */
TEST(KetPlatformTest, ReadsUtf8EnvironmentVariableOnWindows)
{
	constexpr const wchar_t* kWideName = L"KET_PLATFORM_TEST_UTF8_\u30e6";
	constexpr const wchar_t* kWideValue = L"value-\u30c6\u30b9\u30c8";
	const auto restorer = WideEnvironmentVariableRestorer(kWideName);
	static_cast<void>(restorer);
	const auto set_succeeded = ::SetEnvironmentVariableW(kWideName, kWideValue) != 0;

	ASSERT_TRUE(set_succeeded);

	const auto name = std::string("KET_PLATFORM_TEST_UTF8_") + "\xe3\x83\xa6";
	const auto value = ket::platform::ReadEnvironmentVariable(name);
	const auto expected =
		std::optional<std::string>(std::string("value-") + "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88");

	EXPECT_EQ(value, expected);
}

/**
 * @test
 * @brief Windows UTF-8 environment variable名の変換失敗確認。
 * @details invalid UTF-8 byte列を含むnameを入力し、wide APIへ渡さずstd::nulloptを返すことを確認。
 * @pre C++17以降かつWindows環境。
 * @post process environmentの変更なし。
 */
TEST(KetPlatformTest, RejectsInvalidUtf8EnvironmentVariableNameOnWindows)
{
	std::string name = "KET_PLATFORM_TEST_INVALID_UTF8_";
	name.push_back(static_cast<char>(0xffU));

	const auto value = ket::platform::ReadEnvironmentVariable(name);
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

/**
 * @test
 * @brief Windows environment variable値のUTF-8変換失敗確認。
 * @details UTF-8へ変換できないwide valueを設定し、std::nulloptを返すことを確認。
 * @pre C++17以降かつWindows環境。
 * @post 対象environment variableはテスト前の状態へ復元。
 */
TEST(KetPlatformTest, ReturnsNulloptForInvalidUtf16EnvironmentVariableValueOnWindows)
{
	constexpr const wchar_t* kWideName = L"KET_PLATFORM_TEST_INVALID_UTF16_VALUE";
	const auto restorer = WideEnvironmentVariableRestorer(kWideName);
	static_cast<void>(restorer);
	const std::wstring invalid_value(1U, static_cast<wchar_t>(0xd800U));
	const auto set_succeeded = ::SetEnvironmentVariableW(kWideName, invalid_value.c_str()) != 0;

	ASSERT_TRUE(set_succeeded);

	const auto value =
		ket::platform::ReadEnvironmentVariable("KET_PLATFORM_TEST_INVALID_UTF16_VALUE");
	const auto value_has_value = value.has_value();

	EXPECT_FALSE(value_has_value);
}

/**
 * @test
 * @brief 未知Windows error codeのfallback確認。
 * @details 未割り当て想定のWindows error codeを入力し、ASCII fallback文字列が返ることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 現在threadのlast-error codeは変更しない。
 */
TEST(KetPlatformTest, FormatsUnknownWindowsErrorWithFallback)
{
	constexpr auto kUnknownError = static_cast<ket::platform::WindowsErrorCode>(0xFFFFFFFFUL);

	::SetLastError(ERROR_ACCESS_DENIED);

	const auto message = ket::platform::FormatWindowsError(kUnknownError);
	const auto expected = UnknownWindowsErrorMessage(kUnknownError);
	const auto last_error = ::GetLastError();

	EXPECT_EQ(message, expected);
	EXPECT_EQ(last_error, static_cast<DWORD>(ERROR_ACCESS_DENIED));
}

/**
 * @test
 * @brief Windows present environment variable取得時のlast-error保存確認。
 * @details present environment variable読取前のlast-error codeが読取後も保持されることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 対象environment variableはテスト前の状態へ復元し、last-error codeは元値を保持。
 */
TEST(KetPlatformTest, ReadEnvironmentVariablePreservesWindowsLastError)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_LAST_ERROR_ENV";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto set_succeeded = SetEnvironmentVariableForTest(kName, "platform-value");

	ASSERT_TRUE(set_succeeded);

	::SetLastError(ERROR_ACCESS_DENIED);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto last_error = ::GetLastError();
	const auto expected = std::optional<std::string>("platform-value");

	EXPECT_EQ(value, expected);
	EXPECT_EQ(last_error, static_cast<DWORD>(ERROR_ACCESS_DENIED));
}

/**
 * @test
 * @brief Windows missing environment variable取得時のlast-error保存確認。
 * @details missing environment variable読取前のlast-error codeが読取後も保持されることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 対象environment variableはテスト前の状態へ復元し、last-error codeは元値を保持。
 */
TEST(KetPlatformTest, ReadMissingEnvironmentVariablePreservesWindowsLastError)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_LAST_ERROR_MISSING_ENV";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto removed = UnsetEnvironmentVariableForTest(kName);
	static_cast<void>(removed);

	::SetLastError(ERROR_ACCESS_DENIED);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto value_has_value = value.has_value();
	const auto last_error = ::GetLastError();

	EXPECT_FALSE(value_has_value);
	EXPECT_EQ(last_error, static_cast<DWORD>(ERROR_ACCESS_DENIED));
}

/**
 * @test
 * @brief Windows empty environment variable取得時のlast-error保存確認。
 * @details 空値を持つenvironment variable読取前のlast-error codeが読取後も保持されることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 対象environment variableはテスト前の状態へ復元し、last-error codeは元値を保持。
 */
TEST(KetPlatformTest, ReadEmptyEnvironmentVariablePreservesWindowsLastError)
{
	constexpr const char* kName = "KET_PLATFORM_TEST_LAST_ERROR_EMPTY_ENV";
	const auto restorer = EnvironmentVariableRestorer(kName);
	static_cast<void>(restorer);
	const auto set_succeeded = SetEnvironmentVariableForTest(kName, "");

	ASSERT_TRUE(set_succeeded);

	::SetLastError(ERROR_ACCESS_DENIED);

	const auto value = ket::platform::ReadEnvironmentVariable(kName);
	const auto last_error = ::GetLastError();
	const auto expected = std::optional<std::string>("");

	EXPECT_EQ(value, expected);
	EXPECT_EQ(last_error, static_cast<DWORD>(ERROR_ACCESS_DENIED));
}

/**
 * @test
 * @brief Windows error message文字列化時のlast-error保存確認。
 * @details Windows error codeの文字列化前のlast-error codeが文字列化後も保持されることを確認。
 * @pre C++17以降かつWindows環境。
 * @post 現在threadのlast-error codeは元値を保持。
 */
TEST(KetPlatformTest, FormatWindowsErrorPreservesWindowsLastError)
{
	::SetLastError(ERROR_ACCESS_DENIED);

	const auto message = ket::platform::FormatWindowsError(ERROR_FILE_NOT_FOUND);
	const auto message_is_empty = message.empty();
	const auto last_error = ::GetLastError();

	EXPECT_FALSE(message_is_empty);
	EXPECT_EQ(last_error, static_cast<DWORD>(ERROR_ACCESS_DENIED));
}
#endif
