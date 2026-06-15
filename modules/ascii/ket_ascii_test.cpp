#include "ket_ascii.h"

#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

/**
 * @test
 * @brief prefix、suffix、containsの境界一致確認。
 * @details
 * 空needle、完全一致、長すぎるneedle、埋め込みNULを含むbyte列を入力し、byte単位の一致判定を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, ChecksPrefixSuffixAndContainsBoundaries)
{
	const auto text = std::string_view("alpha\0beta", 10U);
	const auto prefix = std::string_view("alpha", 5U);
	const auto suffix = std::string_view("beta", 4U);
	const auto needle = std::string_view("ha\0be", 5U);
	const auto longer = std::string_view("alpha\0beta!", 11U);

	const auto starts_with_empty = ket::ascii::StartsWith(text, "");
	const auto starts_with_prefix = ket::ascii::StartsWith(text, prefix);
	const auto starts_with_longer = ket::ascii::StartsWith(text, longer);
	const auto ends_with_empty = ket::ascii::EndsWith(text, "");
	const auto ends_with_suffix = ket::ascii::EndsWith(text, suffix);
	const auto ends_with_longer = ket::ascii::EndsWith(text, longer);
	const auto contains_empty = ket::ascii::Contains(text, "");
	const auto contains_needle = ket::ascii::Contains(text, needle);
	const auto contains_missing = ket::ascii::Contains(text, "BETA");

	EXPECT_TRUE(starts_with_empty);
	EXPECT_TRUE(starts_with_prefix);
	EXPECT_FALSE(starts_with_longer);
	EXPECT_TRUE(ends_with_empty);
	EXPECT_TRUE(ends_with_suffix);
	EXPECT_FALSE(ends_with_longer);
	EXPECT_TRUE(contains_empty);
	EXPECT_TRUE(contains_needle);
	EXPECT_FALSE(contains_missing);
}

/**
 * @test
 * @brief ASCII whitespace trimの確認。
 * @details 空入力、全ASCII whitespace、両端whitespace、非ASCII
 * byteを含む入力を渡し、ASCIIだけが除去されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, TrimsAsciiWhitespaceOnly)
{
	const auto empty = ket::ascii::Trim("");
	const auto all_whitespace = ket::ascii::Trim(" \t\n\r\f\v");
	const auto left = ket::ascii::TrimLeft(" \tvalue \n");
	const auto right = ket::ascii::TrimRight(" \tvalue \n");
	const auto both = ket::ascii::Trim(" \tvalue \n");
	const auto non_ascii_space = std::string("\xC2\xA0value\xC2\xA0", 9U);

	const auto non_ascii_trimmed = ket::ascii::Trim(non_ascii_space);

	EXPECT_EQ(empty, std::string_view());
	EXPECT_EQ(all_whitespace, std::string_view());
	EXPECT_EQ(left, std::string_view("value \n"));
	EXPECT_EQ(right, std::string_view(" \tvalue"));
	EXPECT_EQ(both, std::string_view("value"));
	EXPECT_EQ(non_ascii_trimmed, std::string_view(non_ascii_space));
}

/**
 * @test
 * @brief StripPrefixとStripSuffixのnon-owning view確認。
 * @details 一致時と不一致時を入力し、戻りviewが元文字列の期待位置を参照することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, StripsPrefixAndSuffixWithoutOwning)
{
	const auto text = std::string("prefix-body-suffix");
	const auto text_view = std::string_view(text);

	const auto stripped_prefix = ket::ascii::StripPrefix(text_view, "prefix-");
	const auto unchanged_prefix = ket::ascii::StripPrefix(text_view, "missing");
	const auto stripped_suffix = ket::ascii::StripSuffix(text_view, "-suffix");
	const auto unchanged_suffix = ket::ascii::StripSuffix(text_view, "missing");

	EXPECT_EQ(stripped_prefix, std::string_view("body-suffix"));
	EXPECT_EQ(stripped_prefix.data(), text.data() + 7U);
	EXPECT_EQ(unchanged_prefix, text_view);
	EXPECT_EQ(unchanged_prefix.data(), text.data());
	EXPECT_EQ(stripped_suffix, std::string_view("prefix-body"));
	EXPECT_EQ(stripped_suffix.data(), text.data());
	EXPECT_EQ(unchanged_suffix, text_view);
	EXPECT_EQ(unchanged_suffix.data(), text.data());
}

/**
 * @test
 * @brief SplitViewsの空要素保持確認。
 * @details 空入力、delimiterなし、leading/trailing/repeated
 * delimiterを入力し、空要素を含むview列を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。戻りviewは元文字列を参照。
 */
TEST(KetAsciiTest, SplitViewsKeepsEmptyFields)
{
	const auto empty = ket::ascii::SplitViews("", ',');
	const auto no_delimiter_text = std::string("abc");
	const auto no_delimiter = ket::ascii::SplitViews(no_delimiter_text, ',');
	const auto separated_text = std::string(",a,,b,");
	const auto separated = ket::ascii::SplitViews(separated_text, ',');

	ASSERT_EQ(empty.size(), 1U);
	EXPECT_EQ(empty[0], std::string_view());

	ASSERT_EQ(no_delimiter.size(), 1U);
	EXPECT_EQ(no_delimiter[0], std::string_view("abc"));
	EXPECT_EQ(no_delimiter[0].data(), no_delimiter_text.data());

	ASSERT_EQ(separated.size(), 5U);
	EXPECT_EQ(separated[0], std::string_view());
	EXPECT_EQ(separated[1], std::string_view("a"));
	EXPECT_EQ(separated[2], std::string_view());
	EXPECT_EQ(separated[3], std::string_view("b"));
	EXPECT_EQ(separated[4], std::string_view());
	EXPECT_EQ(separated[1].data(), separated_text.data() + 1U);
	EXPECT_EQ(separated[3].data(), separated_text.data() + 4U);
}

/**
 * @test
 * @brief Splitのowning文字列確認。
 * @details
 * delimiterを含む文字列を分割後に元文字列を変更し、結果文字列が元文字列に依存しないことを確認。
 * @pre C++17以降。
 * @post 分割結果は期待値を保持。入力用文字列以外の外部状態の変更なし。
 */
TEST(KetAsciiTest, SplitReturnsOwningStrings)
{
	auto text = std::string("a,,b,");

	const auto parts = ket::ascii::Split(text, ',');
	text[0] = 'x';

	const auto expected = std::vector<std::string>{"a", "", "b", ""};

	EXPECT_EQ(text[0], 'x');
	EXPECT_EQ(parts, expected);
}

/**
 * @test
 * @brief Joinのdelimiter付き連結確認。
 * @details 空要素、空delimiter、空partsを入力し、入力順とdelimiter挿入位置を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, JoinsViewsWithDelimiter)
{
	const auto parts = std::vector<std::string_view>{"a", "b", ""};
	const auto empty_parts = std::vector<std::string_view>{};

	const auto joined = ket::ascii::Join(parts, ",");
	const auto joined_without_delimiter = ket::ascii::Join(parts, "");
	const auto joined_empty = ket::ascii::Join(empty_parts, ",");

	EXPECT_EQ(joined, std::string("a,b,"));
	EXPECT_EQ(joined_without_delimiter, std::string("ab"));
	EXPECT_EQ(joined_empty, std::string());
}

/**
 * @test
 * @brief ReplaceAllの非重複置換確認。
 * @details 一致なし、繰り返し一致、削除、UTF-8 byte列を入力し、左から非重複に置換されることを確認。
 * @pre C++17以降。`from`は空でない。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, ReplacesAllNonOverlappingMatches)
{
	const auto no_match = ket::ascii::ReplaceAll("abc", "x", "y");
	const auto repeated = ket::ascii::ReplaceAll("aaaa", "aa", "a");
	const auto expanded = ket::ascii::ReplaceAll("a--b--", "--", "::");
	const auto deleted = ket::ascii::ReplaceAll("a--b--", "--", "");
	const auto utf8_text = std::string("caf\xC3\xA9 caf\xC3\xA9", 11U);
	const auto utf8_replaced = ket::ascii::ReplaceAll(utf8_text, "\xC3\xA9", "e");

	EXPECT_EQ(no_match, std::string("abc"));
	EXPECT_EQ(repeated, std::string("aa"));
	EXPECT_EQ(expanded, std::string("a::b::"));
	EXPECT_EQ(deleted, std::string("ab"));
	EXPECT_EQ(utf8_replaced, std::string("cafe cafe"));
}

/**
 * @test
 * @brief ASCII case変換のbyte保持確認。
 * @details ASCII英字、数字、記号、UTF-8
 * byte列、埋め込みNULを入力し、ASCII英字だけが変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, ConvertsAsciiCaseOnly)
{
	const auto text = std::string("Az_09!\0\xC3\x89", 9U);

	const auto lower = ket::ascii::ToLower(text);
	const auto upper = ket::ascii::ToUpper(text);
	const auto expected_lower = std::string("az_09!\0\xC3\x89", 9U);
	const auto expected_upper = std::string("AZ_09!\0\xC3\x89", 9U);

	EXPECT_EQ(lower, expected_lower);
	EXPECT_EQ(upper, expected_upper);
}

/**
 * @test
 * @brief EqualsIgnoreCaseのASCII限定比較確認。
 * @details ASCII大小差、長さ不一致、UTF-8
 * byte列の一致と不一致を入力し、ASCII英字だけ大小を無視することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, ComparesAsciiIgnoringCaseOnly)
{
	const auto ascii_equal = ket::ascii::EqualsIgnoreCase("Mode-42", "mode-42");
	const auto size_mismatch = ket::ascii::EqualsIgnoreCase("mode", "mode!");
	const auto ascii_mismatch = ket::ascii::EqualsIgnoreCase("mode-a", "mode-b");
	const auto utf8_equal = ket::ascii::EqualsIgnoreCase("\xC3\x89", "\xC3\x89");
	const auto utf8_mismatch = ket::ascii::EqualsIgnoreCase("\xC3\x89", "\xC3\xA9");

	EXPECT_TRUE(ascii_equal);
	EXPECT_FALSE(size_mismatch);
	EXPECT_FALSE(ascii_mismatch);
	EXPECT_TRUE(utf8_equal);
	EXPECT_FALSE(utf8_mismatch);
}
