#pragma once

/**
 * @file ket_ranges.h
 * @brief range要素のindex付き走査API。
 *
 * @details C++17以前で書きがちなindex付きrange走査と、最初に条件を満たす要素のindex取得を
 * 小さいAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 * iterator pair overloadやrange frameworkは扱わず、`begin`/`end`で走査できるobjectに限定。
 * ADLによるfree functionの`begin`/`end`も対象。
 *
 * @par プロジェクトへの適用方法
 * `ket_ranges.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：index付きrange走査を小さく書ける。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 * 標準代替：C++20の`std::ranges`
 * algorithmで一部用途を置き換え可能。ただしindex付き走査の直接代替ではない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::ranges
 * 内部実装：なし
 */

#include <cstddef>
#include <iterator> // IWYU pragma: keep

namespace ket
{
	namespace ranges
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief range要素へのindex付き順次処理。
		 * @param[in] range `begin`と`end`で走査するrange object。ADLによるfree functionも対象。
		 * @param[in,out] f 各要素へ適用するcallable。`f(index,
		 * element)`の形で同じobjectを呼び出す。
		 * @retval void 戻り値なし。
		 * @pre `range`は走査中に有効なbegin/endを提供する。走査中にiteratorを無効化する変更は
		 * 呼び出し側の責任。走査する要素数は`std::size_t`で表現可能。
		 * @post `f`の副作用を除き、module側では`range`と外部状態を変更しない。
		 * @note indexは0から始まり、要素順に1ずつ増加。
		 * @note
		 * `element`は`*it`の参照性とconst性を保持。`f`の例外は捕捉せず伝播。`f`はAPI内でcopyしない。
		 * @code
		 * std::vector<int> values = {10, 20};
		 * ket::ranges::ForEachWithIndex(values, [](std::size_t index, int& value) {
		 *     value += static_cast<int>(index);
		 * });
		 * // values == {10, 21}
		 * @endcode
		 */
		template <typename Range, typename F>
		void ForEachWithIndex(Range&& range, F&& f);

		/**
		 * @brief 条件を満たす最初の要素index取得。
		 * @param[in] range `begin`と`end`で走査するrange object。ADLによるfree functionも対象。
		 * @param[in,out] predicate
		 * 各要素へ適用する述語。`predicate(element)`の形で同じobjectを呼び出す。
		 * @param[out] out 最初に条件を満たした要素の0始まりindex。
		 * @retval true 条件を満たす要素が見つかり、`out`へindexを書き込み。
		 * @retval false 条件を満たす要素なし。`out`は入力時の値を保持。
		 * @pre `range`は走査中に有効なbegin/endを提供する。走査中にiteratorを無効化する変更は
		 * 呼び出し側の責任。走査する要素数は`std::size_t`で表現可能。
		 * @post `predicate`の副作用を除き、module側は成功時のみ`out`を変更。not
		 * found時と例外伝播時は`out`を変更しない。
		 * @note 最初の一致で短絡。`predicate`の例外は捕捉せず伝播。`predicate`はAPI内でcopyしない。
		 * @code
		 * const std::vector<int> values = {10, 20, 30};
		 * std::size_t index = 0U;
		 * const auto found = ket::ranges::FindIndexIf(values, [](int value) {
		 *     return value >= 20;
		 * }, index);
		 * // found == true, index == 1
		 * @endcode
		 */
		template <typename Range, typename Predicate>
		bool FindIndexIf(Range&& range, Predicate&& predicate, std::size_t& out);

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename Range, typename F>
		void ForEachWithIndex(Range&& range, F&& f)
		{
			using std::begin;
			using std::end;

			std::size_t index = 0U;
			auto it = begin(range);
			const auto range_end = end(range);

			for (; it != range_end; ++it)
			{
				f(index, *it);
				++index;
			}
		}

		template <typename Range, typename Predicate>
		bool FindIndexIf(Range&& range, Predicate&& predicate, std::size_t& out)
		{
			using std::begin;
			using std::end;

			std::size_t index = 0U;
			auto it = begin(range);
			const auto range_end = end(range);

			for (; it != range_end; ++it)
			{
				const bool matches = static_cast<bool>(predicate(*it));
				if (matches)
				{
					out = index;
					return true;
				}

				++index;
			}

			return false;
		}

	} // namespace ranges

} // namespace ket
