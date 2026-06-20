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
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::state
 * 内部実装：ket::state::detail
 */

#include <array>
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
		 * @pre `State`と`Event`は等価比較可能。`table`は`N`行読み取り可能な配列。
		 * @post 引数と外部状態の変更なし。比較演算が例外を送出した場合は呼び出し元へ伝播。
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
		constexpr bool IsAllowed(const State& current,
								 const Event& event,
								 const Transition<State, Event> (&table)[N]);

		/**
		 * @brief 現在状態とイベントに対応する遷移の有無判定。
		 * @tparam State 状態値の型。
		 * @tparam Event イベント値の型。
		 * @tparam N 遷移表の行数。
		 * @param[in] current 現在状態。
		 * @param[in] event 発生イベント。
		 * @param[in] table 検索対象の状態遷移表。`N == 0`の場合は空表。
		 * @retval true `table`内に一致する遷移あり。
		 * @retval false `table`内に一致する遷移なし。空表では常にfalse。
		 * @pre `State`と`Event`は等価比較可能。
		 * @post 引数と外部状態の変更なし。比較演算が例外を送出した場合は呼び出し元へ伝播。
		 * @note duplicate行がある場合は先頭の一致行を採用するため、判定結果はtrueで変わらない。
		 * @code
		 * enum class Mode { kIdle, kRunning };
		 * enum class Event { kStart };
		 * const std::array<ket::state::Transition<Mode, Event>, 1> table = {{
		 *     {Mode::kIdle, Event::kStart, Mode::kRunning},
		 * }};
		 * const auto allowed = ket::state::IsAllowed(Mode::kIdle, Event::kStart, table);
		 * // allowed == true
		 * @endcode
		 */
		template <typename State, typename Event, std::size_t N>
		constexpr bool IsAllowed(const State& current,
								 const Event& event,
								 const std::array<Transition<State, Event>, N>& table);

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
		 * @pre `State`と`Event`は等価比較可能。`State`はstd::optionalへ格納可能。
		 * `table`は`N`行読み取り可能な配列。
		 * @post 引数と外部状態の変更なし。比較演算またはstd::optionalへの格納が例外を送出した場合は
		 * 呼び出し元へ伝播。
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
		constexpr std::optional<State>
		Next(const State& current, const Event& event, const Transition<State, Event> (&table)[N]);

		/**
		 * @brief 現在状態とイベントに対応する次状態の取得。
		 * @tparam State 状態値の型。
		 * @tparam Event イベント値の型。
		 * @tparam N 遷移表の行数。
		 * @param[in] current 現在状態。
		 * @param[in] event 発生イベント。
		 * @param[in] table 検索対象の状態遷移表。`N == 0`の場合は空表。
		 * @retval value 先頭一致した遷移の次状態。
		 * @retval std::nullopt `table`内に一致する遷移なし。空表では常にstd::nullopt。
		 * @pre `State`と`Event`は等価比較可能。`State`はstd::optionalへ格納可能。
		 * @post 引数と外部状態の変更なし。比較演算またはstd::optionalへの格納が例外を送出した場合は
		 * 呼び出し元へ伝播。
		 * @note duplicate行がある場合は先頭の一致行を採用。
		 * @code
		 * enum class Mode { kIdle, kRunning };
		 * enum class Event { kStart };
		 * const std::array<ket::state::Transition<Mode, Event>, 1> table = {{
		 *     {Mode::kIdle, Event::kStart, Mode::kRunning},
		 * }};
		 * const auto next = ket::state::Next(Mode::kIdle, Event::kStart, table);
		 * // next.has_value() == true, *next == Mode::kRunning
		 * @endcode
		 */
		template <typename State, typename Event, std::size_t N>
		constexpr std::optional<State> Next(const State& current,
											const Event& event,
											const std::array<Transition<State, Event>, N>& table);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 現在状態とイベントに一致する先頭遷移の検索。
			 * @tparam State 状態値の型。
			 * @tparam Event イベント値の型。
			 * @param[in] current 現在状態。
			 * @param[in] event 発生イベント。
			 * @param[in] table 検索対象の状態遷移表。`count == 0`の場合のみ`nullptr`可。
			 * @param[in] count 遷移表の行数。
			 * @retval value 先頭一致した遷移へのpointer。
			 * @retval nullptr 一致する遷移なし。
			 * @pre `State`と`Event`は等価比較可能。`table`は`count`行読み取り可能。
			 * @post 引数と外部状態の変更なし。比較演算が例外を送出した場合は呼び出し元へ伝播。
			 */
			template <typename State, typename Event>
			constexpr const Transition<State, Event>*
			FindTransition(const State& current,
						   const Event& event,
						   const Transition<State, Event>* table,
						   std::size_t count)
			{
				for (std::size_t index = 0U; index < count; ++index)
				{
					const auto& transition = table[index];
					const auto from_matches = transition.from == current;
					if (from_matches)
					{
						const auto event_matches = transition.event == event;
						if (event_matches)
						{
							return &transition;
						}
					}
				}

				return nullptr;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename State, typename Event, std::size_t N>
		constexpr bool IsAllowed(const State& current,
								 const Event& event,
								 const Transition<State, Event> (&table)[N])
		{
			const auto* const transition = detail::FindTransition(current, event, table, N);
			return transition != nullptr;
		}

		template <typename State, typename Event, std::size_t N>
		constexpr bool IsAllowed(const State& current,
								 const Event& event,
								 const std::array<Transition<State, Event>, N>& table)
		{
			const auto* const transition = detail::FindTransition(current, event, table.data(), N);
			return transition != nullptr;
		}

		template <typename State, typename Event, std::size_t N>
		constexpr std::optional<State>
		Next(const State& current, const Event& event, const Transition<State, Event> (&table)[N])
		{
			const auto* const transition = detail::FindTransition(current, event, table, N);
			if (transition == nullptr)
			{
				return std::nullopt;
			}

			return transition->to;
		}

		template <typename State, typename Event, std::size_t N>
		constexpr std::optional<State> Next(const State& current,
											const Event& event,
											const std::array<Transition<State, Event>, N>& table)
		{
			const auto* const transition = detail::FindTransition(current, event, table.data(), N);
			if (transition == nullptr)
			{
				return std::nullopt;
			}

			return transition->to;
		}

	} // namespace state

} // namespace ket
