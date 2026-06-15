#include "ket_ranges.h"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
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
 * @brief predicate例外伝播とout不変確認。
 * @details predicateが例外を送出するrangeを入力し、例外が伝播してoutが変更されないことを確認。
 * @pre C++17以降。
 * @post outは入力時の値を保持。その他の外部状態の変更なし。
 */
TEST(KetRangesTest, FindIndexIfPropagatesPredicateExceptionAndKeepsOutUnchanged)
{
	const std::vector<int> values = {10, 20, 30};
	std::size_t index = 77U;

	EXPECT_THROW(ket::ranges::FindIndexIf(values, ThrowingPredicate{}, index), std::runtime_error);
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

	EXPECT_THROW(ket::ranges::ForEachWithIndex(values,
											   [](std::size_t index, const int& value)
											   {
												   static_cast<void>(index);
												   static_cast<void>(value);
												   throw std::runtime_error(
													   std::string("callable failed"));
											   }),
				 std::runtime_error);
}
