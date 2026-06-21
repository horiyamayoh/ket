#include "ket_object.h"

#include <memory>
#include <string> // NOLINT(misc-include-cleaner)
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace
{
	class CopyDisabled : public ket::object::NonCopyable
	{
	  public:
		CopyDisabled() = default;
	};

	class MoveDisabled : public ket::object::NonMovable
	{
	  public:
		MoveDisabled() = default;
	};

	class MoveOnlyValue : public ket::object::MoveOnly
	{
	  public:
		explicit MoveOnlyValue(int initial_value) noexcept : value_(initial_value) {}

		int Value() const noexcept // NOLINT(modernize-use-nodiscard)
		{
			return value_;
		}

	  private:
		int value_ = 0;
	};

	struct EmptyBaseHolder : ket::object::NonCopyable
	{
		char marker = '\0';
	};

	struct PlainCharHolder
	{
		char marker = '\0';
	};

	class ComparableMoveOnly : public ket::object::MoveOnly
	{
	  public:
		explicit ComparableMoveOnly(int value) noexcept : value_(value) {}

		friend bool operator==(const ComparableMoveOnly& left,
							   const ComparableMoveOnly& right) noexcept
		{
			return left.value_ == right.value_;
		}

	  private:
		int value_ = 0;
	};

	class ThrowingResetValue
	{
	  public:
		ThrowingResetValue() noexcept(false)
		{
			MayThrow();
		}

		ThrowingResetValue(const ThrowingResetValue&) = delete;
		ThrowingResetValue& operator=(const ThrowingResetValue&) = delete;

		ThrowingResetValue(ThrowingResetValue&& other) noexcept(false) : value_(other.value_)
		{
			MayThrow();
		}

		ThrowingResetValue& operator=(ThrowingResetValue&& other) noexcept(false)
		{
			value_ = other.value_;
			MayThrow();
			return *this;
		}

	  private:
		static void MayThrow() noexcept(false);

		int value_ = 0;
	};

	ket::object::ResetOnMove<int>& MoveAssignForSelfMoveTest(ket::object::ResetOnMove<int>& target,
															 ket::object::ResetOnMove<int>& source)
	{
		return target = std::move(source);
	}

	static_assert(!std::is_copy_constructible_v<CopyDisabled>,
				  "NonCopyable derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable_v<CopyDisabled>,
				  "NonCopyable derived type is not copy assignable.");
	static_assert(std::is_move_constructible_v<CopyDisabled>,
				  "NonCopyable derived type remains move constructible.");
	static_assert(std::is_move_assignable_v<CopyDisabled>,
				  "NonCopyable derived type remains move assignable.");
	static_assert(!std::is_copy_constructible_v<MoveDisabled>,
				  "NonMovable derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable_v<MoveDisabled>,
				  "NonMovable derived type is not copy assignable.");
	static_assert(!std::is_move_constructible_v<MoveDisabled>,
				  "NonMovable derived type is not move constructible.");
	static_assert(!std::is_move_assignable_v<MoveDisabled>,
				  "NonMovable derived type is not move assignable.");
	static_assert(!std::is_copy_constructible_v<MoveOnlyValue>,
				  "MoveOnly derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable_v<MoveOnlyValue>,
				  "MoveOnly derived type is not copy assignable.");
	static_assert(std::is_move_constructible_v<MoveOnlyValue>,
				  "MoveOnly derived type is move constructible.");
	static_assert(std::is_move_assignable_v<MoveOnlyValue>,
				  "MoveOnly derived type is move assignable.");
	static_assert(!std::is_copy_constructible_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove is not copy constructible.");
	static_assert(!std::is_copy_assignable_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove is not copy assignable.");
	static_assert(std::is_move_constructible_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove is move constructible.");
	static_assert(std::is_move_assignable_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove is move assignable.");
	static_assert(std::is_nothrow_move_constructible_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove<int> move construction is noexcept.");
	static_assert(std::is_nothrow_move_assignable_v<ket::object::ResetOnMove<int>>,
				  "ResetOnMove<int> move assignment is noexcept.");
	static_assert(
		!std::is_nothrow_move_constructible_v<ket::object::ResetOnMove<ThrowingResetValue>>,
		"ResetOnMove<T> move construction is not noexcept when T can throw.");
	static_assert(!std::is_nothrow_move_assignable_v<ket::object::ResetOnMove<ThrowingResetValue>>,
				  "ResetOnMove<T> move assignment is not noexcept when T can throw.");

} // namespace

/**
 * @test
 * @brief copy/move禁止mixinのcompile-time契約確認。
 * @details type traitsでcopy禁止、move禁止、move専用の特殊メンバ生成を確認。
 * @pre C++17以降。最低標準のC++11 compile-only checkでも同じstatic_assertを評価。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetObjectTest, FixesCopyAndMoveTraitsAtCompileTime)
{
	const auto copy_disabled_is_copy_constructible = std::is_copy_constructible_v<CopyDisabled>;
	const auto copy_disabled_is_move_constructible = std::is_move_constructible_v<CopyDisabled>;
	const auto move_disabled_is_move_constructible = std::is_move_constructible_v<MoveDisabled>;
	const auto move_only_is_move_constructible = std::is_move_constructible_v<MoveOnlyValue>;
	const auto reset_on_move_is_move_assignable =
		std::is_move_assignable_v<ket::object::ResetOnMove<int>>;
	const auto throwing_reset_move_constructor_is_noexcept =
		std::is_nothrow_move_constructible_v<ket::object::ResetOnMove<ThrowingResetValue>>;
	const auto throwing_reset_move_assignment_is_noexcept =
		std::is_nothrow_move_assignable_v<ket::object::ResetOnMove<ThrowingResetValue>>;

	EXPECT_FALSE(copy_disabled_is_copy_constructible);
	EXPECT_TRUE(copy_disabled_is_move_constructible);
	EXPECT_FALSE(move_disabled_is_move_constructible);
	EXPECT_TRUE(move_only_is_move_constructible);
	EXPECT_TRUE(reset_on_move_is_move_assignable);
	EXPECT_FALSE(throwing_reset_move_constructor_is_noexcept);
	EXPECT_FALSE(throwing_reset_move_assignment_is_noexcept);
}

/**
 * @test
 * @brief ResetOnMoveのDoxygen例の確認。
 * @details default構築、値構築、mutable参照、const参照の短い利用例を確認。
 * @pre C++17以降。
 * @post 各wrapperは有効で、保持値は期待値と一致。
 */
TEST(KetObjectTest, MatchesDocumentedResetOnMoveExamples)
{
	ket::object::ResetOnMove<int> default_value;
	const auto default_current = default_value.Get();

	ket::object::ResetOnMove<int> initial_value(42);
	const auto initial_current = initial_value.Get();

	default_value.Get() = 13;
	const auto mutable_current = default_value.Get();

	const ket::object::ResetOnMove<int> const_value(24);
	const auto const_current = const_value.Get();

	EXPECT_EQ(default_current, 0);
	EXPECT_EQ(initial_current, 42);
	EXPECT_EQ(mutable_current, 13);
	EXPECT_EQ(const_current, 24);
}

/**
 * @test
 * @brief MoveOnly派生型のmove動作確認。
 * @details MoveOnlyを継承した型をmove構築とmove代入し、move先が値を受け取ることを確認。
 * @pre C++17以降。
 * @post move元は有効だが値は未規定。move先は期待値を保持。
 */
TEST(KetObjectTest, MovesMoveOnlyDerivedType)
{
	MoveOnlyValue source(42);

	MoveOnlyValue constructed(std::move(source));
	const auto constructed_value = constructed.Value();

	MoveOnlyValue assigned(0);
	assigned = std::move(constructed);
	const auto assigned_value = assigned.Value();

	EXPECT_EQ(constructed_value, 42);
	EXPECT_EQ(assigned_value, 42);
}

/**
 * @test
 * @brief ResetOnMoveのmove構築後source reset確認。
 * @details int値を保持するwrapperをmove構築し、move先が元値、move元が値初期化状態になることを確認。
 * @pre C++17以降。
 * @post move元とmove先はともに有効なwrapper。
 */
TEST(KetObjectTest, ResetsSourceAfterMoveConstruction)
{
	ket::object::ResetOnMove<int> source(42);

	ket::object::ResetOnMove<int> destination(std::move(source));
	const auto destination_value = destination.Get();
	// cppcheck-suppress accessMoved
	// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
	const auto source_value = source.Get();

	EXPECT_EQ(destination_value, 42);
	EXPECT_EQ(source_value, 0);
}

/**
 * @test
 * @brief ResetOnMoveのmove代入後source reset確認。
 * @details int値を保持するwrapperをmove代入し、代入先が元値、move元が値初期化状態になることを確認。
 * @pre C++17以降。
 * @post move元とmove先はともに有効なwrapper。
 */
TEST(KetObjectTest, ResetsSourceAfterMoveAssignment)
{
	ket::object::ResetOnMove<int> source(7);
	ket::object::ResetOnMove<int> destination(3);

	destination = std::move(source);
	const auto destination_value = destination.Get();
	// cppcheck-suppress accessMoved
	// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
	const auto source_value = source.Get();

	EXPECT_EQ(destination_value, 7);
	EXPECT_EQ(source_value, 0);
}

/**
 * @test
 * @brief ResetOnMoveの自己move代入確認。
 * @details wrapperへ自己move代入し、保持値を変更しないことを確認。
 * @pre C++17以降。
 * @post 対象wrapperは有効で、保持値は代入前の値を維持。
 */
TEST(KetObjectTest, KeepsValueOnSelfMoveAssignment)
{
	ket::object::ResetOnMove<int> value(11);

	auto& source_alias = value;
	const auto& assigned = MoveAssignForSelfMoveTest(value, source_alias);
	const auto same_object = &assigned == &value;
	// cppcheck-suppress accessMoved
	// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
	const auto current = value.Get();

	EXPECT_TRUE(same_object);
	EXPECT_EQ(current, 11);
}

/**
 * @test
 * @brief ResetOnMoveのmove-only保持値確認。
 * @details unique_ptrを保持するwrapperをmove構築し、move元がnullptrへ戻ることを確認。
 * @pre C++17以降。
 * @post move元とmove先はともに有効なwrapper。
 */
TEST(KetObjectTest, SupportsMoveOnlyResetValue)
{
	ket::object::ResetOnMove<std::unique_ptr<int>> source(std::make_unique<int>(9));

	ket::object::ResetOnMove<std::unique_ptr<int>> destination(std::move(source));
	const auto destination_has_value = static_cast<bool>(destination.Get());
	// cppcheck-suppress accessMoved
	// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
	const auto source_has_value = static_cast<bool>(source.Get());
	const auto destination_value = *destination.Get();

	EXPECT_TRUE(destination_has_value);
	EXPECT_FALSE(source_has_value);
	EXPECT_EQ(destination_value, 9);
}

/**
 * @test
 * @brief ResetOnMoveのmove-only保持値のmove代入確認。
 * @details unique_ptrを保持するwrapperをmove代入し、代入先が元値、move元がnullptrへ戻ることを確認。
 * @pre C++17以降。
 * @post move元とmove先はともに有効なwrapper。
 */
TEST(KetObjectTest, SupportsMoveOnlyResetValueAssignment)
{
	ket::object::ResetOnMove<std::unique_ptr<int>> source(std::make_unique<int>(13));
	ket::object::ResetOnMove<std::unique_ptr<int>> destination(std::make_unique<int>(4));

	destination = std::move(source);
	const auto destination_has_value = static_cast<bool>(destination.Get());
	// cppcheck-suppress accessMoved
	// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
	const auto source_has_value = static_cast<bool>(source.Get());
	const auto destination_value = *destination.Get();

	EXPECT_TRUE(destination_has_value);
	EXPECT_FALSE(source_has_value);
	EXPECT_EQ(destination_value, 13);
}

/**
 * @test
 * @brief 空base最適化のsize確認。
 * @details
 * NonCopyableを空baseとして継承した型とcharだけを持つ型を比較し、size増加がないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetObjectTest, KeepsEmptyBaseSize)
{
	const auto empty_base_size = sizeof(EmptyBaseHolder);
	const auto plain_size = sizeof(PlainCharHolder);

	EXPECT_EQ(empty_base_size, plain_size);
}

/**
 * @test
 * @brief 比較演算を利用側で定義できることの確認。
 * @details mixin自体が比較演算を宣言しない前提で、派生型のoperator==が通常通り使えることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetObjectTest, AllowsUserDefinedComparisonAlongsideMixin)
{
	const ComparableMoveOnly first(1);
	const ComparableMoveOnly same(1);
	const ComparableMoveOnly different(2);

	const auto first_equals_same = first == same;
	const auto first_equals_different = first == different;

	EXPECT_TRUE(first_equals_same);
	EXPECT_FALSE(first_equals_different);
}
