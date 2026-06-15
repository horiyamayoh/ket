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
			: name_(name), original_(ket::platform::GetEnvironmentVariable(name))
		{
		}

		EnvironmentVariableRestorer(const EnvironmentVariableRestorer&) = delete;
		EnvironmentVariableRestorer& operator=(const EnvironmentVariableRestorer&) = delete;

		~EnvironmentVariableRestorer()
		{
			const auto original_has_value = original_.has_value();
			if (original_has_value)
			{
				const auto restored = SetEnvironmentVariableForTest(name_, *original_);
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

} // namespace

/**
 * @test
 * @brief 既知errno番号のmessage取得確認。
 * @details EINVALを入力し、platformから非空messageが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPlatformTest, FormatsKnownErrno)
{
	const auto message = ket::platform::FormatErrno(EINVAL);
	const auto message_is_empty = message.empty();

	EXPECT_FALSE(message_is_empty);
}

/**
 * @test
 * @brief 未知errno番号のfallback確認。
 * @details 大きいerrno番号を入力し、非空かつ番号を含むmessageが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetPlatformTest, FormatsUnknownErrnoWithFallback)
{
	constexpr int kUnknownError = 123456789;

	const auto message = ket::platform::FormatErrno(kUnknownError);
	const auto message_is_empty = message.empty();
	const auto number_position = message.find("123456789");
	const auto message_contains_number = number_position != std::string::npos;

	EXPECT_FALSE(message_is_empty);
	EXPECT_TRUE(message_contains_number);
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

	const auto value = ket::platform::GetEnvironmentVariable(kName);
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

	const auto value = ket::platform::GetEnvironmentVariable(kName);
	const auto expected = std::optional<std::string>("platform-value");

	EXPECT_EQ(value, expected);
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
	const auto value = ket::platform::GetEnvironmentVariable(std::string_view());
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
		ket::platform::GetEnvironmentVariable(std::string_view(name.data(), name.size()));
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

	EXPECT_EQ(code, expected);
	EXPECT_FALSE(message_is_empty);
}
#endif
