#include "ket_lang.h"

#include <memory>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace
{
	struct MutableValue
	{
		int number = 0;
	};

	static_assert(ket::lang::ArraySize("ket") == 4U, "string literal size is constexpr");

} // namespace

/**
 * @test
 * @brief 未使用値の明示破棄確認。
 * @details 空引数と複数型の値をIgnoreUnusedへ渡し、入力値が変更されないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetLangTest, IgnoresUnusedValues)
{
	const int value = 42;
	const char* const text = "lang";
	const auto object = MutableValue{7};

	ket::lang::IgnoreUnused();
	ket::lang::IgnoreUnused(value, text, object);

	EXPECT_EQ(value, 42);
	EXPECT_STREQ(text, "lang");
	EXPECT_EQ(object.number, 7);
}

/**
 * @test
 * @brief raw配列の要素数取得確認。
 * @details 1要素、複数要素、const要素、文字配列を入力し、compile-timeの配列長を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetLangTest, GetsRawArraySize)
{
	const int one[1] = {};
	const int values[3] = {};
	const double const_values[2] = {};
	const char text[5] = {};

	const auto one_size = ket::lang::ArraySize(one);
	const auto values_size = ket::lang::ArraySize(values);
	const auto const_values_size = ket::lang::ArraySize(const_values);
	const auto text_size = ket::lang::ArraySize(text);

	EXPECT_EQ(one_size, 1U);
	EXPECT_EQ(values_size, 3U);
	EXPECT_EQ(const_values_size, 2U);
	EXPECT_EQ(text_size, 5U);
}

/**
 * @test
 * @brief lvalueのconst参照化確認。
 * @details mutableなlvalueをAsConstへ渡し、同じobjectを指すconst参照が返ることを確認。
 * @pre C++17以降。
 * @post 入力objectの値と外部状態の変更なし。
 */
TEST(KetLangTest, ConvertsMutableLvalueToConstReference)
{
	MutableValue value{123};

	const auto& const_value = ket::lang::AsConst(value);
	using ConstValueReference = decltype(ket::lang::AsConst(std::declval<MutableValue&>()));

	static_assert(std::is_same_v<ConstValueReference, const MutableValue&>,
				  "AsConst returns const lvalue reference");

	EXPECT_EQ(std::addressof(const_value), std::addressof(value));
	EXPECT_EQ(const_value.number, 123);
}

/**
 * @test
 * @brief const lvalueのconst参照保持確認。
 * @details constなlvalueをAsConstへ渡し、型と参照先が変わらないことを確認。
 * @pre C++17以降。
 * @post 入力objectの値と外部状態の変更なし。
 */
TEST(KetLangTest, KeepsConstLvalueAsConstReference)
{
	const int value = 17;

	const auto& const_value = ket::lang::AsConst(value);
	using ConstValueReference = decltype(ket::lang::AsConst(std::declval<const int&>()));

	static_assert(std::is_same_v<ConstValueReference, const int&>,
				  "AsConst keeps const lvalue reference");

	EXPECT_EQ(std::addressof(const_value), std::addressof(value));
	EXPECT_EQ(const_value, 17);
}
