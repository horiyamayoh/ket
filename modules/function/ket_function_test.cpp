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

	using CopyableOverload = ket::function::Overload<IntHandler>;
	using MoveOnlyOverload = ket::function::Overload<MoveOnlyHandler>;

	static_assert(std::is_copy_constructible_v<CopyableOverload>,
				  "copyable callable keeps copy construction");
	static_assert(std::is_move_constructible_v<CopyableOverload>,
				  "copyable callable keeps move construction");
	static_assert(!std::is_copy_constructible_v<MoveOnlyOverload>,
				  "move-only callable disables copy construction");
	static_assert(std::is_move_constructible_v<MoveOnlyOverload>,
				  "move-only callable keeps move construction");
	static_assert(noexcept(ket::function::Noop{}(1, "ignored")), "Noop call operator is noexcept");

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

	EXPECT_EQ(value, 7);
	EXPECT_THROW(throwing_visitor(1), std::runtime_error);
}

/**
 * @test
 * @brief Noopの任意引数破棄確認。
 * @details 複数型の引数とmove-only値を渡し、Noopが例外を送出せず呼び出せることを確認。
 * @pre C++17以降。
 * @post Noopは引数評価以外の外部状態を変更しない。
 */
TEST(KetFunctionTest, NoopAcceptsArbitraryArguments)
{
	const auto noop = ket::function::Noop{};
	auto ptr = std::make_unique<int>(42);

	EXPECT_NO_THROW(noop(1, std::string("ignored"), std::move(ptr)));
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
	const auto copyable = ket::function::MakeOverload(IntHandler{});
	const auto copied = copyable;
	auto move_only = ket::function::MakeOverload(MoveOnlyHandler{});
	auto moved = std::move(move_only);

	const auto copied_result = copied(1);
	const auto moved_result = moved(41);

	EXPECT_EQ(copied_result, 2);
	EXPECT_EQ(moved_result, 42);
}
