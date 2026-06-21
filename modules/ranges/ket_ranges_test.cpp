#include "ket_ranges.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	class ThrowingPredicate
	{
	  public:
		bool operator()(int value) const
		{
			const auto should_throw = value == 20;
			if (should_throw)
			{
				throw std::runtime_error(std::string("predicate failed"));
			}

			return false;
		}
	};

	class MoveOnlyVisitor
	{
	  public:
		MoveOnlyVisitor() = default;

		MoveOnlyVisitor(const MoveOnlyVisitor&) = delete;
		MoveOnlyVisitor& operator=(const MoveOnlyVisitor&) = delete;
		MoveOnlyVisitor(MoveOnlyVisitor&&) noexcept = default;
		MoveOnlyVisitor& operator=(MoveOnlyVisitor&&) noexcept = default;

		[[nodiscard]] int call_count() const
		{
			return call_count_;
		}

		void operator()(std::size_t index, int& value)
		{
			++call_count_;
			value += static_cast<int>(index);
		}

	  private:
		int call_count_ = 0;
	};

	class MoveOnlyPredicate
	{
	  public:
		MoveOnlyPredicate() = default;

		MoveOnlyPredicate(const MoveOnlyPredicate&) = delete;
		MoveOnlyPredicate& operator=(const MoveOnlyPredicate&) = delete;
		MoveOnlyPredicate(MoveOnlyPredicate&&) noexcept = default;
		MoveOnlyPredicate& operator=(MoveOnlyPredicate&&) noexcept = default;

		[[nodiscard]] int call_count() const
		{
			return call_count_;
		}

		bool operator()(const int& value)
		{
			++call_count_;
			return value == 21;
		}

	  private:
		int call_count_ = 0;
	};

	class IndexTypeProbe
	{
	  public:
		template <typename Index, typename Element>
		void operator()(Index index, Element& element) const
		{
			static_assert(std::is_same<Index, std::size_t>::value,
						  "ForEachWithIndex passes std::size_t index");
			static_cast<void>(index);
			static_cast<void>(element);
		}
	};

	struct AdlRange
	{
		int* first;
		int* last;
	};

	int* begin(AdlRange& range)
	{
		return range.first;
	}

	int* end(AdlRange& range)
	{
		return range.last;
	}

} // namespace

/**
 * @test
 * @brief 空rangeのindex付き走査確認。
 * @details 空のstd::vectorを入力し、callableが呼び出されないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexIgnoresEmptyRange)
{
	const std::vector<int> values;
	int call_count = 0;

	ket::ranges::ForEachWithIndex(values,
								  [&call_count](std::size_t index, const int& value)
								  {
									  static_cast<void>(index);
									  static_cast<void>(value);
									  ++call_count;
								  });

	EXPECT_EQ(call_count, 0);
}

/**
 * @test
 * @brief index昇順と要素順の確認。
 * @details 配列を入力し、0始まりindexと要素がrange順にcallableへ渡されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexVisitsIndexesInOrder)
{
	const std::array<int, 3> values = {{10, 20, 30}};
	std::vector<std::size_t> indexes;
	std::vector<int> visited_values;

	ket::ranges::ForEachWithIndex(values,
								  [&indexes, &visited_values](std::size_t index, const int& value)
								  {
									  indexes.push_back(index);
									  visited_values.push_back(value);
								  });

	const std::vector<std::size_t> expected_indexes = {0U, 1U, 2U};
	const std::vector<int> expected_values = {10, 20, 30};

	EXPECT_EQ(indexes, expected_indexes);
	EXPECT_EQ(visited_values, expected_values);
}

/**
 * @test
 * @brief 非const要素参照の保持確認。
 * @details 非constのstd::vectorを入力し、callableが要素参照経由で値を更新できることを確認。
 * @pre C++17以降。
 * @post range要素は期待する値に変化。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexPreservesMutableElementReference)
{
	std::vector<int> values = {10, 20, 30};

	ket::ranges::ForEachWithIndex(values,
								  [](std::size_t index, int& value)
								  {
									  value += static_cast<int>(index);
								  });

	const std::vector<int> expected_values = {10, 21, 32};

	EXPECT_EQ(values, expected_values);
}

/**
 * @test
 * @brief const要素参照の保持確認。
 * @details constのstd::vectorを入力し、callableがconst参照として要素を受け取れることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexPreservesConstElementReference)
{
	const std::vector<int> values = {4, 5};
	int sum = 0;

	ket::ranges::ForEachWithIndex(values,
								  [&sum](std::size_t index, const int& value)
								  {
									  sum += value + static_cast<int>(index);
								  });

	EXPECT_EQ(sum, 10);
}

/**
 * @test
 * @brief callable objectをcopyしないことの確認。
 * @details copy不可のstateful callableを入力し、同じobjectが各要素に対して呼び出されることを確認。
 * @pre C++17以降。
 * @post range要素とcallable側の呼び出し回数は期待する値に変化。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexInvokesMoveOnlyStatefulCallableWithoutCopy)
{
	std::vector<int> values = {10, 20};
	MoveOnlyVisitor visitor;

	ket::ranges::ForEachWithIndex(values, visitor);

	const std::vector<int> expected_values = {10, 21};

	EXPECT_EQ(values, expected_values);
	EXPECT_EQ(visitor.call_count(), 2);
}

/**
 * @test
 * @brief index型の確認。
 * @details callableのtemplate call
 * operatorを使い、渡されるindexがstd::size_tであることをcompile時に確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexPassesSizeTIndex)
{
	int values[] = {1, 2};
	const IndexTypeProbe probe;

	ket::ranges::ForEachWithIndex(values, probe);
}

/**
 * @test
 * @brief C array rangeの走査確認。
 * @details built-in C
 * arrayを入力し、begin/endで走査できるobjectとしてindex付きに処理されることを確認。
 * @pre C++17以降。
 * @post 入力rangeは変更なし。観測用vectorは期待する値へ変化。
 */
TEST(KetRangesTest, ForEachWithIndexAcceptsBuiltInArray)
{
	const int values[] = {7, 8};
	std::vector<int> visited_values;

	ket::ranges::ForEachWithIndex(values,
								  [&visited_values](std::size_t index, const int& value)
								  {
									  visited_values.push_back(value + static_cast<int>(index));
								  });

	const std::vector<int> expected_values = {7, 9};

	EXPECT_EQ(visited_values, expected_values);
}

/**
 * @test
 * @brief ADL begin/end rangeの走査確認。
 * @details free functionのbegin/endだけを持つrangeを入力し、ADLで走査されることを確認。
 * @pre C++17以降。
 * @post range要素は期待する値に変化。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexAcceptsAdlBeginEndRange)
{
	int values[] = {3, 4};
	AdlRange range = {values, values + 2};

	ket::ranges::ForEachWithIndex(range,
								  [](std::size_t index, int& value)
								  {
									  value += static_cast<int>(index);
								  });

	EXPECT_EQ(values[0], 3);
	EXPECT_EQ(values[1], 5);
}

/**
 * @test
 * @brief not found時のout不変確認。
 * @details 条件を満たす要素がないrangeを入力し、falseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outは入力時の値を保持。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, FindIndexIfKeepsOutUnchangedWhenNotFound)
{
	const std::vector<int> values = {10, 20, 30};
	std::size_t index = 99U;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[](int value)
		{
			return value > 30;
		},
		index);

	EXPECT_FALSE(found);
	EXPECT_EQ(index, 99U);
}

/**
 * @test
 * @brief 空rangeのFindIndexIf確認。
 * @details 空のstd::vectorを入力し、predicateを呼ばずfalseを返してoutを変更しないことを確認。
 * @pre C++17以降。
 * @post outとpredicate呼び出し回数は入力時の値を保持。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, FindIndexIfIgnoresEmptyRange)
{
	const std::vector<int> values;
	std::size_t index = 99U;
	int call_count = 0;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[&call_count](int value)
		{
			static_cast<void>(value);
			++call_count;
			return true;
		},
		index);

	EXPECT_FALSE(found);
	EXPECT_EQ(index, 99U);
	EXPECT_EQ(call_count, 0);
}

/**
 * @test
 * @brief 最初の一致indexと短絡確認。
 * @details 複数の一致候補を持つrangeを入力し、最初の一致indexでtrueを返して走査を止めることを確認。
 * @pre C++17以降。
 * @post outは最初の一致indexへ変化。predicate呼び出し回数は最初の一致まで。
 */
TEST(KetRangesTest, FindIndexIfReturnsFirstMatchingIndex)
{
	const std::vector<int> values = {10, 20, 20, 30};
	std::size_t index = 99U;
	int call_count = 0;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[&call_count](int value)
		{
			++call_count;
			return value == 20;
		},
		index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 1U);
	EXPECT_EQ(call_count, 2);
}

/**
 * @test
 * @brief 先頭要素一致時のindex確認。
 * @details 先頭要素が条件を満たすrangeを入力し、index 0でtrueを返して短絡することを確認。
 * @pre C++17以降。
 * @post outは0へ変化。predicate呼び出し回数は1回。
 */
TEST(KetRangesTest, FindIndexIfReturnsZeroForFirstElementMatch)
{
	const std::vector<int> values = {20, 30};
	std::size_t index = 99U;
	int call_count = 0;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[&call_count](int value)
		{
			++call_count;
			return value == 20;
		},
		index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 0U);
	EXPECT_EQ(call_count, 1);
}

/**
 * @test
 * @brief predicate objectをcopyしないことの確認。
 * @details copy不可のstateful predicateを入力し、同じobjectが最初の一致まで呼び出されることを確認。
 * @pre C++17以降。
 * @post outは最初の一致indexへ変化。predicate側の呼び出し回数は最初の一致まで。
 */
TEST(KetRangesTest, FindIndexIfInvokesMoveOnlyStatefulPredicateWithoutCopy)
{
	std::vector<int> values = {10, 21, 30};
	std::size_t index = 99U;
	MoveOnlyPredicate predicate;

	const auto found = ket::ranges::FindIndexIf(values, predicate, index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 1U);
	EXPECT_EQ(predicate.call_count(), 2);
}

/**
 * @test
 * @brief predicateへの非const要素参照保持確認。
 * @details 非constのstd::vectorを入力し、predicateが要素参照経由で値を更新できることを確認。
 * @pre C++17以降。
 * @post outは最初の一致indexへ変化。range要素はpredicateの副作用で期待する値へ変化。
 */
TEST(KetRangesTest, FindIndexIfPreservesMutableElementReference)
{
	std::vector<int> values = {10, 20, 30};
	std::size_t index = 99U;
	const int* observed_address = nullptr;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[&observed_address](int& value)
		{
			const auto matches = value == 20;
			if (matches)
			{
				observed_address = &value;
				value = 21;
			}

			return matches;
		},
		index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 1U);
	EXPECT_EQ(values[1], 21);
	EXPECT_EQ(observed_address, &values[1]);
}

/**
 * @test
 * @brief predicateへのconst要素参照保持確認。
 * @details constのstd::vectorを入力し、predicateがconst参照として要素を受け取れることを確認。
 * @pre C++17以降。
 * @post outは最初の一致indexへ変化。入力rangeは変更なし。
 */
TEST(KetRangesTest, FindIndexIfPreservesConstElementReference)
{
	const std::vector<int> values = {10, 20, 30};
	std::size_t index = 99U;
	const int* observed_address = nullptr;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[&observed_address](const int& value)
		{
			const auto matches = value == 30;
			if (matches)
			{
				observed_address = &value;
			}

			return matches;
		},
		index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 2U);
	EXPECT_EQ(observed_address, &values[2]);
}

/**
 * @test
 * @brief initializer_list rangeの検索確認。
 * @details
 * 名前付きstd::initializer_listを入力し、begin/endで走査できるobjectとして検索されることを確認。
 * @pre C++17以降。
 * @post outは最初の一致indexへ変化。入力rangeは変更なし。
 */
TEST(KetRangesTest, FindIndexIfAcceptsInitializerListObject)
{
	const std::initializer_list<int> values = {4, 5};
	std::size_t index = 99U;

	const auto found = ket::ranges::FindIndexIf(
		values,
		[](const int& value)
		{
			return value == 5;
		},
		index);

	EXPECT_TRUE(found);
	EXPECT_EQ(index, 1U);
}

/**
 * @test
 * @brief predicate例外伝播とout不変確認。
 * @details predicateが例外を送出するrangeを入力し、例外が伝播してoutが変更されないことを確認。
 * @pre C++17以降。
 * @post outは入力時の値を保持。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, FindIndexIfPropagatesPredicateExceptionAndKeepsOutUnchanged)
{
	const std::vector<int> values = {10, 20, 30};
	std::size_t index = 77U;
	const auto find_index = [&values, &index]()
	{
		const auto found = ket::ranges::FindIndexIf(values, ThrowingPredicate{}, index);
		static_cast<void>(found);
	};

	EXPECT_THROW(find_index(), std::runtime_error);
	EXPECT_EQ(index, 77U);
}

/**
 * @test
 * @brief ForEachWithIndexのcallable例外伝播確認。
 * @details callableが例外を送出するrangeを入力し、例外が捕捉されず呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post callableが送出した例外以外の外部状態の変更なし。
 */
TEST(KetRangesTest, ForEachWithIndexPropagatesCallableException)
{
	const std::vector<int> values = {10};
	const auto for_each = [&values]()
	{
		ket::ranges::ForEachWithIndex(values,
									  [](std::size_t index, const int& value)
									  {
										  static_cast<void>(index);
										  static_cast<void>(value);
										  throw std::runtime_error(std::string("callable failed"));
									  });
	};

	EXPECT_THROW(for_each(), std::runtime_error);
}
