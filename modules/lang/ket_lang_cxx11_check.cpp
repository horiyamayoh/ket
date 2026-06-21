// clang-format off
#include "ket_lang.h"

#include <type_traits>
#include <utility>
// clang-format on

namespace
{
	template <typename T>
	struct ArraySizeAcceptsPointer
	{
		template <typename U>
		static auto Test(int) -> decltype(ket::lang::ArraySize(std::declval<U*>()),
										  std::true_type());

		template <typename>
		static std::false_type Test(...);

		static const bool value = decltype(Test<T>(0))::value;
	};

	template <typename T>
	struct AsConstAcceptsRvalue
	{
		template <typename U>
		static auto Test(int) -> decltype(ket::lang::AsConst(std::declval<U&&>()),
										  std::true_type());

		template <typename>
		static std::false_type Test(...);

		static const bool value = decltype(Test<T>(0))::value;
	};

	template <typename T>
	struct AsConstAcceptsConstRvalue
	{
		template <typename U>
		static auto Test(int) -> decltype(ket::lang::AsConst(std::declval<const U&&>()),
										  std::true_type());

		template <typename>
		static std::false_type Test(...);

		static const bool value = decltype(Test<T>(0))::value;
	};

	int ket_lang_cxx11_values[3] = {};

	static_assert(ket::lang::ArraySize(ket_lang_cxx11_values) == 3U,
				  "ArraySize is C++11 constexpr");
	static_assert(!ArraySizeAcceptsPointer<int>::value, "ArraySize rejects pointers");
	static_assert(!AsConstAcceptsRvalue<int>::value, "AsConst rejects rvalues");
	static_assert(!AsConstAcceptsConstRvalue<int>::value, "AsConst rejects const rvalues");

} // namespace

void KetLangCxx11CompileCheck()
{
	int value = 1;
	typedef decltype(ket::lang::AsConst(value)) ConstValueReference;
	static_assert(std::is_same<ConstValueReference, const int&>::value,
				  "AsConst returns const reference");
	const int& const_value = ket::lang::AsConst(value);
	ket::lang::IgnoreUnused(const_value);
}
