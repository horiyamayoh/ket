#include <type_traits>

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

	static_assert(!std::is_copy_constructible<CopyDisabled>::value,
				  "NonCopyable derived type is not copy constructible.");
	static_assert(!std::is_copy_assignable<CopyDisabled>::value,
				  "NonCopyable derived type is not copy assignable.");
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

	// NOLINTEND(modernize-type-traits)

} // namespace
