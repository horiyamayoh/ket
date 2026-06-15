#include "ket_meta.h"

#include <string> // NOLINT(misc-include-cleaner)
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	struct TypeWithNestedType
	{
		using type = int;
	};

	struct TypeWithoutNestedType
	{
	};

	template <typename T, typename = void>
	struct HasNestedType : std::false_type
	{
	};

	template <typename T>
	struct HasNestedType<T, ket::meta::VoidT<typename T::type>> : std::true_type
	{
	};

	template <typename Trait>
	constexpr bool TraitValue() noexcept
	{
		return Trait::value;
	}

	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<int>, int>>(),
				  "plain type is unchanged");
	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<const volatile int&>, int>>(),
				  "cv-qualified lvalue reference is normalized");
	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<int&&>, int>>(),
				  "rvalue reference is normalized");
	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<const int* const&>, const int*>>(),
				  "top-level cv is removed without changing pointee cv");

	static_assert(TraitValue<std::is_same<ket::meta::TypeIdentity<const int&>::type, const int&>>(),
				  "TypeIdentity preserves references and cv-qualification");

	static_assert(!ket::meta::AlwaysFalse<>::value, "AlwaysFalse is false for an empty pack");
	static_assert(!ket::meta::AlwaysFalse<int, double>::value, "AlwaysFalse is false for any pack");
	static_assert(TraitValue<std::is_base_of<std::false_type, ket::meta::AlwaysFalse<int>>>(),
				  "AlwaysFalse derives from std::false_type");

	static_assert(HasNestedType<TypeWithNestedType>::value,
				  "VoidT enables positive SFINAE detection");
	static_assert(!HasNestedType<TypeWithoutNestedType>::value,
				  "VoidT enables negative SFINAE detection");

} // namespace

/**
 * @test
 * @brief RemoveCvrefの型正規化確認。
 * @details cv修飾、lvalue reference、rvalue reference、top-level const pointerを入力し、
 * 仕様通りの型へ写像されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMetaTest, RemovesCvAndReferences)
{
	const auto plain_type_is_unchanged =
		TraitValue<std::is_same<ket::meta::RemoveCvref<int>, int>>();
	const auto cv_reference_is_normalized =
		TraitValue<std::is_same<ket::meta::RemoveCvref<const volatile int&>, int>>();
	const auto rvalue_reference_is_normalized =
		TraitValue<std::is_same<ket::meta::RemoveCvref<int&&>, int>>();
	const auto pointer_pointee_cv_is_preserved =
		TraitValue<std::is_same<ket::meta::RemoveCvref<const int* const&>, const int*>>();

	EXPECT_TRUE(plain_type_is_unchanged);
	EXPECT_TRUE(cv_reference_is_normalized);
	EXPECT_TRUE(rvalue_reference_is_normalized);
	EXPECT_TRUE(pointer_pointee_cv_is_preserved);
}

/**
 * @test
 * @brief TypeIdentityの型保持確認。
 * @details 参照とcv修飾を含む型を入力し、`type`が入力型そのものになることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMetaTest, PreservesTypeIdentity)
{
	const auto reference_type_is_preserved =
		TraitValue<std::is_same<ket::meta::TypeIdentity<const int&>::type, const int&>>();
	const auto pointer_type_is_preserved =
		TraitValue<std::is_same<ket::meta::TypeIdentity<const int*>::type, const int*>>();

	EXPECT_TRUE(reference_type_is_preserved);
	EXPECT_TRUE(pointer_type_is_preserved);
}

/**
 * @test
 * @brief AlwaysFalseのfalse_type互換確認。
 * @details 空packと複数型packを入力し、常にfalseでstd::false_type派生になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMetaTest, ProvidesAlwaysFalseTrait)
{
	const auto empty_pack_is_false = ket::meta::AlwaysFalse<>::value;
	const auto typed_pack_is_false = ket::meta::AlwaysFalse<int, double>::value;
	const auto derives_from_false_type =
		TraitValue<std::is_base_of<std::false_type, ket::meta::AlwaysFalse<int>>>();

	EXPECT_FALSE(empty_pack_is_false);
	EXPECT_FALSE(typed_pack_is_false);
	EXPECT_TRUE(derives_from_false_type);
}

/**
 * @test
 * @brief VoidTのSFINAE使用確認。
 * @details nested typeの有無を検査するtraitでVoidTを使い、検出成功と置換失敗の両方を確認。
 * C++17標準代替のstd::void_tと同じvoid型になることも確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetMetaTest, EnablesVoidTSfinae)
{
	const auto detects_nested_type = HasNestedType<TypeWithNestedType>::value;
	const auto rejects_missing_nested_type = HasNestedType<TypeWithoutNestedType>::value;
	const auto matches_std_void_t =
		TraitValue<std::is_same<ket::meta::VoidT<int>, std::void_t<int>>>();

	EXPECT_TRUE(detects_nested_type);
	EXPECT_FALSE(rejects_missing_nested_type);
	EXPECT_TRUE(matches_std_void_t);
}
