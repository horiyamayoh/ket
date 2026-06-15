#include "ket_cli.h"

#include <optional>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

/**
 * @test
 * @brief argv viewの境界条件確認。
 * @details 負のargc、null argv、null要素、範囲外indexを入力し、空viewとして扱うことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, HandlesArgvViewBoundaryInputs)
{
	const char* const argv_with_null[] = {"tool", nullptr, "--name"};

	const auto negative_argc = ket::cli::ArgvView(-1, argv_with_null);
	const auto null_argv = ket::cli::ArgvView(3, nullptr);
	const auto args = ket::cli::ArgvView(3, argv_with_null);

	const auto negative_size = negative_argc.Size();
	const auto null_argv_size = null_argv.Size();
	const auto program_name = args.ProgramNameOrEmpty();
	const auto null_argument = args.AtOrEmpty(1U);
	const auto out_of_range_argument = args.AtOrEmpty(3U);

	EXPECT_EQ(negative_size, 0U);
	EXPECT_EQ(null_argv_size, 0U);
	EXPECT_EQ(program_name, std::string_view("tool"));
	EXPECT_EQ(null_argument, std::string_view());
	EXPECT_EQ(out_of_range_argument, std::string_view());
}

/**
 * @test
 * @brief flag optionの有無確認。
 * @details
 * `--help`とinline値付きoptionを入力し、HasOptionが一致するoptionだけを検出することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, DetectsFlagsAndInlineOptions)
{
	const char* const argv[] = {"tool", "--help", "--mode=fast", "input"};
	const auto args = ket::cli::ArgvView(4, argv);

	const auto has_help = ket::cli::HasOption(args, "--help");
	const auto has_mode = ket::cli::HasOption(args, "--mode");
	const auto has_missing = ket::cli::HasOption(args, "--missing");
	const auto has_invalid_name = ket::cli::HasOption(args, "mode");

	EXPECT_TRUE(has_help);
	EXPECT_TRUE(has_mode);
	EXPECT_FALSE(has_missing);
	EXPECT_FALSE(has_invalid_name);
}

/**
 * @test
 * @brief separate値付きoptionの取得確認。
 * @details `--id 123`を入力し、OptionValueとOptionValueOrが次要素の値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, GetsSeparateOptionValue)
{
	const char* const argv[] = {"tool", "--id", "123"};
	const auto args = ket::cli::ArgvView(3, argv);

	const auto value = ket::cli::OptionValue(args, "--id");
	const auto value_or = ket::cli::OptionValueOr(args, "--id", "fallback");

	EXPECT_EQ(value, std::optional<std::string_view>("123"));
	EXPECT_EQ(value_or, std::string_view("123"));
}

/**
 * @test
 * @brief inline値付きoptionの取得確認。
 * @details `--id=123`と空文字列値の`--empty=`を入力し、`=`以降の値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, GetsInlineOptionValue)
{
	const char* const argv[] = {"tool", "--id=123", "--empty="};
	const auto args = ket::cli::ArgvView(3, argv);

	const auto value = ket::cli::OptionValue(args, "--id");
	const auto empty_value = ket::cli::OptionValue(args, "--empty");

	EXPECT_EQ(value, std::optional<std::string_view>("123"));
	EXPECT_EQ(empty_value, std::optional<std::string_view>(""));
}

/**
 * @test
 * @brief 重複optionの先勝ち確認。
 * @details separate値とinline値で同じoptionを複数入力し、最初に現れた値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, ReturnsFirstDuplicateOptionValue)
{
	const char* const separate_first_argv[] = {"tool", "--id", "123", "--id=456"};
	const char* const inline_first_argv[] = {"tool", "--id=abc", "--id", "def"};
	const auto separate_first_args = ket::cli::ArgvView(4, separate_first_argv);
	const auto inline_first_args = ket::cli::ArgvView(4, inline_first_argv);

	const auto separate_first = ket::cli::OptionValue(separate_first_args, "--id");
	const auto inline_first = ket::cli::OptionValue(inline_first_args, "--id");

	EXPECT_EQ(separate_first, std::optional<std::string_view>("123"));
	EXPECT_EQ(inline_first, std::optional<std::string_view>("abc"));
}

/**
 * @test
 * @brief option値欠落時の失敗確認。
 * @details 末尾optionと次要素が別optionの入力を渡し、OptionValueがstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, RejectsMissingSeparateOptionValue)
{
	const char* const end_argv[] = {"tool", "--id"};
	const char* const next_option_argv[] = {"tool", "--id", "--mode"};
	const auto end_args = ket::cli::ArgvView(2, end_argv);
	const auto next_option_args = ket::cli::ArgvView(3, next_option_argv);

	const auto end_value = ket::cli::OptionValue(end_args, "--id");
	const auto next_option_value = ket::cli::OptionValue(next_option_args, "--id");
	const auto fallback_value = ket::cli::OptionValueOr(next_option_args, "--id", "fallback");

	EXPECT_EQ(end_value, std::nullopt);
	EXPECT_EQ(next_option_value, std::nullopt);
	EXPECT_EQ(fallback_value, std::string_view("fallback"));
}

/**
 * @test
 * @brief 不正option名の失敗確認。
 * @details `--`で始まらないoption名を渡し、HasOptionとOptionValueOrが失敗値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, RejectsInvalidOptionName)
{
	const char* const argv[] = {"tool", "--id", "123"};
	const auto args = ket::cli::ArgvView(3, argv);

	const auto has_invalid_name = ket::cli::HasOption(args, "id");
	const auto invalid_value = ket::cli::OptionValue(args, "id");
	const auto invalid_value_or = ket::cli::OptionValueOr(args, "id", "fallback");

	EXPECT_FALSE(has_invalid_name);
	EXPECT_EQ(invalid_value, std::nullopt);
	EXPECT_EQ(invalid_value_or, std::string_view("fallback"));
}

/**
 * @test
 * @brief positional argument取得確認。
 * @details program nameを除くargvから`--`で始まらない要素を抽出し、入力順を保持することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, CollectsPositionalArguments)
{
	const char* const argv[] = {"tool", "input.txt", "--verbose", "output.txt", "--id=123"};
	const auto args = ket::cli::ArgvView(5, argv);

	const auto positional = ket::cli::PositionalArguments(args);
	const auto expected = std::vector<std::string_view>{"input.txt", "output.txt"};

	EXPECT_EQ(positional, expected);
}

/**
 * @test
 * @brief null要素を空positional argumentとして扱う確認。
 * @details argv中のnull要素を入力し、空文字列のpositional argumentとして保持することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCliTest, KeepsNullArgumentAsEmptyPositionalArgument)
{
	const char* const argv[] = {"tool", nullptr, "--name", nullptr};
	const auto args = ket::cli::ArgvView(4, argv);

	const auto positional = ket::cli::PositionalArguments(args);
	const auto expected = std::vector<std::string_view>{std::string_view(), std::string_view()};

	EXPECT_EQ(positional, expected);
}
