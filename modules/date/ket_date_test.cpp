#include "ket_date.h"

#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

namespace
{
	static_assert(ket::date::IsLeapYear(2000), "2000 is a constexpr leap year");
	static_assert(!ket::date::IsLeapYear(1900), "1900 is a constexpr common year");
	static_assert(!ket::date::IsLeapYear(0), "year 0 is constexpr invalid");
	static_assert(ket::date::IsValidMonth(1U), "month 1 is constexpr valid");
	static_assert(ket::date::IsValidMonth(12U), "month 12 is constexpr valid");
	static_assert(!ket::date::IsValidMonth(0U), "month 0 is constexpr invalid");
	static_assert(!ket::date::IsValidMonth(13U), "month 13 is constexpr invalid");
	static_assert(ket::date::IsValidDate(2024, 2U, 29U), "leap day is constexpr valid");
	static_assert(!ket::date::IsValidDate(2023, 2U, 29U), "common-year leap day is invalid");
	static_assert(ket::date::IsValidTime(23U, 59U, 59U), "last second is constexpr valid");
	static_assert(!ket::date::IsValidTime(24U, 0U, 0U), "hour 24 is constexpr invalid");
	static_assert(ket::date::IsValidDateTime(2024, 2U, 29U, 23U, 59U, 59U),
				  "leap-day last second is constexpr valid");
	static_assert(!ket::date::IsValidDateTime(2023, 2U, 29U, 23U, 59U, 59U),
				  "invalid date makes constexpr date-time invalid");
	static_assert(!ket::date::IsValidDateTime(2024, 2U, 29U, 23U, 59U, 60U),
				  "invalid time makes constexpr date-time invalid");
	static_assert(ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 999U),
				  "last millisecond is constexpr valid");
	static_assert(!ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 1000U),
				  "millisecond 1000 is constexpr invalid");

} // namespace

/**
 * @test
 * @brief Gregorian leap year境界の確認。
 * @details 400年、100年、4年の各規則と無効年を入力し、leap year判定を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, DetectsGregorianLeapYears)
{
	const auto leap_2000 = ket::date::IsLeapYear(2000);
	const auto common_1900 = ket::date::IsLeapYear(1900);
	const auto leap_2004 = ket::date::IsLeapYear(2004);
	const auto common_2023 = ket::date::IsLeapYear(2023);
	const auto invalid_zero = ket::date::IsLeapYear(0);
	const auto invalid_negative = ket::date::IsLeapYear(-4);

	EXPECT_TRUE(leap_2000);
	EXPECT_FALSE(common_1900);
	EXPECT_TRUE(leap_2004);
	EXPECT_FALSE(common_2023);
	EXPECT_FALSE(invalid_zero);
	EXPECT_FALSE(invalid_negative);
}

/**
 * @test
 * @brief 月番号境界の確認。
 * @details 0、1、12、13を入力し、Gregorian calendarの1始まり月番号だけを有効と判定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, ValidatesMonthBoundaries)
{
	const auto month_zero = ket::date::IsValidMonth(0U);
	const auto january = ket::date::IsValidMonth(1U);
	const auto december = ket::date::IsValidMonth(12U);
	const auto month_thirteen = ket::date::IsValidMonth(13U);

	EXPECT_FALSE(month_zero);
	EXPECT_TRUE(january);
	EXPECT_TRUE(december);
	EXPECT_FALSE(month_thirteen);
}

/**
 * @test
 * @brief 指定年月の日数取得確認。
 * @details leap yearと平年の2月、30日月、31日月を入力し、当月日数が出力されることを確認。
 * @pre C++17以降。
 * @post 出力引数以外の外部状態の変更なし。
 */
TEST(KetDateTest, GetsDaysInMonth)
{
	unsigned leap_february_days = 0U;
	unsigned common_february_days = 0U;
	unsigned april_days = 0U;
	unsigned january_days = 0U;

	const auto leap_february = ket::date::TryGetDaysInMonth(2024, 2U, leap_february_days);
	const auto common_february = ket::date::TryGetDaysInMonth(2023, 2U, common_february_days);
	const auto april = ket::date::TryGetDaysInMonth(2023, 4U, april_days);
	const auto january = ket::date::TryGetDaysInMonth(2023, 1U, january_days);

	EXPECT_TRUE(leap_february);
	EXPECT_EQ(leap_february_days, 29U);
	EXPECT_TRUE(common_february);
	EXPECT_EQ(common_february_days, 28U);
	EXPECT_TRUE(april);
	EXPECT_EQ(april_days, 30U);
	EXPECT_TRUE(january);
	EXPECT_EQ(january_days, 31U);
}

/**
 * @test
 * @brief 指定年月の日数取得失敗時の出力保持確認。
 * @details 無効年、month 0、month 13を入力し、falseを返して出力引数を変更しないことを確認。
 * @pre C++17以降。
 * @post 出力引数は入力時の番兵値を保持。
 */
TEST(KetDateTest, RejectsInvalidYearOrMonthWithoutChangingOutput)
{
	unsigned year_zero_days = 123U;
	unsigned negative_year_days = 123U;
	unsigned month_zero_days = 123U;
	unsigned month_thirteen_days = 123U;

	const auto year_zero = ket::date::TryGetDaysInMonth(0, 1U, year_zero_days);
	const auto negative_year = ket::date::TryGetDaysInMonth(-1, 1U, negative_year_days);
	const auto month_zero = ket::date::TryGetDaysInMonth(2024, 0U, month_zero_days);
	const auto month_thirteen = ket::date::TryGetDaysInMonth(2024, 13U, month_thirteen_days);

	EXPECT_FALSE(year_zero);
	EXPECT_EQ(year_zero_days, 123U);
	EXPECT_FALSE(negative_year);
	EXPECT_EQ(negative_year_days, 123U);
	EXPECT_FALSE(month_zero);
	EXPECT_EQ(month_zero_days, 123U);
	EXPECT_FALSE(month_thirteen);
	EXPECT_EQ(month_thirteen_days, 123U);
}

/**
 * @test
 * @brief Gregorian年月日境界の確認。
 * @details leap day、平年の2月29日、day 0、月末超過、無効年と無効月を入力して妥当性を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, ValidatesDates)
{
	const auto leap_day = ket::date::IsValidDate(2024, 2U, 29U);
	const auto common_year_leap_day = ket::date::IsValidDate(2023, 2U, 29U);
	const auto april_thirtieth = ket::date::IsValidDate(2023, 4U, 30U);
	const auto april_thirty_first = ket::date::IsValidDate(2023, 4U, 31U);
	const auto day_zero = ket::date::IsValidDate(2023, 1U, 0U);
	const auto month_zero = ket::date::IsValidDate(2023, 0U, 1U);
	const auto month_thirteen = ket::date::IsValidDate(2023, 13U, 1U);
	const auto year_zero = ket::date::IsValidDate(0, 1U, 1U);

	EXPECT_TRUE(leap_day);
	EXPECT_FALSE(common_year_leap_day);
	EXPECT_TRUE(april_thirtieth);
	EXPECT_FALSE(april_thirty_first);
	EXPECT_FALSE(day_zero);
	EXPECT_FALSE(month_zero);
	EXPECT_FALSE(month_thirteen);
	EXPECT_FALSE(year_zero);
}

/**
 * @test
 * @brief 24時間表記の時分秒境界確認。
 * @details 先頭時刻、最終秒、hour 24、minute 60、second 60を入力し、leap secondなしの範囲を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, ValidatesTimes)
{
	const auto midnight = ket::date::IsValidTime(0U, 0U, 0U);
	const auto last_second = ket::date::IsValidTime(23U, 59U, 59U);
	const auto hour_twenty_four = ket::date::IsValidTime(24U, 0U, 0U);
	const auto minute_sixty = ket::date::IsValidTime(23U, 60U, 0U);
	const auto second_sixty = ket::date::IsValidTime(23U, 59U, 60U);

	EXPECT_TRUE(midnight);
	EXPECT_TRUE(last_second);
	EXPECT_FALSE(hour_twenty_four);
	EXPECT_FALSE(minute_sixty);
	EXPECT_FALSE(second_sixty);
}

/**
 * @test
 * @brief Gregorian年月日と24時間表記の時分秒を合わせた境界確認。
 * @details 有効なdate-time、無効日付、有効日付と無効時刻を入力し、合成判定の境界を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, ValidatesDateTimes)
{
	const auto leap_day_last_second = ket::date::IsValidDateTime(2024, 2U, 29U, 23U, 59U, 59U);
	const auto common_year_leap_day = ket::date::IsValidDateTime(2023, 2U, 29U, 23U, 59U, 59U);
	const auto invalid_time = ket::date::IsValidDateTime(2024, 2U, 29U, 24U, 0U, 0U);
	const auto invalid_date_and_time = ket::date::IsValidDateTime(2023, 2U, 29U, 23U, 59U, 60U);

	EXPECT_TRUE(leap_day_last_second);
	EXPECT_FALSE(common_year_leap_day);
	EXPECT_FALSE(invalid_time);
	EXPECT_FALSE(invalid_date_and_time);
}

/**
 * @test
 * @brief millisecond付き24時間表記の境界確認。
 * @details 999 millisecond、1000 millisecond、不正な時分秒を入力し、millisecond範囲を固定。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDateTest, ValidatesTimesWithMilliseconds)
{
	const auto first_millisecond = ket::date::IsValidTimeWithMilliseconds(0U, 0U, 0U, 0U);
	const auto last_millisecond = ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 999U);
	const auto millisecond_one_thousand =
		ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 1000U);
	const auto invalid_time = ket::date::IsValidTimeWithMilliseconds(24U, 0U, 0U, 0U);

	EXPECT_TRUE(first_millisecond);
	EXPECT_TRUE(last_millisecond);
	EXPECT_FALSE(millisecond_one_thousand);
	EXPECT_FALSE(invalid_time);
}
