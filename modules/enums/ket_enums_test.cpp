#include "ket_enums.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	enum class Mode : std::uint8_t
	{
		kIdle = 0,
		kRun = 1,
		kStop = 2,
		kDuplicateName = 3,
		kUnknown = 99,
	};

	enum class SignedCode : std::int8_t
	{
		kNegative = -1,
		kZero = 0,
		kPositive = 1,
		kUnknown = 9,
	};

	enum class Permission : std::uint8_t
	{
		kNone = 0U,
		kRead = 1U,
		kWrite = 2U,
		kExecute = 4U,
		kReadWrite = 3U,
		kWriteExecute = 6U,
		kAll = 7U,
	};

	constexpr ket::enums::Entry<Mode> kModeTable[] = {
		ket::enums::Entry{Mode::kIdle, "idle"},
		ket::enums::Entry{Mode::kRun, "run"},
		ket::enums::Entry{Mode::kStop, "stop"},
	};

	constexpr ket::enums::Entry<Mode> kDuplicateTable[] = {
		ket::enums::Entry{Mode::kRun, "first"},
		ket::enums::Entry{Mode::kRun, "second"},
		ket::enums::Entry{Mode::kDuplicateName, "first"},
	};

	constexpr ket::enums::Entry<SignedCode> kSignedCodeTable[] = {
		ket::enums::Entry{SignedCode::kNegative, "-1"},
		ket::enums::Entry{SignedCode::kZero, "0"},
		ket::enums::Entry{SignedCode::kPositive, "1"},
	};

	static_assert(
		std::is_same_v<decltype(ket::enums::Entry{Mode::kIdle, "idle"}), ket::enums::Entry<Mode>>,
		"Entry deduction guide deduces enum type");
	static_assert(ket::enums::ToUnderlying(Mode::kRun) == 1, "ToUnderlying is constexpr");
	static_assert(ket::enums::ToUnderlying(Permission::kExecute) == 4U,
				  "ToUnderlying keeps underlying value");
	static_assert(ket::enums::ToUnderlying(SignedCode::kNegative) == -1,
				  "ToUnderlying keeps signed underlying value");
	static_assert(ket::enums::Name(Mode::kRun, kModeTable).has_value(), "Name is constexpr");
	static_assert(*ket::enums::Name(Mode::kRun, kModeTable) == std::string_view("run"),
				  "Name returns matching string_view");
	static_assert(ket::enums::NameOr(Mode::kUnknown, kModeTable, "unknown") ==
					  std::string_view("unknown"),
				  "NameOr is constexpr");
	static_assert(ket::enums::Parse<Mode>("run", kModeTable).has_value(), "Parse is constexpr");
	static_assert(*ket::enums::Parse<Mode>("run", kModeTable) == Mode::kRun,
				  "Parse returns matching enum value");
	static_assert(ket::enums::IsValid(Mode::kRun, kModeTable), "IsValid is constexpr");
	static_assert(!ket::enums::IsValid(Mode::kUnknown, kModeTable),
				  "IsValid rejects unknown enum values");
	static_assert(ket::enums::SetFlag(Permission::kRead, Permission::kWrite) ==
					  Permission::kReadWrite,
				  "SetFlag is constexpr");
	static_assert(ket::enums::ClearFlag(static_cast<Permission>(3U), Permission::kRead) ==
					  Permission::kWrite,
				  "ClearFlag is constexpr");
	static_assert(ket::enums::HasFlag(static_cast<Permission>(3U), Permission::kWrite),
				  "HasFlag is constexpr");
	static_assert(ket::enums::HasAnyFlag(static_cast<Permission>(3U), Permission::kRead),
				  "HasAnyFlag is constexpr");
	static_assert(ket::enums::HasAllFlags(Permission::kReadWrite, Permission::kReadWrite),
				  "HasAllFlags is constexpr");

} // namespace

/**
 * @test
 * @brief 既知enum値の名前取得確認。
 * @details tableに存在する先頭、中央、末尾の値を入力し、対応する名前が返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, GetsNameForKnownValues)
{
	const auto idle = ket::enums::Name(Mode::kIdle, kModeTable);
	const auto run = ket::enums::Name(Mode::kRun, kModeTable);
	const auto stop = ket::enums::Name(Mode::kStop, kModeTable);

	EXPECT_EQ(idle, std::optional<std::string_view>(std::string_view("idle")));
	EXPECT_EQ(run, std::optional<std::string_view>(std::string_view("run")));
	EXPECT_EQ(stop, std::optional<std::string_view>(std::string_view("stop")));
}

/**
 * @test
 * @brief 未知enum値の名前取得失敗確認。
 * @details tableに存在しないenum値を入力し、std::nulloptが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, ReturnsNulloptForUnknownValue)
{
	const auto name = ket::enums::Name(Mode::kUnknown, kModeTable);

	EXPECT_EQ(name, std::nullopt);
}

/**
 * @test
 * @brief fallback付き名前取得確認。
 * @details 既知値ではtableの名前を返し、未知値では呼び出し側fallbackを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, GetsNameOrFallback)
{
	const auto fallback = std::string("unknown");
	const auto fallback_view = std::string_view(fallback.data(), fallback.size());

	const auto known = ket::enums::NameOr(Mode::kRun, kModeTable, "unknown");
	const auto unknown = ket::enums::NameOr(Mode::kUnknown, kModeTable, fallback_view);

	EXPECT_EQ(known, std::string_view("run"));
	EXPECT_EQ(unknown, std::string_view("unknown"));
}

/**
 * @test
 * @brief 既知名前のenum値parse確認。
 * @details tableに存在する名前を入力し、対応するenum値が返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, ParsesKnownText)
{
	const auto idle = ket::enums::Parse<Mode>("idle", kModeTable);
	const auto run = ket::enums::Parse<Mode>("run", kModeTable);
	const auto stop = ket::enums::Parse<Mode>("stop", kModeTable);

	EXPECT_EQ(idle, std::optional<Mode>(Mode::kIdle));
	EXPECT_EQ(run, std::optional<Mode>(Mode::kRun));
	EXPECT_EQ(stop, std::optional<Mode>(Mode::kStop));
}

/**
 * @test
 * @brief 未知名前と不完全一致のparse失敗確認。
 * @details 存在しない名前、大文字小文字違い、部分一致を入力し、std::nulloptが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, RejectsUnknownOrNonExactText)
{
	const auto unknown = ket::enums::Parse<Mode>("pause", kModeTable);
	const auto different_case = ket::enums::Parse<Mode>("RUN", kModeTable);
	const auto partial = ket::enums::Parse<Mode>("ru", kModeTable);
	const auto empty = ket::enums::Parse<Mode>("", kModeTable);

	EXPECT_EQ(unknown, std::nullopt);
	EXPECT_EQ(different_case, std::nullopt);
	EXPECT_EQ(partial, std::nullopt);
	EXPECT_EQ(empty, std::nullopt);
}

/**
 * @test
 * @brief 重複entryの先勝ち確認。
 * @details value重複では最初の名前を返し、name重複では最初のenum値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, UsesFirstDuplicateEntry)
{
	const auto name = ket::enums::Name(Mode::kRun, kDuplicateTable);
	const auto value = ket::enums::Parse<Mode>("first", kDuplicateTable);

	EXPECT_EQ(name, std::optional<std::string_view>(std::string_view("first")));
	EXPECT_EQ(value, std::optional<Mode>(Mode::kRun));
}

/**
 * @test
 * @brief enum値のtable存在確認。
 * @details 既知値と未知値を入力し、tableに存在する値だけtrueになることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, ChecksValueValidity)
{
	const auto run_is_valid = ket::enums::IsValid(Mode::kRun, kModeTable);
	const auto unknown_is_valid = ket::enums::IsValid(Mode::kUnknown, kModeTable);

	EXPECT_TRUE(run_is_valid);
	EXPECT_FALSE(unknown_is_valid);
}

/**
 * @test
 * @brief 符号付きunderlying enumのtable変換確認。
 * @details
 * 負値を持つenum値に対して名前取得、parse、存在判定がunderlying符号に依存せず動くことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, HandlesSignedUnderlyingEnumLookup)
{
	const auto negative_name = ket::enums::Name(SignedCode::kNegative, kSignedCodeTable);
	const auto zero_name = ket::enums::Name(SignedCode::kZero, kSignedCodeTable);
	const auto negative_value = ket::enums::Parse<SignedCode>("-1", kSignedCodeTable);
	const auto positive_value = ket::enums::Parse<SignedCode>("1", kSignedCodeTable);
	const auto unknown_value = ket::enums::Parse<SignedCode>("9", kSignedCodeTable);
	const auto negative_is_valid = ket::enums::IsValid(SignedCode::kNegative, kSignedCodeTable);
	const auto unknown_is_valid = ket::enums::IsValid(SignedCode::kUnknown, kSignedCodeTable);

	EXPECT_EQ(negative_name, std::optional<std::string_view>(std::string_view("-1")));
	EXPECT_EQ(zero_name, std::optional<std::string_view>(std::string_view("0")));
	EXPECT_EQ(negative_value, std::optional<SignedCode>(SignedCode::kNegative));
	EXPECT_EQ(positive_value, std::optional<SignedCode>(SignedCode::kPositive));
	EXPECT_EQ(unknown_value, std::nullopt);
	EXPECT_TRUE(negative_is_valid);
	EXPECT_FALSE(unknown_is_valid);
}

/**
 * @test
 * @brief flagsのset、clear、has確認。
 * @details
 * read/write/executeのbitを操作し、単一flagとmaskの判定がunderlying値で行われることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, ManipulatesFlags)
{
	const auto read_write = ket::enums::SetFlag(Permission::kRead, Permission::kWrite);
	const auto read_write_execute = ket::enums::SetFlag(read_write, Permission::kExecute);
	const auto write_execute = ket::enums::ClearFlag(read_write_execute, Permission::kRead);

	const auto read_write_value = ket::enums::ToUnderlying(read_write);
	const auto read_write_execute_value = ket::enums::ToUnderlying(read_write_execute);
	const auto write_execute_value = ket::enums::ToUnderlying(write_execute);

	const auto has_read = ket::enums::HasFlag(read_write, Permission::kRead);
	const auto has_execute_before_set = ket::enums::HasFlag(read_write, Permission::kExecute);
	const auto has_execute_after_clear = ket::enums::HasFlag(write_execute, Permission::kExecute);
	const auto has_read_after_clear = ket::enums::HasFlag(write_execute, Permission::kRead);

	EXPECT_EQ(read_write_value, 3U);
	EXPECT_EQ(read_write_execute_value, 7U);
	EXPECT_EQ(write_execute_value, 6U);
	EXPECT_TRUE(has_read);
	EXPECT_FALSE(has_execute_before_set);
	EXPECT_TRUE(has_execute_after_clear);
	EXPECT_FALSE(has_read_after_clear);
}

/**
 * @test
 * @brief flags操作のno-op境界確認。
 * @details 0 mask、既存flagのset、存在しないflagのclearが入力flag集合を変えないことを確認。
 * @pre C++17以降。flag enumはunsigned underlying typeを持つ。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, KeepsFlagsUnchangedForNoOpMutations)
{
	const auto flags = Permission::kReadWrite;

	const auto set_none = ket::enums::SetFlag(flags, Permission::kNone);
	const auto set_existing = ket::enums::SetFlag(flags, Permission::kRead);
	const auto clear_none = ket::enums::ClearFlag(flags, Permission::kNone);
	const auto clear_absent = ket::enums::ClearFlag(flags, Permission::kExecute);

	const auto set_none_value = ket::enums::ToUnderlying(set_none);
	const auto set_existing_value = ket::enums::ToUnderlying(set_existing);
	const auto clear_none_value = ket::enums::ToUnderlying(clear_none);
	const auto clear_absent_value = ket::enums::ToUnderlying(clear_absent);

	EXPECT_EQ(set_none_value, 3U);
	EXPECT_EQ(set_existing_value, 3U);
	EXPECT_EQ(clear_none_value, 3U);
	EXPECT_EQ(clear_absent_value, 3U);
}

/**
 * @test
 * @brief flagsのany/all境界確認。
 * @details 空mask、部分一致、完全一致を入力し、any/allの境界動作を固定する。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetEnumsTest, ChecksAnyAndAllFlags)
{
	const auto flags = Permission::kReadWrite;
	const auto read_write = Permission::kReadWrite;
	const auto write_execute = Permission::kWriteExecute;

	const auto has_any_read_write = ket::enums::HasAnyFlag(flags, read_write);
	const auto has_any_write_execute = ket::enums::HasAnyFlag(flags, write_execute);
	const auto has_any_none = ket::enums::HasAnyFlag(flags, Permission::kNone);
	const auto has_all_read_write = ket::enums::HasAllFlags(flags, read_write);
	const auto has_all_write_execute = ket::enums::HasAllFlags(flags, write_execute);
	const auto has_all_none = ket::enums::HasAllFlags(flags, Permission::kNone);
	const auto has_none_flag = ket::enums::HasFlag(flags, Permission::kNone);

	EXPECT_TRUE(has_any_read_write);
	EXPECT_TRUE(has_any_write_execute);
	EXPECT_FALSE(has_any_none);
	EXPECT_TRUE(has_all_read_write);
	EXPECT_FALSE(has_all_write_execute);
	EXPECT_TRUE(has_all_none);
	EXPECT_TRUE(has_none_flag);
}
