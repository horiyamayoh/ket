#include "ket_variant.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

namespace
{
	class CopyCountingHandler
	{
	  public:
		CopyCountingHandler(int& calls, int& copies) noexcept;
		CopyCountingHandler(const CopyCountingHandler& other) noexcept;
		CopyCountingHandler(CopyCountingHandler&& other) = delete;
		CopyCountingHandler& operator=(const CopyCountingHandler& other) = delete;
		CopyCountingHandler& operator=(CopyCountingHandler&& other) = delete;

		int operator()(int number) const noexcept;

	  private:
		int* call_count_;
		int* copy_count_;
	};

	class MoveOnlyHandler
	{
	  public:
		explicit MoveOnlyHandler(int value) noexcept : offset_(value) {}

		MoveOnlyHandler(const MoveOnlyHandler& other) = delete;
		MoveOnlyHandler& operator=(const MoveOnlyHandler& other) = delete;
		MoveOnlyHandler(MoveOnlyHandler&& other) noexcept = default;
		MoveOnlyHandler& operator=(MoveOnlyHandler&& other) noexcept = delete;

		int operator()(int number) const noexcept
		{
			return number + offset_;
		}

	  private:
		int offset_;
	};

	struct LvalueOnlyHandler
	{
		int operator()(int number) const& noexcept
		{
			return number + 3;
		}
	};

	struct ThrowsOnMoveConstruction
	{
		ThrowsOnMoveConstruction() = default;
		ThrowsOnMoveConstruction(const ThrowsOnMoveConstruction& other) = delete;
		ThrowsOnMoveConstruction& operator=(const ThrowsOnMoveConstruction& other) = delete;

		// NOLINTBEGIN(bugprone-exception-escape, performance-noexcept-move-constructor)
		ThrowsOnMoveConstruction(ThrowsOnMoveConstruction&& other)
		{
			static_cast<void>(other);
			throw std::runtime_error("variant alternative move construction");
		}
		// NOLINTEND(bugprone-exception-escape, performance-noexcept-move-constructor)
	};

	struct IntHandler
	{
		int operator()(int number) const noexcept
		{
			return number;
		}
	};

	struct StringHandler
	{
		int operator()(const std::string& text) const
		{
			return static_cast<int>(text.size());
		}
	};

	template <typename Variant, typename... Handlers>
	struct HandlersCoverVariant
	{
	  private:
		using Visitor = decltype(ket::variant::detail::MakeOverload(std::declval<Handlers>()...));

		template <std::size_t... Indexes>
		static constexpr bool Check(std::index_sequence<Indexes...> indexes) noexcept
		{
			static_cast<void>(indexes);
			return (std::is_invocable_v<Visitor&, std::variant_alternative_t<Indexes, Variant>&> &&
					...);
		}

	  public:
		static constexpr bool value =
			Check(std::make_index_sequence<std::variant_size_v<Variant>>{});
	};

	static_assert(
		HandlersCoverVariant<std::variant<int, std::string>, IntHandler, StringHandler>::value,
		"handlers cover every variant alternative");
	static_assert(!HandlersCoverVariant<std::variant<int, std::string>, IntHandler>::value,
				  "missing string handler leaves an alternative uncovered");

	constexpr int ConstexprMatch()
	{
		std::variant<int, long> value = 7;
		return ket::variant::Match(
			value,
			[](int number)
			{
				return number + 1;
			},
			[](long number)
			{
				return static_cast<int>(number) + 2;
			});
	}

	static_assert(ConstexprMatch() == 8, "Match is constexpr in C++17");

	CopyCountingHandler::CopyCountingHandler(int& calls, int& copies) noexcept
		: call_count_(&calls), copy_count_(&copies)
	{
	}

	CopyCountingHandler::CopyCountingHandler(const CopyCountingHandler& other) noexcept
		: call_count_(other.call_count_), copy_count_(other.copy_count_)
	{
		++(*copy_count_);
	}

	int CopyCountingHandler::operator()(int number) const noexcept
	{
		++(*call_count_);
		return number + 1;
	}

} // namespace

/**
 * @test
 * @brief non-const lvalue variantの戻り値と参照保持確認。
 * @details int alternativeを参照で返し、Matchの戻り値が同じint参照であることを確認。
 * @pre C++17以降。
 * @post variant内のint値は参照経由の代入後の値へ変化。
 */
TEST(KetVariantTest, MatchesNonConstLvalueVariant)
{
	std::variant<int> value = 7;

	decltype(auto) selected = ket::variant::Match(value,
												  [](int& number) -> int&
												  {
													  ++number;
													  return number;
												  });
	static_assert(std::is_same_v<decltype(selected), int&>, "Match preserves reference return");

	selected = 9;
	const auto stored = std::get<int>(value);

	EXPECT_EQ(stored, 9);
}

/**
 * @test
 * @brief const lvalue variantのconst alternative保持確認。
 * @details const variantを渡し、const参照handlerが選択されて文字列長を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVariantTest, MatchesConstLvalueVariant)
{
	const std::variant<int, std::string> value = std::string("const");

	const auto result = ket::variant::Match(
		value,
		[](const int& number)
		{
			return number;
		},
		[](const std::string& text)
		{
			return static_cast<int>(text.size());
		});

	EXPECT_EQ(result, 5);
}

/**
 * @test
 * @brief rvalue variantのvalue category保持確認。
 * @details move-only alternativeを持つrvalue
 * variantを渡し、rvalue参照handlerが選択されることを確認。
 * @pre C++17以降。
 * @post variant内のunique_ptrはrvalue参照handlerで参照される。
 */
TEST(KetVariantTest, MatchesRvalueVariant)
{
	std::variant<std::unique_ptr<int>, int> value = std::make_unique<int>(42);
	bool pointer_was_observed = false;

	const auto result = ket::variant::Match(
		std::move(value),
		[&pointer_was_observed](std::unique_ptr<int>&& pointer)
		{
			pointer_was_observed = pointer != nullptr;
			return *pointer;
		},
		[](int&& number)
		{
			return number;
		});

	EXPECT_EQ(result, 42);
	EXPECT_TRUE(pointer_was_observed);
}

/**
 * @test
 * @brief lvalue限定handlerのdispatch確認。
 * @details operator()
 * &だけを持つhandlerをrvalueで渡し、内部visitorをlvalueとして呼び出すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVariantTest, InvokesVisitorAsLvalue)
{
	std::variant<int> value = 4;

	const auto result = ket::variant::Match(value, LvalueOnlyHandler{});

	EXPECT_EQ(result, 7);
}

/**
 * @test
 * @brief void戻り値handlerのdispatch確認。
 * @details int alternativeを持つvariantを渡し、void handlerの副作用だけが実行されることを確認。
 * @pre C++17以降。
 * @post observedは選択されたhandlerで設定された値へ変化。
 */
TEST(KetVariantTest, MatchesVoidHandlers)
{
	std::variant<int, std::string> value = 13;
	int observed = 0;

	ket::variant::Match(
		value,
		[&observed](int number)
		{
			observed = number;
		},
		[&observed](const std::string& text)
		{
			observed = static_cast<int>(text.size());
		});

	EXPECT_EQ(observed, 13);
}

/**
 * @test
 * @brief non-void戻り値handlerのdispatch確認。
 * @details 複数alternativeのhandlerが同じ戻り型を持つ場合、選択handlerの戻り値を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVariantTest, MatchesNonVoidHandlers)
{
	std::variant<int, std::string> value = std::string("abc");

	const auto result = ket::variant::Match(
		value,
		[](int number)
		{
			return std::string(static_cast<std::size_t>(number), 'x');
		},
		[](const std::string& text)
		{
			return text + "!";
		});

	EXPECT_EQ(result, std::string("abc!"));
}

/**
 * @test
 * @brief overload resolutionの確認。
 * @details generic handlerとint専用handlerを同時に渡し、int
 * alternativeで専用handlerが選択されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetVariantTest, PrefersSpecificHandlerOverGenericHandler)
{
	std::variant<int, double> value = 3;

	const auto result = ket::variant::Match(
		value,
		[](int number)
		{
			static_cast<void>(number);
			return std::string("int");
		},
		[](auto number)
		{
			static_cast<void>(number);
			return std::string("generic");
		});

	EXPECT_EQ(result, std::string("int"));
}

/**
 * @test
 * @brief lvalue handlerのcopy保持確認。
 * @details lvalue handlerを渡し、内部visitorがhandlerをcopyしてから選択handlerを呼ぶことを確認。
 * @pre C++17以降。
 * @post copy回数と呼び出し回数は観測用counterへ記録される。
 */
TEST(KetVariantTest, CopiesLvalueHandlerIntoVisitor)
{
	std::variant<int> value = 8;
	int call_count = 0;
	int copy_count = 0;
	const CopyCountingHandler handler(call_count, copy_count);

	const auto result = ket::variant::Match(value, handler);

	EXPECT_EQ(result, 9);
	EXPECT_EQ(call_count, 1);
	EXPECT_EQ(copy_count, 1);
}

/**
 * @test
 * @brief move-only rvalue handlerのmove保持確認。
 * @details unique_ptrをmove
 * captureしたhandlerをrvalueで渡し、内部visitorへmoveして呼び出せることを確認。
 * @pre C++17以降。
 * @post move-only handlerが保持する値は選択handler内で利用される。
 */
TEST(KetVariantTest, MovesMoveOnlyRvalueHandlerIntoVisitor)
{
	std::variant<int> value = 4;
	MoveOnlyHandler handler(6);

	const auto result = ket::variant::Match(value, std::move(handler));

	EXPECT_EQ(result, 10);
}

/**
 * @test
 * @brief handler例外の伝播確認。
 * @details 選択されたhandlerがstd::runtime_errorを送出し、Matchが同じ例外を伝播することを確認。
 * @pre C++17以降。
 * @post variantと外部状態の変更なし。
 */
TEST(KetVariantTest, PropagatesHandlerException)
{
	std::variant<int, std::string> value = 1;

	const auto match_operation = [&value]()
	{
		static_cast<void>(ket::variant::Match(
			value,
			[](int number) -> int
			{
				static_cast<void>(number);
				throw std::runtime_error("variant handler");
			},
			[](const std::string& text)
			{
				return static_cast<int>(text.size());
			}));
	};

	EXPECT_THROW(match_operation(), std::runtime_error);
}

/**
 * @test
 * @brief valueless_by_exception variantの例外伝播確認。
 * @details
 * valueless_by_exceptionになったvariantを渡し、std::visit由来のstd::bad_variant_accessを伝播することを確認。
 * @pre C++17以降。
 * @post variantはvalueless_by_exception状態を保持。
 */
TEST(KetVariantTest, PropagatesBadVariantAccessFromValuelessVariant)
{
	std::variant<int, ThrowsOnMoveConstruction> value = 1;
	try
	{
		value.emplace<ThrowsOnMoveConstruction>(ThrowsOnMoveConstruction{});
	}
	catch (const std::runtime_error& error)
	{
		static_cast<void>(error);
	}
	const auto value_is_valueless = value.valueless_by_exception();

	const auto match_operation = [&value]()
	{
		static_cast<void>(ket::variant::Match(
			value,
			[](int number)
			{
				return number;
			},
			[](const ThrowsOnMoveConstruction& alternative)
			{
				static_cast<void>(alternative);
				return 0;
			}));
	};

	EXPECT_TRUE(value_is_valueless);
	EXPECT_THROW(match_operation(), std::bad_variant_access);
}
