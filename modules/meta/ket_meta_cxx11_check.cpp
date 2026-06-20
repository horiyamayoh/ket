// clang-format off
#include "ket_meta.h"

#include <type_traits>
// clang-format on

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
	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<const int (&)[3]>, int[3]>>(),
				  "array references are not decayed to pointers");
	static_assert(TraitValue<std::is_same<ket::meta::RemoveCvref<int (&)(double)>, int(double)>>(),
				  "function references are not decayed to function pointers");

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
