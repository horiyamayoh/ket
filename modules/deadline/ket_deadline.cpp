#include "ket_deadline.h"

#include <chrono> // IWYU pragma: keep
// IWYU pragma: no_include <ratio>

namespace ket
{
	namespace deadline
	{
		Stopwatch::Stopwatch(std::chrono::steady_clock::time_point started_at) noexcept
			: started_at_(started_at)
		{
		}

		Stopwatch Stopwatch::StartNew() noexcept
		{
			return Stopwatch(std::chrono::steady_clock::now());
		}

		void Stopwatch::Restart() noexcept
		{
			started_at_ = std::chrono::steady_clock::now();
		}

		std::chrono::steady_clock::duration Stopwatch::Elapsed() const noexcept
		{
			const auto now = std::chrono::steady_clock::now();
			return now - started_at_;
		}

		std::chrono::milliseconds Stopwatch::ElapsedMilliseconds() const noexcept
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(Elapsed());
		}

		Deadline::Deadline(std::chrono::steady_clock::time_point time_point) noexcept
			: time_point_(time_point)
		{
		}

		Deadline Deadline::After(std::chrono::steady_clock::duration timeout) noexcept
		{
			const auto now = std::chrono::steady_clock::now();
			const auto timeout_is_non_positive =
				timeout <= std::chrono::steady_clock::duration::zero();
			if (timeout_is_non_positive)
			{
				return Deadline(now);
			}

			return Deadline(now + timeout);
		}

		Deadline Deadline::At(std::chrono::steady_clock::time_point time_point) noexcept
		{
			return Deadline(time_point);
		}

		bool Deadline::Expired() const noexcept
		{
			const auto now = std::chrono::steady_clock::now();
			return time_point_ <= now;
		}

		std::chrono::steady_clock::duration Deadline::Remaining() const noexcept
		{
			const auto now = std::chrono::steady_clock::now();
			const auto deadline_has_passed = time_point_ <= now;
			if (deadline_has_passed)
			{
				return std::chrono::steady_clock::duration::zero();
			}

			return time_point_ - now;
		}

		std::chrono::steady_clock::time_point Deadline::TimePoint() const noexcept
		{
			return time_point_;
		}

	} // namespace deadline

} // namespace ket
