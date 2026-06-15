#include <cstddef>
#include <type_traits>

#include "ket_contract.h"

namespace contract_cxx11_check
{
	int* RequirePointer(int* ptr) noexcept
	{
		return KET_REQUIRE_NON_NULL(ptr);
	}

	void CheckContracts(bool value) noexcept
	{
		KET_EXPECTS(value);
		KET_ENSURES(value);
		KET_ASSERT_INVARIANT(value);
	}

	static_assert(noexcept(ket::contract::Fail(ket::contract::Kind::kExpects, nullptr, nullptr, 0)),
				  "Fail is noexcept");
	static_assert(noexcept(ket::contract::Expects(true, nullptr, nullptr, 0)),
				  "Expects is noexcept");
	static_assert(noexcept(ket::contract::Ensures(true, nullptr, nullptr, 0)),
				  "Ensures is noexcept");
	static_assert(noexcept(ket::contract::AssertInvariant(true, nullptr, nullptr, 0)),
				  "AssertInvariant is noexcept");
	static_assert(
		noexcept(ket::contract::RequireNonNull(static_cast<int*>(nullptr), nullptr, nullptr, 0)),
		"RequireNonNull is noexcept");
	static_assert(noexcept(ket::contract::IsInBounds(std::size_t{0U}, std::size_t{0U})),
				  "IsInBounds is noexcept");
	static_assert(
		std::is_same<decltype(KET_REQUIRE_NON_NULL(static_cast<int*>(nullptr))), int*>::value,
		"RequireNonNull preserves pointer type");

} // namespace contract_cxx11_check
