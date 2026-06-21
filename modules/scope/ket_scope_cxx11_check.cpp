#include <type_traits>
#include <utility>

#include "ket_scope.h"

namespace ket_scope_cxx11_check
{
	struct Callback
	{
		explicit Callback(int& count) noexcept : count_(&count) {}

		void operator()() const noexcept
		{
			++(*count_);
		}

	  private:
		int* count_;
	};

	struct ThrowingCopyValue
	{
		explicit ThrowingCopyValue(int value) noexcept : value_(value) {}

		ThrowingCopyValue(const ThrowingCopyValue& other) noexcept(false)
			: value_(Normalize(other.value_))
		{
		}

		ThrowingCopyValue& operator=(const ThrowingCopyValue& other) noexcept
		{
			value_ = Normalize(other.value_);
			return *this;
		}

	  private:
		static int Normalize(int value) noexcept
		{
			return value;
		}

		int value_;
	};

	void UseExit()
	{
		int count = 0;
		auto guard = ket::scope::MakeExit(Callback(count));
		auto moved = std::move(guard);
		moved.Dismiss();
		const auto active = moved.Active();
		static_cast<void>(active);
	}

	void UseRestore()
	{
		int value = 1;
		auto restore = ket::scope::MakeRestore(value);
		value = 2;
		const auto changed_value = value;
		const auto active = restore.Active();
		static_cast<void>(changed_value);
		static_cast<void>(active);
		static_cast<void>(restore);
	}

	static_assert(noexcept(ket::scope::MakeExit(std::declval<Callback>())),
				  "MakeExit must be noexcept for nothrow movable callback.");
	static_assert(noexcept(ket::scope::MakeRestore(std::declval<int&>())),
				  "MakeRestore must be noexcept for nothrow copyable saved value.");
	static_assert(!noexcept(ket::scope::MakeRestore(std::declval<ThrowingCopyValue&>())),
				  "MakeRestore must not be noexcept for throwing copyable saved value.");
	static_assert(!std::is_copy_constructible<ket::scope::Exit<Callback>>::value,
				  "Exit must not be copy constructible.");
	static_assert(std::is_move_constructible<ket::scope::Exit<Callback>>::value,
				  "Exit must be move constructible.");
	static_assert(std::is_nothrow_move_constructible<ket::scope::Exit<Callback>>::value,
				  "Exit must be nothrow move constructible.");
	static_assert(!std::is_copy_assignable<ket::scope::Exit<Callback>>::value,
				  "Exit must not be copy assignable.");
	static_assert(!std::is_move_assignable<ket::scope::Exit<Callback>>::value,
				  "Exit must not be move assignable.");
	static_assert(!std::is_copy_constructible<ket::scope::Restore<int>>::value,
				  "Restore must not be copy constructible.");
	static_assert(std::is_move_constructible<ket::scope::Restore<int>>::value,
				  "Restore must be move constructible for C++11 MakeRestore.");
	static_assert(!std::is_copy_assignable<ket::scope::Restore<int>>::value,
				  "Restore must not be copy assignable.");
	static_assert(!std::is_move_assignable<ket::scope::Restore<int>>::value,
				  "Restore must not be move assignable.");

} // namespace ket_scope_cxx11_check
