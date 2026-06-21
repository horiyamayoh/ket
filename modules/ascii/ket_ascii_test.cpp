#include "ket_ascii.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	template <typename T, typename = void>
	struct CanTrim : std::false_type
	{
	};

	template <typename T>
	struct CanTrim<T, std::void_t<decltype(ket::ascii::Trim(std::declval<T>()))>> : std::true_type
	{
	};

	template <typename T, typename = void>
	struct CanTrimLeft : std::false_type
	{
	};

	template <typename T>
	struct CanTrimLeft<T, std::void_t<decltype(ket::ascii::TrimLeft(std::declval<T>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct CanTrimRight : std::false_type
	{
	};

	template <typename T>
	struct CanTrimRight<T, std::void_t<decltype(ket::ascii::TrimRight(std::declval<T>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct CanStripPrefix : std::false_type
	{
	};

	template <typename T>
	struct CanStripPrefix<T,
						  std::void_t<decltype(ket::ascii::StripPrefix(
							  std::declval<T>(), std::declval<std::string_view>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct CanStripSuffix : std::false_type
	{
	};

	template <typename T>
	struct CanStripSuffix<T,
						  std::void_t<decltype(ket::ascii::StripSuffix(
							  std::declval<T>(), std::declval<std::string_view>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct CanSplitViews : std::false_type
	{
	};

	template <typename T>
	struct CanSplitViews<T, std::void_t<decltype(ket::ascii::SplitViews(std::declval<T>(), ','))>>
		: std::true_type
	{
	};

} // namespace

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
 * @brief 空入力の境界動作確認。
 * @details
 * 判定、strip、split、replace、case変換、case-insensitive比較へ空入力を渡し、空入力固有の仕様を確認。
 * @pre C++17以降。`ReplaceAll`の`from`は空でない。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetAsciiTest, HandlesEmptyInputBoundaries)
{
	const auto starts_with_empty = ket::ascii::StartsWith("", "");
	const auto starts_with_non_empty = ket::ascii::StartsWith("", "x");
	const auto ends_with_empty = ket::ascii::EndsWith("", "");
	const auto ends_with_non_empty = ket::ascii::EndsWith("", "x");
	const auto contains_empty = ket::ascii::Contains("", "");
	const auto contains_non_empty = ket::ascii::Contains("", "x");
	const auto stripped_prefix = ket::ascii::StripPrefix("", "x");
	const auto stripped_suffix = ket::ascii::StripSuffix("", "x");
	const auto split = ket::ascii::Split("", ',');
	const auto replaced = ket::ascii::ReplaceAll("", "x", "y");
	const auto lower = ket::ascii::ToLower("");
	const auto upper = ket::ascii::ToUpper("");
	const auto equals_empty = ket::ascii::EqualsIgnoreCase("", "");

	const auto expected_split = std::vector<std::string>{""};

	EXPECT_TRUE(starts_with_empty);
	EXPECT_FALSE(starts_with_non_empty);
	EXPECT_TRUE(ends_with_empty);
	EXPECT_FALSE(ends_with_non_empty);
	EXPECT_TRUE(contains_empty);
	EXPECT_FALSE(contains_non_empty);
	EXPECT_EQ(stripped_prefix, std::string_view());
	EXPECT_EQ(stripped_suffix, std::string_view());
	EXPECT_EQ(split, expected_split);
	EXPECT_EQ(replaced, std::string());
	EXPECT_EQ(lower, std::string());
	EXPECT_EQ(upper, std::string());
	EXPECT_TRUE(equals_empty);
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
 * @brief trim戻りviewの参照位置確認。
 * @details
 * std::stringを入力し、Trim/TrimLeft/TrimRightの戻りviewが元文字列内の期待位置と長さを参照することを確認。
 * @pre C++17以降。
 * @post 入力文字列と外部状態の変更なし。
 */
TEST(KetAsciiTest, TrimViewsReferenceOriginalString)
{
	const auto text = std::string(" \tvalue \n");

	const auto left = ket::ascii::TrimLeft(text);
	const auto right = ket::ascii::TrimRight(text);
	const auto both = ket::ascii::Trim(text);

	EXPECT_EQ(left, std::string_view("value \n"));
	EXPECT_EQ(left.data(), text.data() + 2U);
	EXPECT_EQ(right, std::string_view(" \tvalue"));
	EXPECT_EQ(right.data(), text.data());
	EXPECT_EQ(both, std::string_view("value"));
	EXPECT_EQ(both.data(), text.data() + 2U);
}

/**
 * @test
 * @brief view返却APIの一時std::string拒否確認。
 * @details rvalue std::stringからdangling viewを生成し得るAPIがcompile時に拒否されることをtype
 * traitで確認。
 * @pre C++17以降。
 * @post compile時判定のみ。runtime状態の変更なし。
 */
TEST(KetAsciiTest, ViewReturningApisRejectTemporaryString)
{
	static_assert(CanTrim<std::string&>::value, "Trim accepts string lvalue");
	static_assert(CanTrim<const char(&)[2]>::value, "Trim accepts string literal");
	static_assert(!CanTrim<std::string&&>::value, "Trim rejects string rvalue");
	static_assert(!CanTrimLeft<std::string&&>::value, "TrimLeft rejects string rvalue");
	static_assert(!CanTrimRight<std::string&&>::value, "TrimRight rejects string rvalue");
	static_assert(!CanStripPrefix<std::string&&>::value, "StripPrefix rejects string rvalue");
	static_assert(!CanStripSuffix<std::string&&>::value, "StripSuffix rejects string rvalue");
	static_assert(!CanSplitViews<std::string&&>::value, "SplitViews rejects string rvalue");
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
	EXPECT_EQ(separated[0].data(), separated_text.data());
	EXPECT_EQ(separated[1].data(), separated_text.data() + 1U);
	EXPECT_EQ(separated[2].data(), separated_text.data() + 3U);
	EXPECT_EQ(separated[3].data(), separated_text.data() + 4U);
	EXPECT_EQ(separated[4].data(), separated_text.data() + 6U);
}

/**
 * @test
 * @brief SplitViewsのNUL delimiter確認。
 * @details 埋め込みNULをdelimiterとして入力し、byte単位で分割されることとview位置を確認。
 * @pre C++17以降。
 * @post 入力文字列と外部状態の変更なし。戻りviewは元文字列を参照。
 */
TEST(KetAsciiTest, SplitViewsAcceptsNulDelimiter)
{
	const auto text = std::string("a\0b\0", 4U);

	const auto parts = ket::ascii::SplitViews(text, '\0');

	ASSERT_EQ(parts.size(), 3U);
	EXPECT_EQ(parts[0], std::string_view("a", 1U));
	EXPECT_EQ(parts[0].data(), text.data());
	EXPECT_EQ(parts[1], std::string_view("b", 1U));
	EXPECT_EQ(parts[1].data(), text.data() + 2U);
	EXPECT_EQ(parts[2], std::string_view());
	EXPECT_EQ(parts[2].data(), text.data() + 4U);
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
	const auto empty = ket::ascii::Split("", ',');
	text[0] = 'x';

	const auto expected = std::vector<std::string>{"a", "", "b", ""};
	const auto expected_empty = std::vector<std::string>{""};

	EXPECT_EQ(text[0], 'x');
	EXPECT_EQ(parts, expected);
	EXPECT_EQ(empty, expected_empty);
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
	const auto empty_edge_parts = std::vector<std::string_view>{"", "a", ""};
	const auto empty_parts = std::vector<std::string_view>{};
	const auto nul_part = std::string_view("a\0b", 3U);
	const auto nul_delimiter = std::string_view("\0", 1U);
	const auto byte_parts = std::vector<std::string_view>{nul_part, "z"};

	const auto joined = ket::ascii::Join(parts, ",");
	const auto joined_without_delimiter = ket::ascii::Join(parts, "");
	const auto joined_empty_edges = ket::ascii::Join(empty_edge_parts, ",");
	const auto joined_empty = ket::ascii::Join(empty_parts, ",");
	const auto joined_bytes = ket::ascii::Join(byte_parts, nul_delimiter);
	const auto expected_bytes = std::string("a\0b\0z", 5U);

	EXPECT_EQ(joined, std::string("a,b,"));
	EXPECT_EQ(joined_without_delimiter, std::string("ab"));
	EXPECT_EQ(joined_empty_edges, std::string(",a,"));
	EXPECT_EQ(joined_empty, std::string());
	EXPECT_EQ(joined_bytes, expected_bytes);
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
	const auto overlapping = ket::ascii::ReplaceAll("aaa", "aa", "b");
	const auto full_match = ket::ascii::ReplaceAll("abc", "abc", "x");
	const auto expanded_single = ket::ascii::ReplaceAll("a", "a", "aa");
	const auto expanded = ket::ascii::ReplaceAll("a--b--", "--", "::");
	const auto deleted = ket::ascii::ReplaceAll("a--b--", "--", "");
	const auto utf8_text = std::string("caf\xC3\xA9 caf\xC3\xA9", 11U);
	const auto utf8_replaced = ket::ascii::ReplaceAll(utf8_text, "\xC3\xA9", "e");

	EXPECT_EQ(no_match, std::string("abc"));
	EXPECT_EQ(repeated, std::string("aa"));
	EXPECT_EQ(overlapping, std::string("ba"));
	EXPECT_EQ(full_match, std::string("x"));
	EXPECT_EQ(expanded_single, std::string("aa"));
	EXPECT_EQ(expanded, std::string("a::b::"));
	EXPECT_EQ(deleted, std::string("ab"));
	EXPECT_EQ(utf8_replaced, std::string("cafe cafe"));
}

/**
 * @test
 * @brief ReplaceAllの空from拒否確認。
 * @details 空fromを入力し、無限置換になり得る入力をstd::invalid_argumentとして拒否することを確認。
 * @pre C++17以降。
 * @post 例外送出のみ。外部状態の変更なし。
 */
TEST(KetAsciiTest, ReplaceAllRejectsEmptyFrom)
{
	EXPECT_THROW(ket::ascii::ReplaceAll("abc", "", "x"), std::invalid_argument);
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
	const auto lower_boundaries = ket::ascii::ToLower("@AZ[");
	const auto upper_boundaries = ket::ascii::ToUpper("`az{");
	const auto expected_lower = std::string("az_09!\0\xC3\x89", 9U);
	const auto expected_upper = std::string("AZ_09!\0\xC3\x89", 9U);

	EXPECT_EQ(lower, expected_lower);
	EXPECT_EQ(upper, expected_upper);
	EXPECT_EQ(lower_boundaries, std::string("@az["));
	EXPECT_EQ(upper_boundaries, std::string("`AZ{"));
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
