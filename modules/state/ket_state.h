#pragma once

/**
 * @file ket_state.h
 * @brief 小さい状態遷移表lookup API。
 *
 * @details 利用者が明示した状態遷移表を先頭から線形走査し、定義済み遷移だけを許可する。
 * FSM frameworkではなく、duplicate先勝ちと未定義遷移の失敗値だけを小さく固定する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_state.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::optional`で未定義遷移を明確にし、table-driven lookupを小さく書ける。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：標準ライブラリに状態遷移表lookupの直接APIなし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::state
 * 内部実装：ket::state::detail
 */

#include <cstddef>
#include <optional>

namespace ket
{
	namespace state
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 状態遷移表の1行。
		 * @tparam State 状態値の型。
		 * @tparam Event イベント値の型。
		 * @note `from`と`event`が一致した行の`to`を遷移先として扱う。
		 */
		template <typename State, typename Event>
		struct Transition
		{
			State from;
			Event event;
			State to;
		};

		/**
		 * @brief 現在状態とイベントに対応する遷移の有無判定。
		 * @tparam State 状態値の型。
		 * @tparam Event イベント値の型。
		 * @tparam N 遷移表の行数。
		 * @param[in] current 現在状態。
		 * @param[in] event 発生イベント。
		 * @param[in] table 検索対象の状態遷移表。
		 * @retval true `table`内に一致する遷移あり。
		 * @retval false `table`内に一致する遷移なし。
		 * @pre `State`と`Event`は等価比較可能で、その比較は例外を送出しない。`State`は
		 * std::optionalへの格納時に例外を送出しない。`table`は`N`行読み取り可能な配列。
		 * @post 引数と外部状態の変更なし。
		 * @note duplicate行がある場合は先頭の一致行を採用するため、判定結果はtrueで変わらない。
		 * @code
		 * enum class Mode { kIdle, kRunning };
		 * enum class Event { kStart };
		 * const ket::state::Transition<Mode, Event> table[] = {
		 *     {Mode::kIdle, Event::kStart, Mode::kRunning},
		 * };
		 * const auto allowed = ket::state::IsAllowed(Mode::kIdle, Event::kStart, table);
		 * // allowed == true
		 * @endcode
		 */
		template <typename State, typename Event, std::size_t N>
		bool
		IsAllowed(State current, Event event, const Transition<State, Event> (&table)[N]) noexcept;

		/**
		 * @brief 現在状態とイベントに対応する次状態の取得。
		 * @tparam State 状態値の型。
		 * @tparam Event イベント値の型。
		 * @tparam N 遷移表の行数。
		 * @param[in] current 現在状態。
		 * @param[in] event 発生イベント。
		 * @param[in] table 検索対象の状態遷移表。
		 * @retval value 先頭一致した遷移の次状態。
		 * @retval std::nullopt `table`内に一致する遷移なし。
		 * @pre `State`と`Event`は等価比較可能で、その比較は例外を送出しない。`State`は
		 * std::optionalへの格納時に例外を送出しない。`table`は`N`行読み取り可能な配列。
		 * @post 引数と外部状態の変更なし。
		 * @note duplicate行がある場合は先頭の一致行を採用。
		 * @code
		 * enum class Mode { kIdle, kRunning };
		 * enum class Event { kStart };
		 * const ket::state::Transition<Mode, Event> table[] = {
		 *     {Mode::kIdle, Event::kStart, Mode::kRunning},
		 * };
		 * const auto next = ket::state::Next(Mode::kIdle, Event::kStart, table);
		 * // next.has_value() == true, *next == Mode::kRunning
		 * @endcode
		 */
		template <typename State, typename Event, std::size_t N>
		std::optional<State>
		Next(State current, Event event, const Transition<State, Event> (&table)[N]) noexcept;

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename State, typename Event, std::size_t N>
		bool
		IsAllowed(State current, Event event, const Transition<State, Event> (&table)[N]) noexcept
		{
			const auto next = Next(current, event, table);
			return next.has_value();
		}

		template <typename State, typename Event, std::size_t N>
		std::optional<State>
		Next(State current, Event event, const Transition<State, Event> (&table)[N]) noexcept
		{
			for (std::size_t index = 0U; index < N; ++index)
			{
				const auto& transition = table[index];
				const auto from_matches = transition.from == current;
				const auto event_matches = transition.event == event;
				const auto transition_matches = from_matches && event_matches;
				if (transition_matches)
				{
					return transition.to;
				}
			}

			return std::nullopt;
		}

	} // namespace state

} // namespace ket
