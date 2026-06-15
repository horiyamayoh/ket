#include "ket_tuple.h"

#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	struct TransformToStableTypes
	{
		long operator()(int value) const
		{
			return static_cast<long>(value) * 10L;
		}

		std::string operator()(char value) const
		{
			return std::string{value};
		}
	};

} // namespace

/**
 * @test
 * @brief 空tupleの反復と変換確認。
 * @details 空tupleを入力し、ForEachがcallableを呼ばず、Transformが空tupleを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTupleTest, HandlesEmptyTuple)
{
	auto empty = std::tuple<>();
	int call_count = 0;

	ket::tuple::ForEach(empty,
						[&call_count](auto&& value)
						{
							static_cast<void>(value);
							++call_count;
						});
	auto transformed = ket::tuple::Transform(empty,
											 [](auto&& value)
											 {
												 return value;
											 });

	static_assert(std::is_same_v<decltype(transformed), std::tuple<>>,
				  "empty transform returns empty tuple");
	const auto transformed_size = std::tuple_size<decltype(transformed)>::value;

	EXPECT_EQ(call_count, 0);
	EXPECT_EQ(transformed, std::tuple<>());
	EXPECT_EQ(transformed_size, 0U);
}

/**
 * @test
 * @brief 異種tupleのindex順反復確認。
 * @details int、char、std::stringを含むtupleを入力し、各要素がindex順にcallableへ渡ることを確認。
 * @pre C++17以降。
 * @post traceは期待する呼び出し順を表す文字列へ変化。テスト対象tupleの変更なし。
 */
TEST(KetTupleTest, ForEachVisitsHeterogeneousElementsInIndexOrder)
{
	auto values = std::make_tuple(1, 'x', std::string("yz"));
	std::string trace;

	ket::tuple::ForEach(values,
						[&trace](const auto& value)
						{
							using Value = std::decay_t<decltype(value)>;
							if constexpr (std::is_same_v<Value, int>)
							{
								trace += "int:";
								trace += std::to_string(value);
								trace += ';';
							}
							else if constexpr (std::is_same_v<Value, char>)
							{
								trace += "char:";
								trace.push_back(value);
								trace += ';';
							}
							else
							{
								trace += "string:";
								trace += value;
								trace += ';';
							}
						});

	EXPECT_EQ(trace, std::string("int:1;char:x;string:yz;"));
}

/**
 * @test
 * @brief 異種tupleの変換戻り型確認。
 * @details intとcharを含むtupleを入力し、各戻り値の型を保持したstd::tupleが返ることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetTupleTest, TransformReturnsTupleWithCallableResultTypes)
{
	const auto values = std::make_tuple(7, 'A');

	auto transformed = ket::tuple::Transform(values, TransformToStableTypes{});

	static_assert(std::is_same_v<decltype(transformed), std::tuple<long, std::string>>,
				  "transform preserves per-element callable result types");
	EXPECT_EQ(std::get<0>(transformed), 70L);
	EXPECT_EQ(std::get<1>(transformed), std::string("A"));
}

/**
 * @test
 * @brief const tuple要素のcv修飾保持確認。
 * @details const tupleを入力し、ForEachとTransformのcallableがconst参照要素を受け取ることを確認。
 * @pre C++17以降。
 * @post all_constは全要素がconstとして渡ったことを示す値へ変化。テスト対象tupleの変更なし。
 */
TEST(KetTupleTest, PreservesConstTupleElements)
{
	const auto values = std::make_tuple(3, std::string("abc"));
	bool all_const = true;

	ket::tuple::ForEach(values,
						[&all_const](auto&& value)
						{
							using Element = std::remove_reference_t<decltype(value)>;
							all_const = all_const && std::is_const_v<Element>;
						});
	auto transformed = ket::tuple::Transform(values,
											 [](const auto& value)
											 {
												 return value;
											 });

	EXPECT_TRUE(all_const);
	EXPECT_EQ(std::get<0>(transformed), 3);
	EXPECT_EQ(std::get<1>(transformed), std::string("abc"));
}

/**
 * @test
 * @brief reference要素の反復と変換確認。
 * @details std::tieで作ったreference
 * tupleを入力し、ForEachの変更とTransformの参照戻り値が元変数へ届くことを確認。
 * @pre C++17以降。
 * @post firstとsecondはForEachと戻りtuple経由の代入結果を反映した値へ変化。
 */
TEST(KetTupleTest, HandlesReferenceElements)
{
	int first = 1;
	int second = 5;
	auto references = std::tie(first, second);

	ket::tuple::ForEach(references,
						[](int& value)
						{
							value += 10;
						});
	auto transformed = ket::tuple::Transform(references,
											 [](int& value) -> int&
											 {
												 value += 1;
												 return value;
											 });
	std::get<0>(transformed) = 42;

	static_assert(std::is_same_v<decltype(transformed), std::tuple<int&, int&>>,
				  "reference-returning transform keeps tuple reference elements");
	EXPECT_EQ(first, 42);
	EXPECT_EQ(second, 16);
}

/**
 * @test
 * @brief Transformの呼び出し順確認。
 * @details 3要素tupleを入力し、変換結果と副作用の記録がindex順に一致することを確認。
 * @pre C++17以降。
 * @post orderはcallableの呼び出し順を表す値列へ変化。テスト対象tupleの変更なし。
 */
TEST(KetTupleTest, TransformInvokesCallableInIndexOrder)
{
	const auto values = std::make_tuple(1, 2, 3);
	std::vector<int> order;

	auto transformed = ket::tuple::Transform(values,
											 [&order](int value)
											 {
												 order.push_back(value);
												 return value * 2;
											 });
	const auto expected_order = std::vector<int>{1, 2, 3};
	const auto expected_transformed = std::make_tuple(2, 4, 6);

	EXPECT_EQ(order, expected_order);
	EXPECT_EQ(transformed, expected_transformed);
}

/**
 * @test
 * @brief callable例外の伝播確認。
 * @details
 * ForEachとTransformの2要素目で例外を送出し、例外が握りつぶされず呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post seenは例外発生までのindex順呼び出しを表す値列へ変化。テスト対象tupleの変更なし。
 */
TEST(KetTupleTest, PropagatesCallableExceptions)
{
	const auto values = std::make_tuple(1, 2, 3);
	std::vector<int> seen;

	EXPECT_THROW(ket::tuple::ForEach(values,
									 [&seen](int value)
									 {
										 seen.push_back(value);
										 const auto should_throw = value == 2;
										 if (should_throw)
										 {
											 throw std::runtime_error("tuple for-each stop");
										 }
									 }),
				 std::runtime_error);

	const auto expected_for_each_seen = std::vector<int>{1, 2};
	EXPECT_EQ(seen, expected_for_each_seen);

	seen.clear();

	EXPECT_THROW(
		{
			const auto unused =
				ket::tuple::Transform(values,
									  [&seen](int value)
									  {
										  seen.push_back(value);
										  const auto should_throw = value == 2;
										  if (should_throw)
										  {
											  throw std::runtime_error("tuple transform stop");
										  }

										  return value;
									  });
			static_cast<void>(unused);
		},
		std::runtime_error);

	const auto expected_transform_seen = std::vector<int>{1, 2};
	EXPECT_EQ(seen, expected_transform_seen);
}
