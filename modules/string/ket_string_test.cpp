#include "ket_string.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

/**
 * @test
 * @brief 空引数の文字列連結確認。
 * @details 引数なしでStrCatを呼び出し、空文字列を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStringTest, ConcatenatesNoParts)
{
	const auto result = ket::StrCat();

	EXPECT_EQ(result, std::string());
}

/**
 * @test
 * @brief 複数種類の文字列片連結確認。
 * @details raw C
 * string、std::string、std::string_view、charを入力し、入力順に連結されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStringTest, ConcatenatesStringLikePartsAndChar)
{
	const auto middle = std::string("b");
	const auto suffix = std::string_view("c");

	const auto result = ket::StrCat("a", middle, suffix, 'd');

	EXPECT_EQ(result, std::string("abcd"));
}

/**
 * @test
 * @brief 長さ付き文字列片のNUL保持確認。
 * @details 埋め込みNULを含むstd::string_viewを入力し、NULを含む全バイトが保持されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStringTest, PreservesEmbeddedNullInStringView)
{
	const auto part = std::string_view("a\0b", 3U);
	const auto expected = std::string("a\0b", 3U);

	const auto result = ket::StrCat(part);
	const auto result_size = result.size();

	EXPECT_EQ(result, expected);
	EXPECT_EQ(result_size, 3U);
}

/**
 * @test
 * @brief 複数文字列片の末尾追記確認。
 * @details 既存文字列に重ならない複数種類の文字列片を渡し、既存内容の末尾へ追記されることを確認。
 * @pre C++17以降。
 * @post destinationは期待する文字列に変化。destination以外の外部状態の変更なし。
 */
TEST(KetStringTest, AppendsNonAliasedPartsToExistingString)
{
	std::string destination = "id=";
	const auto id_text = std::string("42");
	const auto mode_text = std::string_view("run");

	ket::StrAppend(destination, id_text, ", mode=", mode_text, '!');

	EXPECT_EQ(destination, std::string("id=42, mode=run!"));
}

/**
 * @test
 * @brief 長さ付き文字列片のNUL保持追記確認。
 * @details 埋め込みNULを含むstd::string_viewを追記し、NULを含む全バイトが保持されることを確認。
 * @pre C++17以降。
 * @post destinationは期待する文字列に変化。destination以外の外部状態の変更なし。
 */
TEST(KetStringTest, AppendsEmbeddedNullInStringView)
{
	std::string destination = "a";
	const auto part = std::string_view("b\0c", 3U);
	const auto expected = std::string("ab\0cd", 5U);

	ket::StrAppend(destination, part, 'd');
	const auto destination_size = destination.size();

	EXPECT_EQ(destination, expected);
	EXPECT_EQ(destination_size, 5U);
}

/**
 * @test
 * @brief 空引数の末尾追記確認。
 * @details 引数なしでStrAppendを呼び出し、追記先の内容が変化しないことを確認。
 * @pre C++17以降。
 * @post destinationは入力時の文字列を保持。destination以外の外部状態の変更なし。
 */
TEST(KetStringTest, AppendsNoPartsAsNoOp)
{
	std::string destination = "unchanged";

	ket::StrAppend(destination);

	EXPECT_EQ(destination, std::string("unchanged"));
}

/**
 * @test
 * @brief 自己参照を含む末尾追記確認。
 * @details 追記先文字列とそのviewをpartsに含め、追記前の入力内容でsuffixが確定することを確認。
 * @pre C++17以降。
 * @post destinationは追記前の入力内容から作ったsuffixを含む文字列に変化。
 */
TEST(KetStringTest, AppendsSelfReferencesAfterMaterializingSuffix)
{
	std::string destination = "ab";
	const auto destination_view = std::string_view(destination);

	ket::StrAppend(destination, destination, ":", destination_view);

	EXPECT_EQ(destination, std::string("abab:ab"));
}

/**
 * @test
 * @brief 部分viewとraw C string自己参照の末尾追記確認。
 * @details
 * 追記先文字列の部分viewとc_str由来ポインタをpartsに含め、追記前の内容でsuffixが確定することを確認。
 * @pre C++17以降。
 * @post destinationは追記前の入力内容から作ったsuffixを含む文字列に変化。
 */
TEST(KetStringTest, AppendsSubstringViewAndCStringSelfReferences)
{
	std::string destination = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	destination.shrink_to_fit();
	const auto part_view = std::string_view(destination.data() + 10, 5U);
	const char* const part_c_string = destination.c_str() + 52;
	const auto expected = std::string(
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:klmno:0123456789");

	ket::StrAppend(destination, ":", part_view, ":", part_c_string);

	EXPECT_EQ(destination, expected);
}
