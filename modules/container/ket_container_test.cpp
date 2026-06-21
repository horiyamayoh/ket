#include "ket_container.h"

#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	class CountingFactory
	{
	  public:
		CountingFactory(int& call_count, int value) : call_count_(&call_count), value_(value) {}

		int operator()() const
		{
			++(*call_count_);
			return value_;
		}

	  private:
		int* call_count_;
		int value_;
	};

	class ThrowingFactory
	{
	  public:
		int operator()() const
		{
			throw std::runtime_error("factory failed");
		}
	};

	class KeyMutatingFactory
	{
	  public:
		KeyMutatingFactory(std::string& key, int value) : key_(&key), value_(value) {}

		int operator()() const
		{
			*key_ = "two";
			return value_;
		}

	  private:
		std::string* key_;
		int value_;
	};

	class ThrowingPredicate
	{
	  public:
		bool operator()(int value) const
		{
			const auto should_throw = value == 2;
			if (should_throw)
			{
				throw std::runtime_error("predicate failed");
			}

			return false;
		}
	};

	class ThrowingEqualityValue
	{
	  public:
		explicit ThrowingEqualityValue(int value) : value_(value) {}

		friend bool operator==(ThrowingEqualityValue left, ThrowingEqualityValue right)
		{
			const auto should_throw = left.value_ == right.value_;
			if (should_throw)
			{
				throw std::runtime_error("equality failed");
			}

			return false;
		}

	  private:
		int value_;
	};

	struct LookupKey
	{
		int value;
	};

	class ControlledLess
	{
	  public:
		explicit ControlledLess(bool& should_throw) : should_throw_(&should_throw) {}

		bool operator()(LookupKey left, LookupKey right) const
		{
			const auto should_throw = *should_throw_;
			if (should_throw)
			{
				throw std::runtime_error("lookup failed");
			}

			return left.value < right.value;
		}

	  private:
		bool* should_throw_;
	};

	class CopyControlledValue
	{
	  public:
		CopyControlledValue(int value, bool& should_throw)
			: value_(value), should_throw_(&should_throw)
		{
		}

		CopyControlledValue(const CopyControlledValue& other)
			: value_(other.value_), should_throw_(other.should_throw_)
		{
			const auto should_throw = *should_throw_;
			if (should_throw)
			{
				throw std::runtime_error("copy failed");
			}
		}

		CopyControlledValue& operator=(const CopyControlledValue&) = delete;

	  private:
		int value_;
		bool* should_throw_;
	};

	class CompareControlledValue
	{
	  public:
		CompareControlledValue(int value, bool& should_throw)
			: value_(value), should_throw_(&should_throw)
		{
		}

		friend bool operator<(const CompareControlledValue& left,
							  const CompareControlledValue& right)
		{
			const auto should_throw = *left.should_throw_ || *right.should_throw_;
			if (should_throw)
			{
				throw std::runtime_error("compare failed");
			}

			return left.value_ < right.value_;
		}

		friend bool operator==(const CompareControlledValue& left,
							   const CompareControlledValue& right)
		{
			return left.value_ == right.value_;
		}

	  private:
		int value_;
		bool* should_throw_;
	};

} // namespace

/**
 * @test
 * @brief sequence内の値存在確認。
 * @details vectorに存在する値、存在しない値、空vectorを入力し、Containsの真偽を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContainerTest, ContainsFindsValuesInSequence)
{
	const std::vector<int> values = {1, 2, 3};
	const std::vector<int> empty_values;

	const auto has_two = ket::container::Contains(values, 2);
	const auto has_four = ket::container::Contains(values, 4);
	const auto empty_has_one = ket::container::Contains(empty_values, 1);

	EXPECT_TRUE(has_two);
	EXPECT_FALSE(has_four);
	EXPECT_FALSE(empty_has_one);
}

/**
 * @test
 * @brief map key存在確認。
 * @details std::mapとstd::unordered_mapに対してkeyあり、keyなし、空mapの結果を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContainerTest, ContainsKeyFindsKeysInAssociativeContainers)
{
	const std::map<std::string, int> ordered = {{"one", 1}, {"two", 2}};
	const std::unordered_map<std::string, int> hashed = {{"one", 1}, {"two", 2}};
	const std::map<std::string, int> empty_ordered;

	const auto ordered_has_one = ket::container::ContainsKey(ordered, std::string("one"));
	const auto ordered_has_three = ket::container::ContainsKey(ordered, std::string("three"));
	const auto hashed_has_two = ket::container::ContainsKey(hashed, std::string("two"));
	const auto empty_has_one = ket::container::ContainsKey(empty_ordered, std::string("one"));

	EXPECT_TRUE(ordered_has_one);
	EXPECT_FALSE(ordered_has_three);
	EXPECT_TRUE(hashed_has_two);
	EXPECT_FALSE(empty_has_one);
}

/**
 * @test
 * @brief const/non-const mapのAtOrNull確認。
 * @details keyありではmap内要素pointerを返し、keyなしではnullptrを返すことを確認。
 * @pre C++17以降。
 * @post non-const pointer経由で対象値のみ変更。map構造と他の外部状態の変更なし。
 */
TEST(KetContainerTest, AtOrNullReturnsPointerOrNull)
{
	std::map<std::string, int> values = {{"one", 1}, {"two", 2}};
	const std::map<std::string, int>& const_values = values;

	auto* const mutable_value = ket::container::AtOrNull(values, std::string("one"));
	const auto* const const_value = ket::container::AtOrNull(const_values, std::string("two"));
	auto* const missing_value = ket::container::AtOrNull(values, std::string("three"));

	ASSERT_NE(mutable_value, nullptr);
	ASSERT_NE(const_value, nullptr);
	EXPECT_EQ(*mutable_value, 1);
	EXPECT_EQ(*const_value, 2);
	EXPECT_EQ(missing_value, nullptr);

	*mutable_value = 10;
	const auto stored_value = values.find("one")->second;

	EXPECT_EQ(stored_value, 10);
}

/**
 * @test
 * @brief map値のdefault fallback取得確認。
 * @details keyありではmap内の値を返し、keyなしでは渡したdefault値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContainerTest, AtOrReturnsStoredValueOrDefaultValue)
{
	const std::map<std::string, int> values = {{"one", 1}, {"two", 2}};

	const auto found_value = ket::container::AtOr(values, std::string("two"), 0);
	const auto missing_value = ket::container::AtOr(values, std::string("three"), 0);

	EXPECT_EQ(found_value, 2);
	EXPECT_EQ(missing_value, 0);
}

/**
 * @test
 * @brief AtOrのcopy例外伝播確認。
 * @details keyありでmap内要素のcopyが例外を送出した場合、AtOrから呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post map内容は入力時の要素を保持。例外制御flag以外の外部状態変更なし。
 */
TEST(KetContainerTest, AtOrPropagatesStoredValueCopyException)
{
	bool should_throw = false;
	std::map<std::string, CopyControlledValue> values;
	values.emplace(std::string("one"), CopyControlledValue(1, should_throw));
	should_throw = true;
	const auto copy_should_throw = should_throw;

	ASSERT_TRUE(copy_should_throw);
	EXPECT_THROW(
		{
			const auto result = ket::container::AtOr(
				values, std::string("one"), CopyControlledValue(0, should_throw));
			static_cast<void>(result);
		},
		std::runtime_error);
	const auto size = values.size();

	EXPECT_EQ(size, 1U);
}

/**
 * @test
 * @brief AtOrCreateの既存key取得確認。
 * @details keyが存在する場合、factoryを呼ばずに既存要素への参照を返すことを確認。
 * @pre C++17以降。
 * @post 既存要素の値のみ参照経由で変更。map構造とfactory呼び出し回数は変化なし。
 */
TEST(KetContainerTest, AtOrCreateReturnsExistingValueWithoutCallingFactory)
{
	std::map<std::string, int> values = {{"one", 1}};
	int call_count = 0;

	int& result =
		ket::container::AtOrCreate(values, std::string("one"), CountingFactory(call_count, 7));
	result = 3;
	const auto stored_value = values.find("one")->second;

	EXPECT_EQ(call_count, 0);
	EXPECT_EQ(stored_value, 3);
}

/**
 * @test
 * @brief AtOrCreateのmissing key生成確認。
 * @details keyが存在しない場合、factoryを1回だけ呼び、生成値をmapへ挿入することを確認。
 * @pre C++17以降。
 * @post missing keyの要素が1件挿入され、factory呼び出し回数が1増加。
 */
TEST(KetContainerTest, AtOrCreateCallsFactoryOnceForMissingKey)
{
	std::map<std::string, int> values;
	int call_count = 0;

	int& result =
		ket::container::AtOrCreate(values, std::string("one"), CountingFactory(call_count, 7));
	result = 8;
	const auto stored_value = values.find("one")->second;
	const auto size = values.size();

	EXPECT_EQ(call_count, 1);
	EXPECT_EQ(stored_value, 8);
	EXPECT_EQ(size, 1U);
}

/**
 * @test
 * @brief AtOrCreateのkey materialization順序確認。
 * @details factoryが外側のkey変数を変更しても、挿入keyは呼び出し時のkeyで固定されることを確認。
 * @pre C++17以降。factoryは対象mapを変更しない。
 * @post missing keyの要素が呼び出し時keyで1件挿入され、外側のkey変数だけがfactoryにより変更。
 */
TEST(KetContainerTest, AtOrCreateMaterializesKeyBeforeCallingFactory)
{
	std::map<std::string, int> values;
	std::string key = "one";

	int& result = ket::container::AtOrCreate(values, key, KeyMutatingFactory(key, 7));
	result = 8;
	const auto has_original_key = ket::container::ContainsKey(values, std::string("one"));
	const auto has_mutated_key = ket::container::ContainsKey(values, std::string("two"));
	const auto stored_value = values.find("one")->second;

	EXPECT_TRUE(has_original_key);
	EXPECT_FALSE(has_mutated_key);
	EXPECT_EQ(stored_value, 8);
	EXPECT_EQ(key, std::string("two"));
}

/**
 * @test
 * @brief AtOrCreateのfactory例外伝播確認。
 * @details factoryが例外を送出した場合、例外が呼び出し元へ伝播し、mapへ要素を挿入しないことを確認。
 * @pre C++17以降。
 * @post mapは空のまま。factory例外以外の外部状態変更なし。
 */
TEST(KetContainerTest, AtOrCreatePropagatesFactoryException)
{
	std::map<std::string, int> values;

	EXPECT_THROW(
		{ ket::container::AtOrCreate(values, std::string("one"), ThrowingFactory{}); },
		std::runtime_error);
	const auto size = values.size();

	EXPECT_EQ(size, 0U);
}

/**
 * @test
 * @brief sequence等値比較の例外伝播確認。
 * @details Containsの等値比較が例外を送出した場合、呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post sequence内容は入力時の要素を保持。例外以外の外部状態変更なし。
 */
TEST(KetContainerTest, ContainsPropagatesEqualityException)
{
	const std::vector<ThrowingEqualityValue> values = {ThrowingEqualityValue(1)};

	EXPECT_THROW(
		{
			const auto result = ket::container::Contains(values, ThrowingEqualityValue(1));
			static_cast<void>(result);
		},
		std::runtime_error);
	const auto size = values.size();

	EXPECT_EQ(size, 1U);
}

/**
 * @test
 * @brief lookup例外伝播確認。
 * @details map comparatorがlookup中に例外を送出した場合、lookup系APIから伝播することを確認。
 * @pre C++17以降。
 * @post map内容は入力時の要素を保持。例外制御flag以外の外部状態変更なし。
 */
TEST(KetContainerTest, PropagatesLookupExceptionFromMapApis)
{
	bool contains_should_throw = false;
	std::map<LookupKey, int, ControlledLess> contains_values(
		(ControlledLess(contains_should_throw)));
	contains_values.emplace(LookupKey{1}, 10);
	contains_should_throw = true;
	const auto contains_lookup_should_throw = contains_should_throw;

	bool at_or_null_should_throw = false;
	std::map<LookupKey, int, ControlledLess> at_or_null_values(
		(ControlledLess(at_or_null_should_throw)));
	at_or_null_values.emplace(LookupKey{1}, 10);
	at_or_null_should_throw = true;
	const auto at_or_null_lookup_should_throw = at_or_null_should_throw;

	bool const_at_or_null_should_throw = false;
	std::map<LookupKey, int, ControlledLess> const_at_or_null_source(
		(ControlledLess(const_at_or_null_should_throw)));
	const_at_or_null_source.emplace(LookupKey{1}, 10);
	const std::map<LookupKey, int, ControlledLess>& const_at_or_null_values =
		const_at_or_null_source;
	const_at_or_null_should_throw = true;
	const auto const_at_or_null_lookup_should_throw = const_at_or_null_should_throw;

	bool at_or_should_throw = false;
	std::map<LookupKey, int, ControlledLess> at_or_values((ControlledLess(at_or_should_throw)));
	at_or_values.emplace(LookupKey{1}, 10);
	at_or_should_throw = true;
	const auto at_or_lookup_should_throw = at_or_should_throw;

	bool at_or_create_should_throw = false;
	std::map<LookupKey, int, ControlledLess> at_or_create_values(
		(ControlledLess(at_or_create_should_throw)));
	at_or_create_values.emplace(LookupKey{1}, 10);
	at_or_create_should_throw = true;
	const auto at_or_create_lookup_should_throw = at_or_create_should_throw;
	int at_or_create_call_count = 0;

	ASSERT_TRUE(contains_lookup_should_throw);
	ASSERT_TRUE(at_or_null_lookup_should_throw);
	ASSERT_TRUE(const_at_or_null_lookup_should_throw);
	ASSERT_TRUE(at_or_lookup_should_throw);
	ASSERT_TRUE(at_or_create_lookup_should_throw);
	EXPECT_THROW(
		{
			const auto result = ket::container::ContainsKey(contains_values, LookupKey{1});
			static_cast<void>(result);
		},
		std::runtime_error);
	EXPECT_THROW(
		{
			const auto result = ket::container::AtOrNull(at_or_null_values, LookupKey{1});
			static_cast<void>(result);
		},
		std::runtime_error);
	EXPECT_THROW(
		{
			const auto result = ket::container::AtOrNull(const_at_or_null_values, LookupKey{1});
			static_cast<void>(result);
		},
		std::runtime_error);
	EXPECT_THROW(
		{
			const auto result = ket::container::AtOr(at_or_values, LookupKey{1}, 0);
			static_cast<void>(result);
		},
		std::runtime_error);
	EXPECT_THROW(
		{
			const auto* const result = &ket::container::AtOrCreate(
				at_or_create_values, LookupKey{1}, CountingFactory(at_or_create_call_count, 7));
			static_cast<void>(result);
		},
		std::runtime_error);

	const auto contains_size = contains_values.size();
	const auto at_or_null_size = at_or_null_values.size();
	const auto const_at_or_null_size = const_at_or_null_source.size();
	const auto at_or_size = at_or_values.size();
	const auto at_or_create_size = at_or_create_values.size();

	EXPECT_EQ(contains_size, 1U);
	EXPECT_EQ(at_or_null_size, 1U);
	EXPECT_EQ(const_at_or_null_size, 1U);
	EXPECT_EQ(at_or_size, 1U);
	EXPECT_EQ(at_or_create_size, 1U);
	EXPECT_EQ(at_or_create_call_count, 0);
}

/**
 * @test
 * @brief sequence要素の条件削除件数確認。
 * @details 偶数削除、削除対象なし、空sequenceを入力し、EraseIfが削除件数を返すことを確認。
 * @pre C++17以降。
 * @post 条件一致要素だけ削除され、残存要素の相対順序を保持。
 */
TEST(KetContainerTest, EraseIfRemovesMatchingElementsAndReturnsCount)
{
	std::vector<int> values = {1, 2, 3, 4, 5};
	std::vector<int> no_matches = {1, 3, 5};
	std::vector<int> empty_values;

	const auto removed = ket::container::EraseIf(values,
												 [](int value)
												 {
													 return (value % 2) == 0;
												 });
	const auto removed_none = ket::container::EraseIf(no_matches,
													  [](int value)
													  {
														  return (value % 2) == 0;
													  });
	const auto removed_empty = ket::container::EraseIf(empty_values,
													   [](int value)
													   {
														   return (value % 2) == 0;
													   });
	const std::vector<int> expected_values = {1, 3, 5};

	EXPECT_EQ(removed, 2U);
	EXPECT_EQ(values, expected_values);
	EXPECT_EQ(removed_none, 0U);
	EXPECT_EQ(no_matches, expected_values);
	EXPECT_EQ(removed_empty, 0U);
}

/**
 * @test
 * @brief EraseIfのpredicate例外伝播確認。
 * @details predicateが例外を送出した場合、EraseIfから呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post 例外伝播後のsequence状態は標準erase-remove手順の例外時規則に従う。
 */
TEST(KetContainerTest, EraseIfPropagatesPredicateException)
{
	std::vector<int> values = {1, 2, 3};

	EXPECT_THROW(
		{
			const auto removed = ket::container::EraseIf(values, ThrowingPredicate{});
			static_cast<void>(removed);
		},
		std::runtime_error);
}

/**
 * @test
 * @brief vector-like値列のsortと重複削除確認。
 * @details 重複を含む未整列vectorと空vectorを入力し、昇順の一意な値列になることを確認。
 * @pre C++17以降。
 * @post 入力vectorは昇順かつ重複なし。空vectorは空のまま。
 */
TEST(KetContainerTest, SortUniqueSortsAndRemovesDuplicates)
{
	std::vector<int> values = {3, 1, 2, 3, 2, 1};
	std::vector<int> empty_values;

	ket::container::SortUnique(values);
	ket::container::SortUnique(empty_values);
	const std::vector<int> expected_values = {1, 2, 3};
	const auto empty_values_is_empty = empty_values.empty();

	EXPECT_EQ(values, expected_values);
	EXPECT_TRUE(empty_values_is_empty);
}

/**
 * @test
 * @brief SortUniqueの比較例外伝播確認。
 * @details sort中の比較が例外を送出した場合、SortUniqueから呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post 例外伝播後のsequence状態は標準sort手順の例外時規則に従う。
 */
TEST(KetContainerTest, SortUniquePropagatesComparisonException)
{
	bool should_throw = false;
	std::vector<CompareControlledValue> values;
	values.emplace_back(2, should_throw);
	values.emplace_back(1, should_throw);
	should_throw = true;
	const auto compare_should_throw = should_throw;

	ASSERT_TRUE(compare_should_throw);
	EXPECT_THROW({ ket::container::SortUnique(values); }, std::runtime_error);
}
