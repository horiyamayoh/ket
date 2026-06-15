#include "ket_variant.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

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
 * @post variant内のunique_ptrはhandlerでmoveされる。
 */
TEST(KetVariantTest, MatchesRvalueVariant)
{
	std::variant<std::unique_ptr<int>, int> value = std::make_unique<int>(42);

	const auto result = ket::variant::Match(
		std::move(value),
		[](std::unique_ptr<int>&& pointer)
		{
			const auto moved = std::move(pointer);
			return *moved;
		},
		[](int&& number)
		{
			return number;
		});

	EXPECT_EQ(result, 42);
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
 * @brief handler例外の伝播確認。
 * @details 選択されたhandlerがstd::runtime_errorを送出し、Matchが同じ例外を伝播することを確認。
 * @pre C++17以降。
 * @post variantと外部状態の変更なし。
 */
TEST(KetVariantTest, PropagatesHandlerException)
{
	std::variant<int, std::string> value = 1;

	EXPECT_THROW(ket::variant::Match(
					 value,
					 [](int number) -> int
					 {
						 static_cast<void>(number);
						 throw std::runtime_error("variant handler");
					 },
					 [](const std::string& text)
					 {
						 return static_cast<int>(text.size());
					 }),
				 std::runtime_error);
}
