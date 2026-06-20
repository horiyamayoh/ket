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

	class MoveOnlyIntFactory
	{
	  public:
		explicit MoveOnlyIntFactory(int* call_count) noexcept : call_count_(call_count) {}

		MoveOnlyIntFactory(const MoveOnlyIntFactory&) = delete;
		MoveOnlyIntFactory& operator=(const MoveOnlyIntFactory&) = delete;

		MoveOnlyIntFactory(MoveOnlyIntFactory&&) = delete;
		MoveOnlyIntFactory& operator=(MoveOnlyIntFactory&&) = delete;

		int operator()() const noexcept
		{
			++(*call_count_);
			return 13;
		}

	  private:
		int* call_count_;
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
	const bool initial_has_value = int_value.HasValue();
	static_cast<void>(initial_has_value);

	const int& created_int = int_value.GetOrCreate(IntFactory());
	static_cast<void>(created_int);

	const bool created_has_value = int_value.HasValue();
	static_cast<void>(created_has_value);

	int factory_count = 0;
	const MoveOnlyIntFactory int_factory(&factory_count);
	const int& created_from_lvalue_factory = int_value.GetOrCreate(int_factory);
	static_cast<void>(created_from_lvalue_factory);

	int_value.Reset();
	const bool reset_has_value = int_value.HasValue();
	static_cast<void>(reset_has_value);

	ket::cache::Lazy<MoveOnlyValue> move_only_value;
	const MoveOnlyValue& created_move_only = move_only_value.GetOrCreate(MoveOnlyFactory());
	const auto stored_value = created_move_only.Value();
	static_cast<void>(stored_value);
}
