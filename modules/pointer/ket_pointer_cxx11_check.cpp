// clang-format off
#include "ket_pointer.h"

#include <memory>
#include <type_traits>
#include <utility>
// clang-format on

namespace
{
	class BaseTarget
	{
	  public:
		explicit BaseTarget(int value) noexcept : value_(value) {}

		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
	};

	class DerivedTarget : public BaseTarget
	{
	  public:
		explicit DerivedTarget(int value) noexcept : BaseTarget(value) {}
	};

	class OverloadedAddress
	{
	  public:
		explicit OverloadedAddress(int value) noexcept : value_(value) {}

		OverloadedAddress* operator&() noexcept
		{
			return nullptr;
		}

		const OverloadedAddress* operator&() const noexcept
		{
			return nullptr;
		}

		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
	};

	int kValue = 0;
	constexpr ket::pointer::NotNull<int> kConstexprPointer(&kValue);

	template <typename T>
	struct HasAddressOfExpression
	{
	  private:
		template <typename U>
		static auto Test(int) -> decltype(ket::pointer::AddressOf(std::declval<U>()),
										  std::true_type());

		template <typename>
		static std::false_type Test(...);

	  public:
		static const bool value = decltype(Test<T>(0))::value;
	};

	static_assert(kConstexprPointer.Get() == &kValue, "NotNull construction is constexpr");
	static_assert(std::is_same<decltype(kConstexprPointer.Get()), int*>::value,
				  "NotNull::Get returns the stored raw pointer type");
	static_assert(noexcept(kConstexprPointer.Get()), "NotNull::Get is noexcept");

	static_assert(
		std::is_same<decltype(*std::declval<const ket::pointer::NotNull<int>&>()), int&>::value,
		"NotNull::operator* returns a mutable reference for mutable pointees");
	static_assert(noexcept(*std::declval<const ket::pointer::NotNull<int>&>()),
				  "NotNull::operator* is noexcept");

	static_assert(
		std::is_same<decltype(std::declval<const ket::pointer::NotNull<int>&>().operator->()),
					 int*>::value,
		"NotNull::operator-> returns the stored raw pointer type");
	static_assert(noexcept(std::declval<const ket::pointer::NotNull<int>&>().operator->()),
				  "NotNull::operator-> is noexcept");

	static_assert(
		std::is_convertible<ket::pointer::NotNull<int>, ket::pointer::NotNull<const int>>::value,
		"NotNull converts mutable pointees to const pointees");
	static_assert(
		!std::is_convertible<ket::pointer::NotNull<const int>, ket::pointer::NotNull<int>>::value,
		"NotNull does not drop pointee const qualification");
	static_assert(std::is_convertible<ket::pointer::NotNull<DerivedTarget>,
									  ket::pointer::NotNull<BaseTarget>>::value,
				  "NotNull supports derived-to-base pointer conversion");
	static_assert(!std::is_convertible<ket::pointer::NotNull<BaseTarget>,
									   ket::pointer::NotNull<DerivedTarget>>::value,
				  "NotNull rejects base-to-derived pointer conversion");

	static_assert(
		std::is_same<decltype(ket::pointer::LockWeak(std::declval<const std::weak_ptr<int>&>())),
					 std::shared_ptr<int>>::value,
		"LockWeak returns shared_ptr<T>");
	static_assert(noexcept(ket::pointer::LockWeak(std::declval<const std::weak_ptr<int>&>())),
				  "LockWeak is noexcept");

	static_assert(
		std::is_same<decltype(ket::pointer::AddressOf(std::declval<int&>())), int*>::value,
		"AddressOf returns mutable pointer for mutable lvalue");
	static_assert(std::is_same<decltype(ket::pointer::AddressOf(std::declval<const int&>())),
							   const int*>::value,
				  "AddressOf returns const pointer for const lvalue");
	static_assert(HasAddressOfExpression<int&>::value, "AddressOf accepts mutable lvalues");
	static_assert(HasAddressOfExpression<const int&>::value, "AddressOf accepts const lvalues");
	static_assert(!HasAddressOfExpression<int&&>::value, "AddressOf rejects mutable rvalues");
	static_assert(!HasAddressOfExpression<const int&&>::value, "AddressOf rejects const rvalues");
	static_assert(noexcept(ket::pointer::AddressOf(std::declval<int&>())), "AddressOf is noexcept");

} // namespace

int KetPointerCxx11Check()
{
	int value = 1;
	const ket::pointer::NotNull<int> ptr(&value);
	*ptr = 2;

	const int* const raw = ptr.Get();
	const ket::pointer::NotNull<const int> const_ptr = ptr;

	DerivedTarget derived(3);
	const ket::pointer::NotNull<DerivedTarget> derived_ptr(&derived);
	const ket::pointer::NotNull<BaseTarget> base_ptr = derived_ptr;

	const auto owner = std::make_shared<int>(4);
	const std::weak_ptr<int> weak = owner;
	const auto locked = ket::pointer::LockWeak(weak);
	const std::weak_ptr<int> empty_weak;
	const auto empty_locked = ket::pointer::LockWeak(empty_weak);

	OverloadedAddress object(5);
	const OverloadedAddress* const actual_address = ket::pointer::AddressOf(object);
	const OverloadedAddress const_object(6);
	const OverloadedAddress* const const_actual_address = ket::pointer::AddressOf(const_object);

	static_cast<void>(empty_locked);

	return *raw + *const_ptr + base_ptr->Value() + *locked + actual_address->Value() +
		const_actual_address->Value();
}
