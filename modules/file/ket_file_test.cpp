#include "ket_file.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios> // IWYU pragma: keep
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	std::filesystem::path MakeTemporaryPath(std::string_view name)
	{
		std::error_code error;
		const auto base_path = std::filesystem::temp_directory_path(error);
		const auto query_failed = static_cast<bool>(error);
		if (query_failed)
		{
			throw std::runtime_error("failed to get temporary directory");
		}

		const auto directory_name = std::string("ket_file_test_") + std::string(name);
		return base_path / directory_name;
	}

	class TemporaryDirectory
	{
	  public:
		explicit TemporaryDirectory(std::string_view name) : path_(MakeTemporaryPath(name))
		{
			std::error_code error;
			const auto removed = std::filesystem::remove_all(path_, error);
			static_cast<void>(removed);
			const auto cleanup_failed = static_cast<bool>(error);
			if (cleanup_failed)
			{
				throw std::runtime_error("failed to clean temporary directory");
			}

			const auto created = std::filesystem::create_directories(path_, error);
			static_cast<void>(created);
			const auto create_failed = static_cast<bool>(error);
			if (create_failed)
			{
				throw std::runtime_error("failed to create temporary directory");
			}
		}

		~TemporaryDirectory() noexcept
		{
			std::error_code error;
			const auto removed = std::filesystem::remove_all(path_, error);
			static_cast<void>(removed);
		}

		[[nodiscard]] const std::filesystem::path& path() const noexcept
		{
			return path_;
		}

	  private:
		std::filesystem::path path_;
	};

	void CreateEmptyFileForTest(const std::filesystem::path& path)
	{
		std::ofstream stream(path, std::ios::binary);
		const auto stream_is_open = stream.is_open();
		ASSERT_TRUE(stream_is_open);

		stream.close();
		const auto close_succeeded = static_cast<bool>(stream);
		ASSERT_TRUE(close_succeeded);
	}

	void WriteStringForTest(const std::filesystem::path& path, std::string_view text)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		const auto stream_is_open = stream.is_open();
		ASSERT_TRUE(stream_is_open);

		const auto text_is_empty = text.empty();
		if (!text_is_empty)
		{
			stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		}

		stream.close();
		const auto write_succeeded = static_cast<bool>(stream);
		ASSERT_TRUE(write_succeeded);
	}

	void WriteBytesForTest(const std::filesystem::path& path,
						   const std::vector<std::uint8_t>& bytes)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		const auto stream_is_open = stream.is_open();
		ASSERT_TRUE(stream_is_open);

		const auto bytes_are_empty = bytes.empty();
		if (!bytes_are_empty)
		{
			const auto* const data = reinterpret_cast<const char*>(bytes.data());
			stream.write(data, static_cast<std::streamsize>(bytes.size()));
		}

		stream.close();
		const auto write_succeeded = static_cast<bool>(stream);
		ASSERT_TRUE(write_succeeded);
	}

} // namespace

/**
 * @test
 * @brief 空fileのtext読み込みとquery確認。
 * @details 空fileを作成し、TryReadAllTextが空文字列を返し、Sizeが0を返すことを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReadsEmptyTextFileAndQueriesIt)
{
	const TemporaryDirectory temporary("empty_text");
	const auto path = temporary.path() / "empty.txt";
	CreateEmptyFileForTest(path);

	std::string text = "unchanged";
	std::error_code error = std::make_error_code(std::errc::permission_denied);

	const auto read_succeeded = ket::file::TryReadAllText(path, text, &error);
	const auto exists = ket::file::Exists(path);
	const auto is_directory = ket::file::IsDirectory(path);
	const auto size = ket::file::Size(path);
	const auto error_is_set = static_cast<bool>(error);

	EXPECT_TRUE(read_succeeded);
	EXPECT_EQ(text, std::string());
	EXPECT_FALSE(error_is_set);
	EXPECT_TRUE(exists);
	EXPECT_FALSE(is_directory);
	EXPECT_EQ(size, std::optional<std::uintmax_t>(std::uintmax_t{0U}));
}

/**
 * @test
 * @brief text読み込みのbyte保持確認。
 * @details CRLFとNULを含む内容をbinary
 * modeで作成し、TryReadAllTextが変換なしで同じbytesを返すことを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReadsTextBytesWithoutConversion)
{
	const TemporaryDirectory temporary("text_bytes");
	const auto path = temporary.path() / "text.txt";
	const auto expected = std::string("alpha\r\nbravo\0charlie", 20U);
	WriteStringForTest(path, expected);

	std::string text;
	const auto read_succeeded = ket::file::TryReadAllText(path, text);
	const auto text_size = text.size();

	EXPECT_TRUE(read_succeeded);
	EXPECT_EQ(text, expected);
	EXPECT_EQ(text_size, 20U);
}

/**
 * @test
 * @brief binary fileのbytes読み込み確認。
 * @details 0x00と0xFFを含むbytes列を作成し、TryReadAllBytesが同じ順序と値を返すことを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReadsBinaryBytes)
{
	const TemporaryDirectory temporary("binary_bytes");
	const auto path = temporary.path() / "payload.bin";
	const auto expected = std::vector<std::uint8_t>{std::uint8_t{0x00U},
													std::uint8_t{0x01U},
													std::uint8_t{0x7FU},
													std::uint8_t{0x80U},
													std::uint8_t{0xFFU}};
	WriteBytesForTest(path, expected);

	std::vector<std::uint8_t> bytes = {std::uint8_t{0xAAU}};
	std::error_code error;

	const auto read_succeeded = ket::file::TryReadAllBytes(path, bytes, &error);
	const auto error_is_set = static_cast<bool>(error);

	EXPECT_TRUE(read_succeeded);
	EXPECT_EQ(bytes, expected);
	EXPECT_FALSE(error_is_set);
}

/**
 * @test
 * @brief missing file読み込みの失敗と出力不変確認。
 * @details 存在しないpathを読み込み、falseを返し、出力を変更せずerror_codeを設定することを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post 読み込み対象pathは作成されず、temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, PreservesOutputsAndSetsErrorForMissingReads)
{
	const TemporaryDirectory temporary("missing_reads");
	const auto path = temporary.path() / "missing.txt";

	std::string text = "keep";
	std::vector<std::uint8_t> bytes = {std::uint8_t{0xAAU}};
	const auto expected_bytes = bytes;
	std::error_code error;

	const auto text_read = ket::file::TryReadAllText(path, text, &error);
	const auto text_error_is_set = static_cast<bool>(error);
	const auto bytes_read = ket::file::TryReadAllBytes(path, bytes, nullptr);
	const auto exists = ket::file::Exists(path);
	const auto size = ket::file::Size(path);

	EXPECT_FALSE(text_read);
	EXPECT_TRUE(text_error_is_set);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(bytes, expected_bytes);
	EXPECT_FALSE(exists);
	EXPECT_EQ(size, std::nullopt);
}

/**
 * @test
 * @brief directory path読み込みとSizeの失敗確認。
 * @details directory
 * pathを読み込み対象に指定し、falseを返し、出力を変更せずSizeがstd::nulloptを返すことを確認。
 * @pre C++17以降。temporary directory作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, RejectsDirectoryReadsAndReportsDirectoryQueries)
{
	const TemporaryDirectory temporary("directory_reads");
	const auto& path = temporary.path();

	std::string text = "keep";
	std::vector<std::uint8_t> bytes = {std::uint8_t{0xAAU}};
	std::error_code text_error;
	std::error_code bytes_error;

	const auto text_read = ket::file::TryReadAllText(path, text, &text_error);
	const auto bytes_read = ket::file::TryReadAllBytes(path, bytes, &bytes_error);
	const auto exists = ket::file::Exists(path);
	const auto is_directory = ket::file::IsDirectory(path);
	const auto size = ket::file::Size(path);
	const auto text_error_is_set = static_cast<bool>(text_error);
	const auto bytes_error_is_set = static_cast<bool>(bytes_error);

	EXPECT_FALSE(text_read);
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_EQ(bytes, std::vector<std::uint8_t>({std::uint8_t{0xAAU}}));
	EXPECT_TRUE(text_error_is_set);
	EXPECT_TRUE(bytes_error_is_set);
	EXPECT_TRUE(exists);
	EXPECT_TRUE(is_directory);
	EXPECT_EQ(size, std::nullopt);
}

/**
 * @test
 * @brief textとbytesの全体書き込み確認。
 * @details TryWriteAllTextとTryWriteAllBytesでfileを作成し、読み込みAPIで同じ内容が戻ることを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, WritesTextAndBytes)
{
	const TemporaryDirectory temporary("write_all");
	const auto text_path = temporary.path() / "text.txt";
	const auto bytes_path = temporary.path() / "payload.bin";
	const auto expected_text = std::string("line1\nline2");
	const auto expected_bytes = std::array<std::uint8_t, 4>{
		{std::uint8_t{0x00U}, std::uint8_t{0x7FU}, std::uint8_t{0x80U}, std::uint8_t{0xFFU}}};
	std::error_code text_error;
	std::error_code bytes_error;

	const auto text_written = ket::file::TryWriteAllText(text_path, expected_text, &text_error);
	const auto bytes_written = ket::file::TryWriteAllBytes(
		bytes_path, expected_bytes.data(), expected_bytes.size(), &bytes_error);

	std::string actual_text;
	std::vector<std::uint8_t> actual_bytes;
	const auto text_read = ket::file::TryReadAllText(text_path, actual_text);
	const auto bytes_read = ket::file::TryReadAllBytes(bytes_path, actual_bytes);
	const auto text_error_is_set = static_cast<bool>(text_error);
	const auto bytes_error_is_set = static_cast<bool>(bytes_error);
	const auto expected_byte_vector =
		std::vector<std::uint8_t>(expected_bytes.begin(), expected_bytes.end());

	EXPECT_TRUE(text_written);
	EXPECT_TRUE(bytes_written);
	EXPECT_FALSE(text_error_is_set);
	EXPECT_FALSE(bytes_error_is_set);
	EXPECT_TRUE(text_read);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(actual_text, expected_text);
	EXPECT_EQ(actual_bytes, expected_byte_vector);
}

/**
 * @test
 * @brief 空bytesのnullptr書き込み確認。
 * @details dataがnullptrかつsizeが0の入力を空fileとして書き込み、Sizeが0を返すことを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, WritesEmptyBytesFromNullPointer)
{
	const TemporaryDirectory temporary("empty_bytes");
	const auto path = temporary.path() / "empty.bin";
	std::error_code error = std::make_error_code(std::errc::permission_denied);

	const auto write_succeeded = ket::file::TryWriteAllBytes(path, nullptr, 0U, &error);
	const auto size = ket::file::Size(path);
	const auto error_is_set = static_cast<bool>(error);

	EXPECT_TRUE(write_succeeded);
	EXPECT_EQ(size, std::optional<std::uintmax_t>(std::uintmax_t{0U}));
	EXPECT_FALSE(error_is_set);
}

/**
 * @test
 * @brief bytes書き込みのnullptr不正入力確認。
 * @details
 * dataがnullptrかつsizeが1の入力を渡し、falseを返しerror_codeを設定し、fileを作らないことを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, RejectsNullBytesWithNonZeroSize)
{
	const TemporaryDirectory temporary("null_bytes");
	const auto path = temporary.path() / "invalid.bin";
	std::error_code error;

	const auto write_succeeded = ket::file::TryWriteAllBytes(path, nullptr, 1U, &error);
	const auto error_is_set = static_cast<bool>(error);
	const auto exists = ket::file::Exists(path);

	EXPECT_FALSE(write_succeeded);
	EXPECT_TRUE(error_is_set);
	EXPECT_FALSE(exists);
}

/**
 * @test
 * @brief directory path書き込み失敗確認。
 * @details directory pathを書き込み対象に指定し、falseを返しerror_codeを設定することを確認。
 * @pre C++17以降。temporary directory作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReportsWriteFailureForDirectoryPath)
{
	const TemporaryDirectory temporary("directory_writes");
	const auto& path = temporary.path();
	const auto bytes = std::array<std::uint8_t, 1>{{std::uint8_t{0x42U}}};
	std::error_code text_error;
	std::error_code bytes_error;

	const auto text_written = ket::file::TryWriteAllText(path, "x", &text_error);
	const auto bytes_written =
		ket::file::TryWriteAllBytes(path, bytes.data(), bytes.size(), &bytes_error);
	const auto text_error_is_set = static_cast<bool>(text_error);
	const auto bytes_error_is_set = static_cast<bool>(bytes_error);

	EXPECT_FALSE(text_written);
	EXPECT_FALSE(bytes_written);
	EXPECT_TRUE(text_error_is_set);
	EXPECT_TRUE(bytes_error_is_set);
}
