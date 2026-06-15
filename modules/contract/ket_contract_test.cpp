#define NDEBUG

#include "ket_contract.h"

#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	const std::string kAnyDeathOutputRegex;

	int* CountedPointer(int& call_count, int* ptr) noexcept
	{
		++call_count;
		return ptr;
	}

	bool CountedTrue(int& call_count) noexcept
	{
		++call_count;
		return true;
	}

} // namespace

/**
 * @test
 * @brief 契約成立時のmacro正常系確認。
 * @details `NDEBUG`定義下でもprecondition、postcondition、invariantが成功時に戻ることを確認。
 * @pre C++17以降。test translation unitでは`NDEBUG`が定義済み。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContractTest, AcceptsValidContracts)
{
	const auto true_condition = true;

	KET_EXPECTS(true_condition);
	KET_ENSURES(true_condition);
	KET_ASSERT_INVARIANT(true_condition);

	SUCCEED();
}

/**
 * @test
 * @brief precondition違反時のprocess termination確認。
 * @details `KET_EXPECTS(false)`が例外ではなくprocess terminationで終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, TerminatesWhenExpectsFails)
{
	EXPECT_DEATH({ KET_EXPECTS(false); }, kAnyDeathOutputRegex.c_str());
}

/**
 * @test
 * @brief postcondition違反時のprocess termination確認。
 * @details `KET_ENSURES(false)`が例外ではなくprocess terminationで終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, TerminatesWhenEnsuresFails)
{
	EXPECT_DEATH({ KET_ENSURES(false); }, kAnyDeathOutputRegex.c_str());
}

/**
 * @test
 * @brief invariant違反時のprocess termination確認。
 * @details `KET_ASSERT_INVARIANT(false)`が例外ではなくprocess terminationで終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, TerminatesWhenInvariantFails)
{
	EXPECT_DEATH({ KET_ASSERT_INVARIANT(false); }, kAnyDeathOutputRegex.c_str());
}

/**
 * @test
 * @brief null位置情報でもterminateすることの確認。
 * @details `expression`と`file`がnullptrでも診断文字列生成に依存せず終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, TerminatesWithoutDiagnosticPointers)
{
	EXPECT_DEATH(
		{ ket::contract::Expects(false, nullptr, nullptr, 0); }, kAnyDeathOutputRegex.c_str());
}

/**
 * @test
 * @brief 契約式の1回評価確認。
 * @details `KET_EXPECTS`へ副作用を持つ式を渡し、成功時に1回だけ評価されることを確認。
 * @pre C++17以降。
 * @post counterは1へ変化。その他の外部状態の変更なし。
 */
TEST(KetContractTest, EvaluatesConditionOnce)
{
	int call_count = 0;

	KET_EXPECTS(CountedTrue(call_count));

	EXPECT_EQ(call_count, 1);
}

/**
 * @test
 * @brief non-null pointer要求の成功確認。
 * @details `KET_REQUIRE_NON_NULL`がnon-null pointerを同じ値で返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContractTest, ReturnsNonNullPointer)
{
	int value = 42;
	int* const ptr = &value;

	int* const result = KET_REQUIRE_NON_NULL(ptr);

	EXPECT_EQ(result, ptr);
	EXPECT_EQ(*result, 42);
}

/**
 * @test
 * @brief non-null pointer要求の型保持確認。
 * @details const pointerを含む入力について、戻り値が同じpointer型になることをcompile時に確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContractTest, PreservesPointerType)
{
	int value = 42;
	int* ptr = &value;
	const int const_value = 7;
	const int* const_ptr = &const_value;

	static_assert(std::is_same_v<decltype(KET_REQUIRE_NON_NULL(ptr)), int*>,
				  "mutable pointer type is preserved");
	static_assert(std::is_same_v<decltype(KET_REQUIRE_NON_NULL(const_ptr)), const int*>,
				  "const pointer type is preserved");

	int* const result = KET_REQUIRE_NON_NULL(ptr);
	const int* const const_result = KET_REQUIRE_NON_NULL(const_ptr);

	EXPECT_EQ(result, ptr);
	EXPECT_EQ(const_result, const_ptr);
}

/**
 * @test
 * @brief pointer式の1回評価確認。
 * @details
 * `KET_REQUIRE_NON_NULL`へ副作用を持つpointer式を渡し、成功時に1回だけ評価されることを確認。
 * @pre C++17以降。
 * @post counterは1へ変化。その他の外部状態の変更なし。
 */
TEST(KetContractTest, EvaluatesPointerExpressionOnce)
{
	int value = 42;
	int call_count = 0;

	int* const result = KET_REQUIRE_NON_NULL(CountedPointer(call_count, &value));

	EXPECT_EQ(result, &value);
	EXPECT_EQ(call_count, 1);
}

/**
 * @test
 * @brief null pointer要求時のprocess termination確認。
 * @details `KET_REQUIRE_NON_NULL`へnull pointerを渡し、例外ではなくprocess
 * terminationで終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, TerminatesWhenPointerIsNull)
{
	EXPECT_DEATH(
		{
			int* const ptr = nullptr;
			int* const result = KET_REQUIRE_NON_NULL(ptr);
			static_cast<void>(result);
		},
		kAnyDeathOutputRegex.c_str());
}

/**
 * @test
 * @brief 境界内判定の境界値確認。
 * @details 空範囲、先頭、末尾、末尾直後を入力し、`index < size`で判定されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetContractTest, ChecksIndexBounds)
{
	const auto empty_range = ket::contract::IsInBounds(0U, 0U);
	const auto first_element = ket::contract::IsInBounds(0U, 1U);
	const auto last_element = ket::contract::IsInBounds(2U, 3U);
	const auto one_past_last = ket::contract::IsInBounds(3U, 3U);

	EXPECT_FALSE(empty_range);
	EXPECT_TRUE(first_element);
	EXPECT_TRUE(last_element);
	EXPECT_FALSE(one_past_last);
}

/**
 * @test
 * @brief `NDEBUG`定義下でも契約式が評価されることの確認。
 * @details `NDEBUG`が定義済みのtranslation unitで`KET_EXPECTS(false)`が終了することを確認。
 * @pre C++17以降。GoogleTest death testが利用可能。
 * @post 親processの外部状態の変更なし。
 */
TEST(KetContractTest, EvaluatesWhenNdebugIsDefined)
{
	static_assert(
#ifdef NDEBUG
		true,
#else
		false,
#endif
		"NDEBUG is defined for this translation unit");

	EXPECT_DEATH({ KET_EXPECTS(false); }, kAnyDeathOutputRegex.c_str());
}
