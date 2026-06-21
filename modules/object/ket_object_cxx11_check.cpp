#include <type_traits>
#include <utility>

#include "ket_object.h"

namespace
{
	// NOLINTBEGIN(modernize-type-traits)

	class CopyDisabled : public ket::object::NonCopyable
	{
	  public:
		CopyDisabled() = default;
	};

	class MoveDisabled : public ket::object::NonMovable
	{
	  public:
		MoveDisabled() = default;
	};

	class MoveOnlyValue : public ket::object::MoveOnly
	{
	  public:
		MoveOnlyValue() = default;
	};

	class MoveOnlyPayload : public ket::object::MoveOnly
	{
	  public:
		MoveOnlyPayload() = default;
	};

	class ThrowingResetValue
	{
	  public:
		ThrowingResetValue() noexcept(false)
		{
			MayThrow();
		}

		ThrowingResetValue(const ThrowingResetValue&) = delete;
		ThrowingResetValue& operator=(const ThrowingResetValue&) = delete;

		ThrowingResetValue(ThrowingResetValue&& other) noexcept(false) : value_(other.value_)
		{
			MayThrow();
		}

		ThrowingResetValue& operator=(ThrowingResetValue&& other) noexcept(false)
		{
			value_ = other.value_;
			MayThrow();
			return *this;
		}

	  private:
		static void MayThrow() noexcept(false) {}

		int value_ = 0;
	};

	static_assert(!std::is_copy_constructible<CopyDisabled>::value,
				  "NonCopyable derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable<CopyDisabled>::value,
				  "NonCopyable derived type is not copy assignable.");
	static_assert(std::is_move_constructible<CopyDisabled>::value,
				  "NonCopyable derived type remains move constructible.");
	static_assert(std::is_move_assignable<CopyDisabled>::value,
				  "NonCopyable derived type remains move assignable.");
	static_assert(!std::is_copy_constructible<MoveDisabled>::value,
				  "NonMovable derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable<MoveDisabled>::value,
				  "NonMovable derived type is not copy assignable.");
	static_assert(!std::is_move_constructible<MoveDisabled>::value,
				  "NonMovable derived type is not move constructible.");
	static_assert(!std::is_move_assignable<MoveDisabled>::value,
				  "NonMovable derived type is not move assignable.");
	static_assert(!std::is_copy_constructible<MoveOnlyValue>::value,
				  "MoveOnly derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable<MoveOnlyValue>::value,
				  "MoveOnly derived type is not copy assignable.");
	static_assert(std::is_move_constructible<MoveOnlyValue>::value,
				  "MoveOnly derived type is move constructible.");
	static_assert(std::is_move_assignable<MoveOnlyValue>::value,
				  "MoveOnly derived type is move assignable.");
	static_assert(!std::is_copy_constructible<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove is not copy constructible.");
	static_assert(!std::is_copy_assignable<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove is not copy assignable.");
	static_assert(std::is_move_constructible<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove is move constructible.");
	static_assert(std::is_move_assignable<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove is move assignable.");
	static_assert(std::is_nothrow_move_constructible<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove<int> move construction is noexcept.");
	static_assert(std::is_nothrow_move_assignable<ket::object::ResetOnMove<int>>::value,
				  "ResetOnMove<int> move assignment is noexcept.");
	static_assert(
		!std::is_nothrow_move_constructible<ket::object::ResetOnMove<ThrowingResetValue>>::value,
		"ResetOnMove<T> move construction is not noexcept when T can throw.");
	static_assert(
		!std::is_nothrow_move_assignable<ket::object::ResetOnMove<ThrowingResetValue>>::value,
		"ResetOnMove<T> move assignment is not noexcept when T can throw.");

	class ResetOnMoveExercise
	{
	  public:
		ResetOnMoveExercise()
		{
			ket::object::ResetOnMove<int> source(1);
			ket::object::ResetOnMove<int> moved(std::move(source));
			ket::object::ResetOnMove<int> assigned;

			assigned = std::move(moved);
			assigned.Get() = 2;

			const ket::object::ResetOnMove<int>& const_assigned = assigned;
			const int current = const_assigned.Get();
			(void)current;

			ket::object::ResetOnMove<MoveOnlyPayload> move_only_source{MoveOnlyPayload()};
			ket::object::ResetOnMove<MoveOnlyPayload> move_only_destination;
			move_only_destination = std::move(move_only_source);
			const MoveOnlyPayload& move_only_payload = move_only_destination.Get();
			(void)move_only_payload;
		}
	};

	ResetOnMoveExercise reset_on_move_exercise;

	// NOLINTEND(modernize-type-traits)

} // namespace
