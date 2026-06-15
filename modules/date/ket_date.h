#pragma once

/**
 * @file ket_date.h
 * @brief Gregorian日付と時刻の小さい妥当性判定API。
 *
 * @details calendar frameworkを持ち込まず、leap year、年月日、時分秒、
 * millisecond付き時刻の境界判定を標準ライブラリ不要の算術で扱う。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_date.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11〜17。
 * 推奨理由：C++17以前では標準calendar APIがなく、Gregorian妥当性判定を小さく持ち出せる。
 * 本ライブラリの適用を推奨しない C++ バージョン：C++20以降。
 * 非推奨理由：C++20 `std::chrono`
 * calendarを使える環境では、日付表現と妥当性判定を標準型へ寄せられる。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::date
 * 内部実装：ket::date::detail
 */

namespace ket
{
	namespace date
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief Gregorian calendarのleap year判定。
		 * @param[in] year 判定対象の年。
		 * @retval true `year >= 1`、かつ4で割り切れ、100で割り切れないか400で割り切れる年。
		 * @retval false `year < 1`、またはGregorian calendar上の平年。
		 * @pre なし。`year < 1`は無効な年としてfalse。
		 * @post 引数と外部状態の変更なし。
		 * @note timezoneとleap secondは対象外。
		 * @code
		 * const auto leap = ket::date::IsLeapYear(2000);
		 * // leap == true
		 * @endcode
		 */
		constexpr bool IsLeapYear(int year) noexcept;

		/**
		 * @brief Gregorian calendarの月番号妥当性判定。
		 * @param[in] month 判定対象の月番号。
		 * @retval true `1 <= month <= 12`。
		 * @retval false 0、または13以上。
		 * @pre なし。月番号は1始まりとして判定。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto valid = ket::date::IsValidMonth(12U);
		 * // valid == true
		 * @endcode
		 */
		constexpr bool IsValidMonth(unsigned month) noexcept;

		/**
		 * @brief Gregorian calendarの指定年月に含まれる日数取得。
		 * @param[in] year 日数を取得する年。
		 * @param[in] month 日数を取得する月番号。
		 * @param[out] out 成功時に当月日数を書き込む出力引数。
		 * @retval true `year >= 1`かつ`1 <= month <= 12`で、`out`へ日数を書き込んだ。
		 * @retval false 無効な年または月。`out`は変更なし。
		 * @pre `out`は書き込み可能なunsignedオブジェクトへの参照。
		 * @post 成功時のみ`out`を28、29、30、31のいずれかへ変更。失敗時は`out`を変更なし。
		 * @note C++11では出力引数を変更するTry APIをconstexpr化できないためnoexceptのみ。
		 * @code
		 * unsigned days = 0U;
		 * const auto ok = ket::date::TryGetDaysInMonth(2024, 2U, days);
		 * // ok == true, days == 29
		 * @endcode
		 */
		inline bool TryGetDaysInMonth(int year, unsigned month, unsigned& out) noexcept;

		/**
		 * @brief Gregorian calendarの年月日妥当性判定。
		 * @param[in] year 判定対象の年。
		 * @param[in] month 判定対象の月番号。
		 * @param[in] day 判定対象の日。
		 * @retval true `year >= 1`、有効な月、かつ当月日数内の日。
		 * @retval false 無効な年、月、0日、または月末を超える日。
		 * @pre なし。timezone、calendar conversion、date arithmeticは対象外。
		 * @post 引数と外部状態の変更なし。
		 * @note constexprを保つため、内部constexpr helperで当月日数を判定。
		 * @code
		 * const auto valid = ket::date::IsValidDate(2024, 2U, 29U);
		 * // valid == true
		 * @endcode
		 */
		constexpr bool IsValidDate(int year, unsigned month, unsigned day) noexcept;

		/**
		 * @brief 24時間表記の時分秒妥当性判定。
		 * @param[in] hour 判定対象の時。
		 * @param[in] minute 判定対象の分。
		 * @param[in] second 判定対象の秒。
		 * @retval true `hour < 24`、`minute < 60`、`second < 60`。
		 * @retval false 時、分、秒のいずれかが範囲外。
		 * @pre なし。leap secondは扱わず、`second == 60`は無効。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto valid = ket::date::IsValidTime(23U, 59U, 59U);
		 * // valid == true
		 * @endcode
		 */
		constexpr bool IsValidTime(unsigned hour, unsigned minute, unsigned second) noexcept;

		/**
		 * @brief millisecond付き24時間表記の時刻妥当性判定。
		 * @param[in] hour 判定対象の時。
		 * @param[in] minute 判定対象の分。
		 * @param[in] second 判定対象の秒。
		 * @param[in] millisecond 判定対象のmillisecond。
		 * @retval true 時分秒が有効で、`millisecond < 1000`。
		 * @retval false 時分秒、またはmillisecondが範囲外。
		 * @pre なし。leap secondは扱わず、`second == 60`は無効。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto valid = ket::date::IsValidTimeWithMilliseconds(23U, 59U, 59U, 999U);
		 * // valid == true
		 * @endcode
		 */
		constexpr bool IsValidTimeWithMilliseconds(unsigned hour,
												   unsigned minute,
												   unsigned second,
												   unsigned millisecond) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief Gregorian calendarで扱う年番号の妥当性判定。
			 * @param[in] year 判定対象の年。
			 * @retval true `year >= 1`。
			 * @retval false `year < 1`。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsPositiveYear(int year) noexcept
			{
				return year >= 1;
			}

			/**
			 * @brief Gregorian calendarのleap year判定本体。
			 * @param[in] year 判定対象の年。
			 * @retval true `year >= 1`かつGregorian calendar上のleap year。
			 * @retval false 無効な年、または平年。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsLeapYearValue(int year) noexcept
			{
				return IsPositiveYear(year) &&
					(((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0);
			}

			/**
			 * @brief 31日月の判定。
			 * @param[in] month 判定対象の月番号。
			 * @retval true 1、3、5、7、8、10、12月。
			 * @retval false 31日月ではない月番号。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsThirtyOneDayMonth(unsigned month) noexcept
			{
				return month == 1U || month == 3U || month == 5U || month == 7U || month == 8U ||
					month == 10U || month == 12U;
			}

			/**
			 * @brief 30日月の判定。
			 * @param[in] month 判定対象の月番号。
			 * @retval true 4、6、9、11月。
			 * @retval false 30日月ではない月番号。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsThirtyDayMonth(unsigned month) noexcept
			{
				return month == 4U || month == 6U || month == 9U || month == 11U;
			}

			/**
			 * @brief bool値の0/1変換。
			 * @param[in] value 変換対象のbool値。
			 * @retval 1 `value == true`。
			 * @retval 0 `value == false`。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr unsigned BoolToUnsigned(bool value) noexcept
			{
				return value ? 1U : 0U;
			}

			/**
			 * @brief 指定年月の日数取得。
			 * @param[in] year 日数を取得する年。
			 * @param[in] month 日数を取得する月番号。
			 * @retval value 28、29、30、31のいずれか。
			 * @retval 0 無効な年または月。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr unsigned DaysInMonthOrZero(int year, unsigned month) noexcept
			{
				return BoolToUnsigned(IsPositiveYear(year)) *
					((BoolToUnsigned(IsThirtyOneDayMonth(month)) * 31U) +
					 (BoolToUnsigned(IsThirtyDayMonth(month)) * 30U) +
					 (BoolToUnsigned(month == 2U) * (28U + BoolToUnsigned(IsLeapYearValue(year)))));
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr bool IsLeapYear(int year) noexcept
		{
			return detail::IsLeapYearValue(year);
		}

		constexpr bool IsValidMonth(unsigned month) noexcept
		{
			return month >= 1U && month <= 12U;
		}

		inline bool TryGetDaysInMonth(int year, unsigned month, unsigned& out) noexcept
		{
			const auto days = detail::DaysInMonthOrZero(year, month);
			const auto has_days = days != 0U;
			if (!has_days)
			{
				return false;
			}

			out = days;
			return true;
		}

		constexpr bool IsValidDate(int year, unsigned month, unsigned day) noexcept
		{
			return day >= 1U && day <= detail::DaysInMonthOrZero(year, month);
		}

		constexpr bool IsValidTime(unsigned hour, unsigned minute, unsigned second) noexcept
		{
			return hour < 24U && minute < 60U && second < 60U;
		}

		constexpr bool IsValidTimeWithMilliseconds(unsigned hour,
												   unsigned minute,
												   unsigned second,
												   unsigned millisecond) noexcept
		{
			return IsValidTime(hour, minute, second) && millisecond < 1000U;
		}

	} // namespace date

} // namespace ket
