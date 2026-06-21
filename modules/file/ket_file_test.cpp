#include "ket_file.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios> // IWYU pragma: keep
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

namespace
{
	std::filesystem::path CreateTemporaryDirectoryForTest(std::string_view name)
	{
		std::error_code error;
		const auto base_path = std::filesystem::temp_directory_path(error);
		const auto query_failed = static_cast<bool>(error);
		if (query_failed)
		{
			throw std::runtime_error("failed to get temporary directory");
		}

		std::random_device seed_source;
		const auto seed = seed_source();
		for (int attempt = 0; attempt < 100; ++attempt)
		{
			const auto directory_name = std::string("ket_file_test_") + std::string(name) + "_" +
				std::to_string(seed) + "_" + std::to_string(attempt);
			const auto path = base_path / directory_name;
			error.clear();
			const auto created = std::filesystem::create_directory(path, error);
			if (created)
			{
				return path;
			}

			const auto create_failed = static_cast<bool>(error);
			if (create_failed)
			{
				throw std::runtime_error("failed to create temporary directory");
			}
		}

		throw std::runtime_error("failed to allocate unique temporary directory");
	}

	class TemporaryDirectory
	{
	  public:
		explicit TemporaryDirectory(std::string_view name)
			: path_(CreateTemporaryDirectoryForTest(name))
		{
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

	void ExpectBytesForTest(const std::filesystem::path& path,
							const std::vector<std::uint8_t>& expected)
	{
		std::vector<std::uint8_t> actual;
		const auto read_succeeded = ket::file::TryReadAllBytes(path, actual);

		EXPECT_TRUE(read_succeeded);
		EXPECT_EQ(actual, expected);
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
 * @brief 空fileのbytes読み込み確認。
 * @details
 * 空fileを読み込み、TryReadAllBytesが既存vectorを空vectorへ置換し、古いerror_codeをclearすることを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReadsEmptyBytesFileAndClearsError)
{
	const TemporaryDirectory temporary("empty_binary_read");
	const auto path = temporary.path() / "empty.bin";
	CreateEmptyFileForTest(path);

	std::vector<std::uint8_t> bytes = {std::uint8_t{0xAAU}};
	std::error_code error = std::make_error_code(std::errc::permission_denied);

	const auto read_succeeded = ket::file::TryReadAllBytes(path, bytes, &error);
	const auto error_is_set = static_cast<bool>(error);

	EXPECT_TRUE(read_succeeded);
	EXPECT_EQ(bytes, std::vector<std::uint8_t>());
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
	std::error_code text_error;
	std::error_code bytes_error;

	const auto text_read = ket::file::TryReadAllText(path, text, &text_error);
	const auto bytes_read = ket::file::TryReadAllBytes(path, bytes, &bytes_error);
	const auto exists = ket::file::Exists(path);
	const auto is_directory = ket::file::IsDirectory(path);
	const auto size = ket::file::Size(path);

	EXPECT_FALSE(text_read);
	EXPECT_EQ(text_error, std::make_error_code(std::errc::no_such_file_or_directory));
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(bytes_error, std::make_error_code(std::errc::no_such_file_or_directory));
	EXPECT_EQ(bytes, expected_bytes);
	EXPECT_FALSE(exists);
	EXPECT_FALSE(is_directory);
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

	EXPECT_FALSE(text_read);
	EXPECT_FALSE(bytes_read);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_EQ(bytes, std::vector<std::uint8_t>({std::uint8_t{0xAAU}}));
	EXPECT_EQ(text_error, std::make_error_code(std::errc::is_a_directory));
	EXPECT_EQ(bytes_error, std::make_error_code(std::errc::is_a_directory));
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
	std::error_code text_error = std::make_error_code(std::errc::permission_denied);
	std::error_code bytes_error = std::make_error_code(std::errc::permission_denied);

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
	const auto text_size = ket::file::Size(text_path);
	const auto bytes_size = ket::file::Size(bytes_path);

	EXPECT_TRUE(text_written);
	EXPECT_TRUE(bytes_written);
	EXPECT_FALSE(text_error_is_set);
	EXPECT_FALSE(bytes_error_is_set);
	EXPECT_TRUE(text_read);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(actual_text, expected_text);
	EXPECT_EQ(actual_bytes, expected_byte_vector);
	EXPECT_EQ(text_size, std::optional<std::uintmax_t>(std::uintmax_t{11U}));
	EXPECT_EQ(bytes_size, std::optional<std::uintmax_t>(std::uintmax_t{4U}));
}

/**
 * @test
 * @brief 既存fileのtruncate付き置換確認。
 * @details
 * 長い既存内容を短いtext/bytesで書き換え、旧末尾が残らずSizeも短い内容に一致することを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, WritesReplaceExistingFilesByTruncating)
{
	const TemporaryDirectory temporary("truncate_existing");
	const auto text_path = temporary.path() / "text.txt";
	const auto bytes_path = temporary.path() / "payload.bin";
	WriteStringForTest(text_path, "old-long-text");
	WriteBytesForTest(
		bytes_path,
		std::vector<std::uint8_t>{std::uint8_t{0x10U}, std::uint8_t{0x11U}, std::uint8_t{0x12U}});
	const auto new_bytes = std::array<std::uint8_t, 1>{{std::uint8_t{0xAAU}}};

	const auto text_written = ket::file::TryWriteAllText(text_path, "new");
	const auto bytes_written =
		ket::file::TryWriteAllBytes(bytes_path, new_bytes.data(), new_bytes.size());

	std::string actual_text;
	std::vector<std::uint8_t> actual_bytes;
	const auto text_read = ket::file::TryReadAllText(text_path, actual_text);
	const auto bytes_read = ket::file::TryReadAllBytes(bytes_path, actual_bytes);
	const auto text_size = ket::file::Size(text_path);
	const auto bytes_size = ket::file::Size(bytes_path);

	EXPECT_TRUE(text_written);
	EXPECT_TRUE(bytes_written);
	EXPECT_TRUE(text_read);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(actual_text, std::string("new"));
	EXPECT_EQ(actual_bytes, std::vector<std::uint8_t>({std::uint8_t{0xAAU}}));
	EXPECT_EQ(text_size, std::optional<std::uintmax_t>(std::uintmax_t{3U}));
	EXPECT_EQ(bytes_size, std::optional<std::uintmax_t>(std::uintmax_t{1U}));
}

/**
 * @test
 * @brief text書き込みのbyte保持確認。
 * @details NULを含むstd::string_viewを書き込み、encoding変換なしで同じbytesを読み戻せることを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, WritesTextBytesWithoutConversion)
{
	const TemporaryDirectory temporary("write_text_bytes");
	const auto path = temporary.path() / "text.bin";
	const auto expected = std::string("ab\0cd", 5U);

	const auto write_succeeded = ket::file::TryWriteAllText(path, expected);

	std::string actual;
	const auto read_succeeded = ket::file::TryReadAllText(path, actual);
	const auto size = ket::file::Size(path);

	EXPECT_TRUE(write_succeeded);
	EXPECT_TRUE(read_succeeded);
	EXPECT_EQ(actual, expected);
	EXPECT_EQ(size, std::optional<std::uintmax_t>(std::uintmax_t{5U}));
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
 * @brief 空textと非null空bytesのtruncate書き込み確認。
 * @details 既存内容を持つfileへ空textとsize 0の非null bytesを書き込み、0
 * byteへ置換されることを確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, WritesEmptyInputsByTruncatingExistingFiles)
{
	const TemporaryDirectory temporary("empty_inputs");
	const auto text_path = temporary.path() / "text.txt";
	const auto bytes_path = temporary.path() / "payload.bin";
	WriteStringForTest(text_path, "old");
	WriteBytesForTest(bytes_path, std::vector<std::uint8_t>{std::uint8_t{0x10U}});
	const auto byte = std::uint8_t{0xAAU};
	std::error_code text_error = std::make_error_code(std::errc::permission_denied);
	std::error_code bytes_error = std::make_error_code(std::errc::permission_denied);

	const auto text_written = ket::file::TryWriteAllText(text_path, "", &text_error);
	const auto bytes_written = ket::file::TryWriteAllBytes(bytes_path, &byte, 0U, &bytes_error);

	std::string actual_text = "not empty";
	std::vector<std::uint8_t> actual_bytes = {std::uint8_t{0xFFU}};
	const auto text_read = ket::file::TryReadAllText(text_path, actual_text);
	const auto bytes_read = ket::file::TryReadAllBytes(bytes_path, actual_bytes);
	const auto text_size = ket::file::Size(text_path);
	const auto bytes_size = ket::file::Size(bytes_path);
	const auto text_error_is_set = static_cast<bool>(text_error);
	const auto bytes_error_is_set = static_cast<bool>(bytes_error);

	EXPECT_TRUE(text_written);
	EXPECT_TRUE(bytes_written);
	EXPECT_FALSE(text_error_is_set);
	EXPECT_FALSE(bytes_error_is_set);
	EXPECT_TRUE(text_read);
	EXPECT_TRUE(bytes_read);
	EXPECT_EQ(actual_text, std::string());
	EXPECT_EQ(actual_bytes, std::vector<std::uint8_t>());
	EXPECT_EQ(text_size, std::optional<std::uintmax_t>(std::uintmax_t{0U}));
	EXPECT_EQ(bytes_size, std::optional<std::uintmax_t>(std::uintmax_t{0U}));
}

/**
 * @test
 * @brief bytes書き込みのnullptr不正入力確認。
 * @details
 * dataがnullptrかつsizeが1の入力を新規pathと既存fileへ渡し、失敗と既存内容不変を確認。
 * @pre C++17以降。temporary directoryへfile作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, RejectsNullBytesWithNonZeroSize)
{
	const TemporaryDirectory temporary("null_bytes");
	const auto path = temporary.path() / "invalid.bin";
	const auto existing_path = temporary.path() / "existing.bin";
	const auto expected_bytes = std::vector<std::uint8_t>{std::uint8_t{0x10U}, std::uint8_t{0x11U}};
	WriteBytesForTest(existing_path, expected_bytes);
	std::error_code error;
	std::error_code existing_error;

	const auto write_succeeded = ket::file::TryWriteAllBytes(path, nullptr, 1U, &error);
	const auto existing_write_succeeded =
		ket::file::TryWriteAllBytes(existing_path, nullptr, 1U, &existing_error);
	const auto exists = ket::file::Exists(path);
	const auto existing_size = ket::file::Size(existing_path);

	EXPECT_FALSE(write_succeeded);
	EXPECT_EQ(error, std::make_error_code(std::errc::invalid_argument));
	EXPECT_FALSE(existing_write_succeeded);
	EXPECT_EQ(existing_error, std::make_error_code(std::errc::invalid_argument));
	EXPECT_FALSE(exists);
	EXPECT_EQ(existing_size, std::optional<std::uintmax_t>(std::uintmax_t{2U}));
	ExpectBytesForTest(existing_path, expected_bytes);
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

	EXPECT_FALSE(text_written);
	EXPECT_FALSE(bytes_written);
	EXPECT_EQ(text_error, std::make_error_code(std::errc::is_a_directory));
	EXPECT_EQ(bytes_error, std::make_error_code(std::errc::is_a_directory));
}

/**
 * @test
 * @brief 通常file以外のpath拒否確認。
 * @details POSIX環境でFIFOを作成し、read/writeがblockせず失敗値を返すことを確認。
 * @pre C++17以降。POSIX FIFOを作成可能な環境。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, RejectsNonRegularFilePaths)
{
#if defined(__unix__) || defined(__APPLE__)
	const TemporaryDirectory temporary("non_regular");
	const auto fifo_path = temporary.path() / "pipe";
	const auto fifo_created = ::mkfifo(fifo_path.c_str(), S_IRUSR | S_IWUSR) == 0;
	if (!fifo_created)
	{
		GTEST_SKIP() << "FIFO cannot be created in this environment";
	}

	std::string text = "keep";
	std::vector<std::uint8_t> bytes = {std::uint8_t{0xAAU}};
	std::error_code text_error;
	std::error_code bytes_error;
	std::error_code write_error;

	const auto text_read = ket::file::TryReadAllText(fifo_path, text, &text_error);
	const auto bytes_read = ket::file::TryReadAllBytes(fifo_path, bytes, &bytes_error);
	const auto text_written = ket::file::TryWriteAllText(fifo_path, "x", &write_error);
	const auto exists = ket::file::Exists(fifo_path);
	const auto is_directory = ket::file::IsDirectory(fifo_path);

	EXPECT_FALSE(text_read);
	EXPECT_FALSE(bytes_read);
	EXPECT_FALSE(text_written);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_EQ(bytes, std::vector<std::uint8_t>({std::uint8_t{0xAAU}}));
	EXPECT_EQ(text_error, std::make_error_code(std::errc::operation_not_supported));
	EXPECT_EQ(bytes_error, std::make_error_code(std::errc::operation_not_supported));
	EXPECT_EQ(write_error, std::make_error_code(std::errc::operation_not_supported));
	EXPECT_TRUE(exists);
	EXPECT_FALSE(is_directory);
#else
	GTEST_SKIP() << "non-regular file path test requires POSIX FIFO support";
#endif
}

/**
 * @test
 * @brief invalid pathの失敗確認。
 * @details 長すぎるpath
 * componentを対象にし、read/write/queryが失敗値を返し、出力を変更しないことを確認。
 * @pre C++17以降。temporary directory作成可能。
 * @post temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReportsFailureForInvalidPath)
{
	const TemporaryDirectory temporary("invalid_path");
	const auto invalid_path = temporary.path() / std::string(5000U, 'x');
	std::string text = "keep";
	std::error_code read_error;
	std::error_code write_error;

	const auto read_succeeded = ket::file::TryReadAllText(invalid_path, text, &read_error);
	const auto write_succeeded = ket::file::TryWriteAllText(invalid_path, "x", &write_error);
	const auto exists = ket::file::Exists(invalid_path);
	const auto is_directory = ket::file::IsDirectory(invalid_path);
	const auto size = ket::file::Size(invalid_path);
	const auto read_error_is_set = static_cast<bool>(read_error);
	const auto write_error_is_set = static_cast<bool>(write_error);

	EXPECT_FALSE(read_succeeded);
	EXPECT_FALSE(write_succeeded);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_TRUE(read_error_is_set);
	EXPECT_TRUE(write_error_is_set);
	EXPECT_FALSE(exists);
	EXPECT_FALSE(is_directory);
	EXPECT_EQ(size, std::nullopt);
}

/**
 * @test
 * @brief permission denied読み込み失敗確認。
 * @details
 * 読み取り権限を落としたfileを読み込み、環境が拒否を強制できる場合にfalseと出力不変を確認。
 * @pre C++17以降。filesystem permissionsを変更可能。
 * @post 対象fileのpermissionを復元。temporary directory配下以外の外部状態の変更なし。
 */
TEST(KetFileTest, ReportsPermissionDeniedWhenReadIsDenied)
{
	const TemporaryDirectory temporary("permission_denied");
	const auto path = temporary.path() / "secret.txt";
	WriteStringForTest(path, "secret");

	std::error_code permission_error;
	std::filesystem::permissions(path,
								 std::filesystem::perms::none,
								 std::filesystem::perm_options::replace,
								 permission_error);
	const auto permission_change_failed = static_cast<bool>(permission_error);
	if (permission_change_failed)
	{
		GTEST_SKIP() << "filesystem permissions cannot be changed in this environment";
	}

	std::string text = "keep";
	std::error_code read_error;
	const auto read_succeeded = ket::file::TryReadAllText(path, text, &read_error);

	std::error_code restore_error;
	std::filesystem::permissions(path,
								 std::filesystem::perms::owner_read |
									 std::filesystem::perms::owner_write,
								 std::filesystem::perm_options::replace,
								 restore_error);

	if (read_succeeded)
	{
		GTEST_SKIP() << "read permission denial is not enforced in this environment";
	}

	const auto read_error_is_set = static_cast<bool>(read_error);

	EXPECT_FALSE(read_succeeded);
	EXPECT_EQ(text, std::string("keep"));
	EXPECT_TRUE(read_error_is_set);
}
