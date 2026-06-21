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

	struct OptionalMapper
	{
		std::optional<int> operator()(int value) const noexcept
		{
			return {value};
		}
	};

	struct VoidMapper
	{
		void operator()(int value) const noexcept
		{
			static_cast<void>(value);
		}
	};

	struct ConstReferenceMapper
	{
		const int& operator()(const int& value) const noexcept
		{
			return value;
		}
	};

	struct OptionalReferenceMapper
	{
		std::optional<int>& operator()(std::optional<int>& value) const noexcept
		{
			return value;
		}
	};

	struct InvokeTarget
	{
		constexpr explicit InvokeTarget(int input_value) noexcept : value_(input_value) {}

		[[nodiscard]] constexpr int Triple() const noexcept
		{
			return value_ * 3;
		}

		[[nodiscard]] constexpr std::optional<int> TripleOptional() const noexcept
		{
			return {value_ * 3};
		}

	  private:
		int value_ = 0;
	};

	class LvalueOnlyMapper
	{
	  public:
		explicit LvalueOnlyMapper(int& call_count) noexcept : call_count_(&call_count) {}

		LvalueOnlyMapper(const LvalueOnlyMapper&) = delete;
		LvalueOnlyMapper& operator=(const LvalueOnlyMapper&) = delete;

		int operator()(int& value) const&
		{
			++(*call_count_);
			value += 5;
			return value;
		}

	  private:
		int* call_count_;
	};

	class LvalueOnlyFactory
	{
	  public:
		explicit LvalueOnlyFactory(int& call_count) noexcept : call_count_(&call_count) {}

		LvalueOnlyFactory(const LvalueOnlyFactory&) = delete;
		LvalueOnlyFactory& operator=(const LvalueOnlyFactory&) = delete;

		std::string operator()() const&
		{
			++(*call_count_);
			return {"fallback"};
		}

	  private:
		int* call_count_;
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
 * @brief mutable lvalue optionalのMap確認。
 * @details copy不可のlvalue mapperをMapへ渡し、保持値がmutable参照で渡ることを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は1。入力optionalの保持値はmapperにより更新される。
 */
TEST(KetOptionalTest, MapPassesMutableLvalueValueWithoutCopyingMapper)
{
	int mapper_call_count = 0;
	auto value = std::optional<int>(2);
	const LvalueOnlyMapper mapper(mapper_call_count);

	const auto result = ket::optional::Map(value, mapper);

	EXPECT_EQ(result, std::optional<int>(7));
	EXPECT_EQ(value, std::optional<int>(7));
	EXPECT_EQ(mapper_call_count, 1);
}

/**
 * @test
 * @brief Mapのstd::invoke互換確認。
 * @details member function pointerをmapperとして渡し、std::invoke互換callableを扱えることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, MapAcceptsStdInvokeCompatibleMapper)
{
	const auto value = std::optional<InvokeTarget>(std::in_place, 3);

	const auto result = ket::optional::Map(value, &InvokeTarget::Triple);

	EXPECT_EQ(result, std::optional<int>(9));
}

/**
 * @test
 * @brief 空rvalue optionalのMap遅延確認。
 * @details 空のrvalue optionalをMapへ渡し、mapperを呼ばずに空optionalを返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は0。テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, MapDoesNotCallMapperForEmptyRvalueOptional)
{
	int mapper_call_count = 0;
	auto value = std::optional<MoveTracked>();

	const auto result = ket::optional::Map(std::move(value),
										   [&mapper_call_count](MoveTracked item)
										   {
											   ++mapper_call_count;
											   return item.Payload();
										   });
	const auto result_has_value = result.has_value();

	EXPECT_FALSE(result_has_value);
	EXPECT_EQ(mapper_call_count, 0);
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
 * @brief mutable lvalue optionalのAndThen確認。
 * @details 値を持つmutable
 * optionalをAndThenへ渡し、mapperが保持値を更新してoptionalを返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は1。入力optionalの保持値はmapperにより更新される。
 */
TEST(KetOptionalTest, AndThenPassesMutableLvalueValue)
{
	int mapper_call_count = 0;
	auto value = std::optional<int>(2);

	const auto result = ket::optional::AndThen(value,
											   [&mapper_call_count](int& item)
											   {
												   ++mapper_call_count;
												   item += 5;
												   return std::optional<int>(item * 2);
											   });

	EXPECT_EQ(result, std::optional<int>(14));
	EXPECT_EQ(value, std::optional<int>(7));
	EXPECT_EQ(mapper_call_count, 1);
}

/**
 * @test
 * @brief AndThenのstd::invoke互換確認。
 * @details member function pointerをmapperとして渡し、std::invoke互換callableを扱えることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, AndThenAcceptsStdInvokeCompatibleMapper)
{
	const auto value = std::optional<InvokeTarget>(std::in_place, 4);

	const auto result = ket::optional::AndThen(value, &InvokeTarget::TripleOptional);

	EXPECT_EQ(result, std::optional<int>(12));
}

/**
 * @test
 * @brief 空rvalue optionalのAndThen遅延確認。
 * @details 空のrvalue optionalをAndThenへ渡し、mapperを呼ばずに空optionalを返すことを確認。
 * @pre C++17以降。
 * @post mapper呼び出し回数は0。テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, AndThenDoesNotCallMapperForEmptyRvalueOptional)
{
	int mapper_call_count = 0;
	auto value = std::optional<MoveTracked>();

	const auto result = ket::optional::AndThen(std::move(value),
											   [&mapper_call_count](MoveTracked item)
											   {
												   ++mapper_call_count;
												   return std::optional<int>(item.Payload());
											   });
	const auto result_has_value = result.has_value();

	EXPECT_FALSE(result_has_value);
	EXPECT_EQ(mapper_call_count, 0);
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
 * @brief ValueOrEvalのcopy不可lvalue factory確認。
 * @details copy不可のlvalue
 * factoryをValueOrEvalへ渡し、std::invoke互換factoryをcopyせず扱うことを確認。
 * @pre C++17以降。
 * @post factory呼び出し回数は1。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, ValueOrEvalAcceptsLvalueOnlyFactory)
{
	int factory_call_count = 0;
	const LvalueOnlyFactory factory(factory_call_count);
	const auto value = std::optional<std::string>();

	const auto result = ket::optional::ValueOrEval(value, factory);

	EXPECT_EQ(result, std::string("fallback"));
	EXPECT_EQ(factory_call_count, 1);
}

/**
 * @test
 * @brief 空rvalue optionalのValueOrEval遅延fallback確認。
 * @details 空のrvalue
 * optionalをValueOrEvalへ渡し、factoryが1回だけ呼ばれてmove-only値を返すことを確認。
 * @pre C++17以降。
 * @post factory呼び出し回数は1。テスト対象API以外の外部状態の変更なし。
 */
TEST(KetOptionalTest, ValueOrEvalCallsFactoryForEmptyRvalueOptional)
{
	bool fallback_moved_from = false;
	int factory_call_count = 0;
	auto value = std::optional<MoveTracked>();

	const auto result = ket::optional::ValueOrEval(std::move(value),
												   [&fallback_moved_from, &factory_call_count]
												   {
													   ++factory_call_count;
													   return MoveTracked(fallback_moved_from);
												   });
	const auto payload = result.Payload();

	EXPECT_EQ(payload, 42);
	EXPECT_EQ(factory_call_count, 1);
}

/**
 * @test
 * @brief constexpr文脈の評価確認。
 * @details Map、AndThen、ValueOrEvalをconstexpr文脈で使い、単純な値変換を定数評価できることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetOptionalTest, SupportsConstexprEvaluation)
{
	constexpr auto mapped = ket::optional::Map(std::optional<int>(2),
											   [](int item)
											   {
												   return item * 3;
											   });
	static_assert(mapped.has_value(), "Map returns engaged optional in constexpr context");
	static_assert(*mapped == 6, "Map computes constexpr result");

	constexpr auto chained = ket::optional::AndThen(std::optional<int>(2),
													[](int item)
													{
														return std::optional<int>(item * 4);
													});
	static_assert(chained.has_value(), "AndThen returns engaged optional in constexpr context");
	static_assert(*chained == 8, "AndThen computes constexpr result");

	constexpr auto fallback = ket::optional::ValueOrEval(std::optional<int>(),
														 []
														 {
															 return 42;
														 });
	static_assert(fallback == 42, "ValueOrEval computes constexpr fallback");

	SUCCEED();
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
	static_assert(
		ket::optional::detail::IsValidAndThenInvocation<OptionalMapper&, const int&>::value,
		"AndThen accepts optional mapper results through its invocation constraint");
	static_assert(
		!ket::optional::detail::IsValidAndThenInvocation<NonOptionalMapper&, const int&>::value,
		"AndThen rejects non-optional mapper results through its invocation constraint");
	static_assert(!ket::optional::detail::IsValidAndThenInvocation<OptionalReferenceMapper&,
																   std::optional<int>&>::value,
				  "AndThen rejects optional reference results through its invocation constraint");

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
	static_assert(
		ket::optional::detail::IsValidMapInvocation<NonOptionalMapper&, const int&>::value,
		"Map accepts object value result through its invocation constraint");
	static_assert(!ket::optional::detail::IsValidMapInvocation<VoidMapper&, const int&>::value,
				  "Map rejects void result through its invocation constraint");
	static_assert(
		!ket::optional::detail::IsValidMapInvocation<ConstReferenceMapper&, const int&>::value,
		"Map rejects direct reference result through its invocation constraint");

	SUCCEED();
}
