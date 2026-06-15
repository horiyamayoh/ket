#include <type_traits>

#include "ket_cache.h"

namespace
{
	class MoveOnlyValue
	{
	  public:
		explicit MoveOnlyValue(int value) noexcept : value_(value) {}

		MoveOnlyValue(const MoveOnlyValue&) = delete;
		MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;

		MoveOnlyValue(MoveOnlyValue&& other) noexcept : value_(other.value_)
		{
			other.value_ = 0;
		}

		MoveOnlyValue& operator=(MoveOnlyValue&&) = delete;

		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
	};

	struct IntFactory
	{
		int operator()() const noexcept
		{
			return 42;
		}
	};

	struct MoveOnlyFactory
	{
		MoveOnlyValue operator()() const noexcept
		{
			return MoveOnlyValue(7);
		}
	};

	static_assert(!std::is_copy_constructible<ket::cache::Lazy<int>>::value,
				  "ket::cache::Lazy is not copy constructible.");
	static_assert(!std::is_copy_assignable<ket::cache::Lazy<int>>::value,
				  "ket::cache::Lazy is not copy assignable.");
	static_assert(!std::is_move_constructible<ket::cache::Lazy<int>>::value,
				  "ket::cache::Lazy is not move constructible.");
	static_assert(!std::is_move_assignable<ket::cache::Lazy<int>>::value,
				  "ket::cache::Lazy is not move assignable.");

} // namespace

void KetCacheCxx11CompileCheck()
{
	ket::cache::Lazy<int> int_value;
	const int& created_int = int_value.GetOrCreate(IntFactory());
	static_cast<void>(created_int);

	ket::cache::Lazy<MoveOnlyValue> move_only_value;
	const MoveOnlyValue& created_move_only = move_only_value.GetOrCreate(MoveOnlyFactory());
	const auto stored_value = created_move_only.Value();
	static_cast<void>(stored_value);
}
