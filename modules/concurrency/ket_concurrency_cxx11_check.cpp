#include <future>
#include <thread>
#include <type_traits>
#include <utility>

#include "ket_concurrency.h"

namespace
{
	// NOLINTBEGIN(modernize-type-traits)

	static_assert(!std::is_copy_constructible<ket::concurrency::JoiningThread>::value,
				  "JoiningThread is not copy constructible.");
	static_assert(!std::is_copy_assignable<ket::concurrency::JoiningThread>::value,
				  "JoiningThread is not copy assignable.");
	static_assert(std::is_move_constructible<ket::concurrency::JoiningThread>::value,
				  "JoiningThread is move constructible.");
	static_assert(std::is_move_assignable<ket::concurrency::JoiningThread>::value,
				  "JoiningThread is move assignable.");
	static_assert(std::is_nothrow_default_constructible<ket::concurrency::JoiningThread>::value,
				  "JoiningThread default construction is noexcept.");
	static_assert(std::is_nothrow_move_constructible<ket::concurrency::JoiningThread>::value,
				  "JoiningThread move construction is noexcept.");
	static_assert(std::is_nothrow_move_assignable<ket::concurrency::JoiningThread>::value,
				  "JoiningThread move assignment is noexcept.");

	class ConcurrencyExercise
	{
	  public:
		ConcurrencyExercise()
		{
			std::promise<int> promise;
			std::future<int> future = promise.get_future();
			const std::future<int>& const_future = future;
			const bool future_ready = ket::concurrency::IsReady(const_future);
			(void)future_ready;

			const std::shared_future<int> shared_future = future.share();
			const bool shared_future_ready = ket::concurrency::IsReady(shared_future);
			(void)shared_future_ready;

			const ket::concurrency::JoiningThread empty;
			const bool empty_is_joinable = empty.Joinable();
			(void)empty_is_joinable;

			std::thread raw_thread;
			ket::concurrency::JoiningThread moved_thread(std::move(raw_thread));
			ket::concurrency::JoiningThread target;
			target = std::move(moved_thread);

			ket::concurrency::JoiningThread joinable_source(std::thread(
				[]()
				{
				}));
			const ket::concurrency::JoiningThread moved_joinable(std::move(joinable_source));
			ket::concurrency::JoiningThread replaced(std::thread(
				[]()
				{
				}));
			ket::concurrency::JoiningThread replacement(std::thread(
				[]()
				{
				}));
			replaced = std::move(replacement);
		}
	};

	ConcurrencyExercise concurrency_exercise;

	// NOLINTEND(modernize-type-traits)

} // namespace
