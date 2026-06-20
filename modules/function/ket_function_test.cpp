#include "ket_function.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

namespace
{
	struct IntHandler
	{
		int operator()(int value) const
		{
			return value + 1;
		}
	};

	struct StringSizeHandler
	{
		int operator()(const std::string& value) const
		{
			return static_cast<int>(value.size());
		}
	};

	struct ThrowingHandler
	{
		int operator()(int value) const
		{
			static_cast<void>(value);
			throw std::runtime_error("handler failure");
		}
	};

	struct MoveOnlyHandler
	{
		MoveOnlyHandler() = default;
		MoveOnlyHandler(const MoveOnlyHandler&) = delete;
		MoveOnlyHandler& operator=(const MoveOnlyHandler&) = delete;
		MoveOnlyHandler(MoveOnlyHandler&&) noexcept = default;
		MoveOnlyHandler& operator=(MoveOnlyHandler&&) noexcept = default;

		int operator()(int value) const noexcept
		{
			return value + 1;
		}
	};

	struct StatefulHandler
	{
		explicit StatefulHandler(int value) : value_(value) {}

		void SetValue(int value)
		{
			value_ = value;
		}

		int operator()(int input) const noexcept
		{
			return value_ + input;
		}

	  private:
		int value_;
	};

	using CopyableOverload = ket::function::Overload<IntHandler>;
	using MoveOnlyOverload = ket::function::Overload<MoveOnlyHandler>;
	using ThrowingOverload = ket::function::Overload<ThrowingHandler>;
	using GeneratedOverload = decltype(ket::function::MakeOverload(IntHandler{}));
	using DeductedOverload = decltype(ket::function::Overload{IntHandler{}});

	static_assert(std::is_copy_constructible_v<CopyableOverload>,
				  "copyable callable keeps copy construction");
	static_assert(std::is_move_constructible_v<CopyableOverload>,
				  "copyable callable keeps move construction");
	static_assert(!std::is_copy_constructible_v<MoveOnlyOverload>,
				  "move-only callable disables copy construction");
	static_assert(std::is_move_constructible_v<MoveOnlyOverload>,
				  "move-only callable keeps move construction");
	static_assert(std::is_same_v<GeneratedOverload, ket::function::Overload<IntHandler>>,
				  "MakeOverload stores decayed callable types");
	static_assert(std::is_same_v<DeductedOverload, ket::function::Overload<IntHandler>>,
				  "Overload has a C++17 deduction guide");
	static_assert(noexcept(ket::function::MakeOverload(MoveOnlyHandler{})),
				  "MakeOverload preserves nothrow construction");
	static_assert(noexcept(std::declval<const MoveOnlyOverload&>()(1)),
				  "noexcept handler keeps noexcept call");
	static_assert(!noexcept(std::declval<const ThrowingOverload&>()(1)),
				  "potentially throwing handler keeps throwing call");
	static_assert(noexcept(ket::function::Noop{}(1, "ignored")), "Noop call operator is noexcept");

	constexpr bool NoopCanBeEvaluated()
	{
		ket::function::Noop{}();
		return true;
	}

	static_assert(NoopCanBeEvaluated(), "Noop can be evaluated in constexpr context");

} // namespace

/**
 * @test
 * @brief std::visit用visitor生成確認。
 * @details intとstd::stringを持つvariantへMakeOverloadで作ったvisitorを渡し、
 * 各alternativeに対応するhandlerが選ばれることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetFunctionTest, VisitsVariantWithGeneratedOverload)
{
	const auto visitor = ket::function::MakeOverload(
		[](int value)
		{
			return std::string("int:") + std::to_string(value);
		},
		[](const std::string& value)
		{
			return std::string("string:") + value;
		});
	const auto number = std::variant<int, std::string>(42);
	const auto text = std::variant<int, std::string>(std::string("ket"));

	const auto number_result = std::visit(visitor, number);
	const auto text_result = std::visit(visitor, text);

	EXPECT_EQ(number_result, std::string("int:42"));
	EXPECT_EQ(text_result, std::string("string:ket"));
}

/**
 * @test
 * @brief 複数型handlerのoverload解決確認。
 * @details 明示したcallable型からOverloadを作り、intとstd::stringの呼び出しで異なる
 * operatorが選ばれることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetFunctionTest, ResolvesMultipleCallableTypes)
{
	const auto visitor =
		ket::function::Overload<IntHandler, StringSizeHandler>{IntHandler{}, StringSizeHandler{}};
	const auto text = std::string("abcd");

	const auto int_result = visitor(41);
	const auto string_result = visitor(text);

	EXPECT_EQ(int_result, 42);
	EXPECT_EQ(string_result, 4);
}

/**
 * @test
 * @brief handler戻り値と例外伝播確認。
 * @details handlerの戻り値がそのまま返り、handlerが送出した例外が呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetFunctionTest, PreservesReturnValuesAndPropagatesExceptions)
{
	const auto value_visitor = ket::function::MakeOverload(IntHandler{});
	const auto throwing_visitor = ket::function::MakeOverload(ThrowingHandler{});

	const auto value = value_visitor(6);
	const auto throws_handler_error = [&throwing_visitor]()
	{
		static_cast<void>(throwing_visitor(1));
	};

	EXPECT_EQ(value, 7);
	EXPECT_THROW(throws_handler_error(), std::runtime_error);
}

/**
 * @test
 * @brief Noopの任意引数破棄確認。
 * @details 0個の引数、複数型の引数、move-only値を渡し、Noopが引数評価後に値を消費しないことを確認。
 * @pre C++17以降。
 * @post Noopは引数評価以外の外部状態を変更しない。
 */
TEST(KetFunctionTest, NoopAcceptsArbitraryArguments)
{
	const auto noop = ket::function::Noop{};
	auto ptr = std::make_unique<int>(42);
	auto value = 7;

	noop();
	noop(value, std::string("ignored"), ptr);
	noop(std::make_unique<int>(24));

	EXPECT_EQ(value, 7);
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(*ptr, 42);
}

/**
 * @test
 * @brief callableのcopy/move制約保持確認。
 * @details copy可能handlerはcopy可能なOverloadになり、move-only handlerはmoveのみ可能な
 * Overloadとして生成・移動できることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetFunctionTest, PreservesCallableCopyAndMoveConstraints)
{
	auto stateful = StatefulHandler(10);
	const auto copied_from_lvalue = ket::function::MakeOverload(stateful);
	const auto copyable = ket::function::MakeOverload(IntHandler{});
	const auto copied = copyable;
	auto move_only = ket::function::MakeOverload(MoveOnlyHandler{});
	auto moved = std::move(move_only);

	stateful.SetValue(100);
	const auto copied_from_lvalue_result = copied_from_lvalue(5);
	const auto copied_result = copied(1);
	const auto moved_result = moved(41);

	EXPECT_EQ(copied_from_lvalue_result, 15);
	EXPECT_EQ(copied_result, 2);
	EXPECT_EQ(moved_result, 42);
}
