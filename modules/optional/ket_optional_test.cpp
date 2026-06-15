#include "ket_optional.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace
{
	class MoveTracked
	{
	  public:
		explicit MoveTracked(bool& moved_from) noexcept : moved_from_(&moved_from) {}

		MoveTracked(const MoveTracked&) = delete;
		MoveTracked& operator=(const MoveTracked&) = delete;

		MoveTracked(MoveTracked&& other) noexcept : moved_from_(other.moved_from_)
		{
			payload_ = other.payload_;
			if (other.moved_from_ != nullptr)
			{
				*other.moved_from_ = true;
			}
			other.moved_from_ = nullptr;
		}

		MoveTracked& operator=(MoveTracked&& other) noexcept
		{
			if (this != &other)
			{
				moved_from_ = other.moved_from_;
				payload_ = other.payload_;
				if (other.moved_from_ != nullptr)
				{
					*other.moved_from_ = true;
				}
				other.moved_from_ = nullptr;
			}

			return *this;
		}

		[[nodiscard]] int Payload() const noexcept
		{
			return payload_;
		}

	  private:
		bool* moved_from_ = nullptr;
		int payload_ = 42;
	};

	struct ThrowingMapper
	{
		int operator()(int value) const
		{
			static_cast<void>(value);
			throw std::runtime_error("mapper failed");
		}
	};

	struct NonOptionalMapper
	{
		int operator()(int value) const noexcept
		{
			return value;
		}
	};

} // namespace

/**
 * @test
 * @brief 値ありoptionalのMap変換確認。
 * @details 値を持つoptionalをMapへ渡し、mapperが1回だけ呼ばれて変換結果を保持することを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は1。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, MapsEngagedOptional)
{
	int mapper_call_count = 0;
	const auto value = std::optional<int>(2);

	auto result = ket::optional::Map(value,
									 [&mapper_call_count](int item)
									 {
										 ++mapper_call_count;
										 return item * 3;
									 });

	static_assert(std::is_same_v<decltype(result), std::optional<int>>,
				  "Map keeps decayed mapper result in std::optional");
	EXPECT_EQ(result, std::optional<int>(6));
	EXPECT_EQ(mapper_call_count, 1);
}

/**
 * @test
 * @brief 空optionalのMap遅延確認。
 * @details 空optionalをMapへ渡し、mapperを呼ばずにstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は0。テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, MapDoesNotCallMapperForEmptyOptional)
{
	int mapper_call_count = 0;
	const auto value = std::optional<int>();

	const auto result = ket::optional::Map(value,
										   [&mapper_call_count](int item)
										   {
											   ++mapper_call_count;
											   return item * 3;
										   });
	const auto result_has_value = result.has_value();

	EXPECT_FALSE(result_has_value);
	EXPECT_EQ(mapper_call_count, 0);
}

/**
 * @test
 * @brief Mapの戻り型decay確認。
 * @details mapperがconst値を返す場合、Mapの戻り値がstd::optionalへdecay後の型を保持することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, MapStoresDecayedResultType)
{
	const auto value = std::optional<int>(2);

	auto result = ket::optional::Map(value,
									 [](int item)
									 {
										 const double mapped = static_cast<double>(item) + 0.5;
										 return mapped;
									 });

	static_assert(std::is_same_v<decltype(result), std::optional<double>>,
				  "Map stores std::decay_t mapper result");
	EXPECT_EQ(result, std::optional<double>(2.5));
}

/**
 * @test
 * @brief Mapのreference_wrapper保持確認。
 * @details
 * mapperがstd::reference_wrapperを明示的に返す場合、参照先を書き換えられる形で保持することを確認。
 * @pre C++17以降。
 * @post 参照先整数は更新される。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, MapStoresReferenceWrapperWhenMapperReturnsItExplicitly)
{
	int referenced = 10;
	const auto value = std::optional<int>(1);

	auto result = ket::optional::Map(value,
									 [&referenced](int item)
									 {
										 static_cast<void>(item);
										 return std::ref(referenced);
									 });

	static_assert(std::is_same_v<decltype(result), std::optional<std::reference_wrapper<int>>>,
				  "Map stores reference_wrapper by value");
	int fallback = -1;
	const auto mapped_reference = ket::optional::ValueOrEval(result,
															 [&fallback]
															 {
																 return std::ref(fallback);
															 });
	mapped_reference.get() = 20;

	EXPECT_EQ(referenced, 20);
	EXPECT_EQ(fallback, -1);
}

/**
 * @test
 * @brief rvalue optionalのMap move確認。
 * @details rvalue optionalをMapへ渡し、保持値がmapperへmoveされることを確認。
 * @pre C++17以降。
 * @post 入力optionalの保持値はmove元になり得る。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, MapMovesRvalueOptionalValueIntoMapper)
{
	bool moved_from = false;
	auto value = std::optional<MoveTracked>(std::in_place, moved_from);

	const auto result = ket::optional::Map(std::move(value),
										   [](MoveTracked item)
										   {
											   return item.Payload();
										   });

	EXPECT_EQ(result, std::optional<int>(42));
	EXPECT_TRUE(moved_from);
}

/**
 * @test
 * @brief 値ありoptionalのAndThen合成確認。
 * @details 値を持つoptionalをAndThenへ渡し、mapperが返したoptionalをそのまま返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は1。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, AndThenReturnsMapperOptionalForEngagedValue)
{
	int mapper_call_count = 0;
	const auto value = std::optional<int>(2);

	auto result =
		ket::optional::AndThen(value,
							   [&mapper_call_count](int item)
							   {
								   ++mapper_call_count;
								   return std::optional<std::string>(std::to_string(item * 3));
							   });

	static_assert(std::is_same_v<decltype(result), std::optional<std::string>>,
				  "AndThen returns mapper optional type");
	EXPECT_EQ(result, std::optional<std::string>("6"));
	EXPECT_EQ(mapper_call_count, 1);
}

/**
 * @test
 * @brief 空optionalのAndThen遅延確認。
 * @details 空optionalをAndThenへ渡し、mapperを呼ばずに空optionalを返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は0。テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, AndThenDoesNotCallMapperForEmptyOptional)
{
	int mapper_call_count = 0;
	const auto value = std::optional<int>();

	const auto result = ket::optional::AndThen(value,
											   [&mapper_call_count](int item)
											   {
												   ++mapper_call_count;
												   return std::optional<int>(item * 3);
											   });
	const auto result_has_value = result.has_value();

	EXPECT_FALSE(result_has_value);
	EXPECT_EQ(mapper_call_count, 0);
}

/**
 * @test
 * @brief rvalue optionalのAndThen move確認。
 * @details rvalue optionalをAndThenへ渡し、保持値がmapperへmoveされることを確認。
 * @pre C++17以降。
 * @post 入力optionalの保持値はmove元になり得る。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, AndThenMovesRvalueOptionalValueIntoMapper)
{
	bool moved_from = false;
	auto value = std::optional<MoveTracked>(std::in_place, moved_from);

	const auto result = ket::optional::AndThen(std::move(value),
											   [](MoveTracked item)
											   {
												   return std::optional<int>(item.Payload());
											   });

	EXPECT_EQ(result, std::optional<int>(42));
	EXPECT_TRUE(moved_from);
}

/**
 * @test
 * @brief ValueOrEvalの値ありcopy取得確認。
 * @details 値を持つconst optionalをValueOrEvalへ渡し、fallback
 * factoryを呼ばずに保持値をcopyすることを確認。
 * @pre C++17以降。
 * @post factory呼び出し回数は0。テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, ValueOrEvalCopiesEngagedConstOptionalWithoutCallingFactory)
{
	int factory_call_count = 0;
	const auto value = std::optional<std::string>("value");

	const auto result = ket::optional::ValueOrEval(value,
												   [&factory_call_count]
												   {
													   ++factory_call_count;
													   return std::string("fallback");
												   });

	EXPECT_EQ(result, std::string("value"));
	EXPECT_EQ(factory_call_count, 0);
}

/**
 * @test
 * @brief ValueOrEvalの空optional遅延fallback確認。
 * @details 空optionalをValueOrEvalへ渡し、fallback factoryが1回だけ呼ばれて値を返すことを確認。
 * @pre C++17以降。
 * @post factory呼び出し回数は1。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, ValueOrEvalCallsFactoryOnlyForEmptyOptional)
{
	int factory_call_count = 0;
	const auto value = std::optional<std::string>();

	const auto result = ket::optional::ValueOrEval(value,
												   [&factory_call_count]
												   {
													   ++factory_call_count;
													   return std::string("fallback");
												   });

	EXPECT_EQ(result, std::string("fallback"));
	EXPECT_EQ(factory_call_count, 1);
}

/**
 * @test
 * @brief rvalue optionalのValueOrEval move取得確認。
 * @details 値を持つrvalue optionalをValueOrEvalへ渡し、保持値が戻り値へmoveされることを確認。
 * @pre C++17以降。
 * @post 入力optionalの保持値はmove元になり得る。fallback factoryは呼ばれない。
 */
TEST(KetOptionalTest, ValueOrEvalMovesEngagedRvalueOptional)
{
	bool moved_from = false;
	bool fallback_moved_from = false;
	int factory_call_count = 0;
	auto value = std::optional<MoveTracked>(std::in_place, moved_from);

	const auto result = ket::optional::ValueOrEval(std::move(value),
												   [&fallback_moved_from, &factory_call_count]
												   {
													   ++factory_call_count;
													   return MoveTracked(fallback_moved_from);
												   });

	const auto payload = result.Payload();

	EXPECT_EQ(payload, 42);
	EXPECT_TRUE(moved_from);
	EXPECT_EQ(factory_call_count, 0);
}

/**
 * @test
 * @brief mapper例外の伝播確認。
 * @details MapとAndThenのmapperが例外を送出する場合、その例外が呼び出し元へ伝播することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, PropagatesMapperExceptions)
{
	const auto value = std::optional<int>(2);

	EXPECT_THROW(ket::optional::Map(value, ThrowingMapper{}), std::runtime_error);
	EXPECT_THROW(ket::optional::AndThen(value,
										[](int item) -> std::optional<int>
										{
											static_cast<void>(item);
											throw std::runtime_error("mapper failed");
										}),
				 std::runtime_error);
}

/**
 * @test
 * @brief 非optional mapperのcompile-fail相当確認。
 * @details
 * AndThenで使うoptional判定traitに対し、std::optionalは許容し、非optional戻り型は拒否されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, DetectsNonOptionalAndThenMapperResultForCompileFailure)
{
	using ValidResult = decltype(std::declval<NonOptionalMapper>()(0));

	static_assert(!ket::optional::detail::IsOptional<ValidResult>::value,
				  "AndThen rejects non-optional mapper results");
	static_assert(ket::optional::detail::IsOptional<std::optional<int>>::value,
				  "AndThen accepts std::optional mapper results");

	SUCCEED();
}

/**
 * @test
 * @brief Map mapper戻り型制約のcompile-fail相当確認。
 * @details Mapで使う戻り型判定traitに対し、値戻り型を許容し、voidと直接参照を拒否することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, DetectsInvalidMapMapperResultForCompileFailure)
{
	static_assert(ket::optional::detail::IsValidMapResult<int>::value,
				  "Map accepts object value result");
	static_assert(!ket::optional::detail::IsValidMapResult<void>::value, "Map rejects void result");
	static_assert(!ket::optional::detail::IsValidMapResult<int&>::value,
				  "Map rejects direct reference result");

	SUCCEED();
}
