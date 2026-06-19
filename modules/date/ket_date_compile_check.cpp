#include "ket_date.h"

static_assert(ket::date::IsLeapYear(2000), "2000 is a leap year");
static_assert(!ket::date::IsLeapYear(1900), "1900 is not a leap year");
static_assert(!ket::date::IsLeapYear(0), "year 0 is invalid");
static_assert(ket::date::IsValidMonth(12U), "month 12 is valid");
static_assert(!ket::date::IsValidMonth(13U), "month 13 is invalid");
static_assert(ket::date::IsValidDate(2024, 2U, 29U), "2024-02-29 is valid");
static_assert(!ket::date::IsValidDate(2023, 2U, 29U), "2023-02-29 is invalid");
static_assert(ket::date::IsValidTime(23U, 59U, 59U), "23:59:59 is valid");
static_assert(!ket::date::IsValidTime(23U, 59U, 60U), "leap second is invalid");
static_assert(ket::date::IsValidDateTime(2024, 2U, 29U, 23U, 59U, 59U),
			  "2024-02-29T23:59:59 is valid");
static_assert(!ket::date::IsValidDateTime(2023, 2U, 29U, 23U, 59U, 59U),
			  "invalid date makes date-time invalid");
static_assert(!ket::date::IsValidDateTime(2024, 2U, 29U, 23U, 59U, 60U),
			  "invalid time makes date-time invalid");
static_assert(ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 999U), "23:59:59.999 is valid");
static_assert(!ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 1000U),
			  "millisecond 1000 is invalid");

namespace ket_date_compile_check
{
	bool UsesTryGetDaysInMonth() noexcept
	{
		unsigned days = 0U;
		const auto succeeded = ket::date::TryGetDaysInMonth(2024, 2U, days);
		return succeeded && days == 29U;
	}

} // namespace ket_date_compile_check
