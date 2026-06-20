#pragma once

/**
 * @file ket_tuple.h
 * @brief tuple要素の反復と変換補助API。
 *
 * @details `std::apply` と fold expression
 * を使い、tuple要素ごとの副作用呼び出しと変換を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。呼び出し順と戻りtuple型をmodule内で固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_tuple.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::apply` と fold expression でtuple走査を小さく表現可能。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::tuple
 * 内部実装：ket::tuple::detail
 */

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ket
{
	namespace tuple
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief tuple要素へのindex順の副作用呼び出し。
		 * @param[in,out] tuple 走査対象のtuple-like
		 * object。non-const要素やrvalue要素は`f`により変更またはmoveされ得る。
		 * @param[in,out] f 各要素を1引数で受け取るcallable。渡されたcallable
		 * objectを直接呼び出し、API内でcopyしない。
		 * @retval void 戻り値なし。
		 * @pre
		 * `tuple`はstd::applyで展開可能。`f`はlvalue callable
		 * objectとして各転送済み要素型で呼び出し可能。
		 * @post
		 * `f`はtuple要素のindex昇順に最大1回ずつ同じobjectとして呼び出される。callable例外は呼び出し元へ伝播。
		 * @note `f`の戻り値は破棄。tuple要素のvalue categoryとcv修飾はstd::applyの規則に従う。
		 * @code
		 * auto values = std::make_tuple(1, 2, 3);
		 * int sum = 0;
		 * ket::tuple::ForEach(values, [&sum](int value) { sum += value; });
		 * // sum == 6
		 * @endcode
		 */
		template <typename Tuple, typename F>
		void ForEach(Tuple&& tuple, F&& f);

		/**
		 * @brief tuple要素のindex順変換。
		 * @param[in,out] tuple 変換対象のtuple-like
		 * object。non-const要素やrvalue要素は`f`により変更またはmoveされ得る。
		 * @param[in,out] f
		 * 各要素を1引数で受け取り、変換後の要素を返すcallable。渡されたcallable
		 * objectを直接呼び出し、API内でcopyしない。
		 * @retval value 各要素への`f`の戻り値を同じindex順に格納したstd::tuple。
		 * @pre
		 * `tuple`はstd::applyで展開可能。`f`はlvalue callable
		 * objectとして各転送済み要素型で呼び出し可能で、戻り値はstd::tuple要素として構成可能。戻りtupleの参照要素を使う場合、参照先は戻りtupleの利用中に有効。
		 * @post
		 * `f`はtuple要素のindex昇順に最大1回ずつ同じobjectとして呼び出される。引数と外部状態は`f`の副作用以外で変更なし。
		 * @note
		 * callable例外は呼び出し元へ伝播。`f`が参照を返す場合、戻り値は参照要素を持つtuple。rvalue
		 * tuple要素への参照を返すcallableではdangling referenceに注意。
		 * @code
		 * const auto values = std::make_tuple(1, 2, 3);
		 * const auto doubled = ket::tuple::Transform(values, [](int value) { return value * 2; });
		 * // std::get<0>(doubled) == 2, std::get<2>(doubled) == 6
		 * @endcode
		 */
		template <typename Tuple, typename F>
		auto Transform(Tuple&& tuple, F&& f);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 展開済みtuple要素への副作用呼び出し。
			 * @param[in,out] f 各要素を1引数で受け取るcallable。
			 * @param[in] elements 呼び出し対象のtuple要素。
			 * @retval void 戻り値なし。
			 * @pre `f`は各`elements`で呼び出し可能。
			 * @post
			 * `f`は`elements`の展開順に最大1回ずつ呼び出される。callable例外は呼び出し元へ伝播。
			 */
			template <typename F, typename... Elements>
			void ForEachElements(F& f, Elements&&... elements)
			{
				(static_cast<void>(std::invoke(f, std::forward<Elements>(elements))), ...);
			}

			/**
			 * @brief 展開済みtuple要素の変換。
			 * @param[in,out] f 各要素を1引数で受け取り、変換後の要素を返すcallable。
			 * @param[in] elements 変換対象のtuple要素。
			 * @retval value 各`elements`への`f`の戻り値を同じ順序で格納したstd::tuple。
			 * @pre `f`は各`elements`で呼び出し可能で、戻り値はstd::tuple要素として構成可能。
			 * @post
			 * `f`は`elements`の展開順に最大1回ずつ呼び出される。callable例外は呼び出し元へ伝播。
			 */
			template <typename F, typename... Elements>
			auto TransformElements(F& f, Elements&&... elements)
			{
				using ResultTuple = std::tuple<std::invoke_result_t<F&, Elements&&>...>;

				return ResultTuple{std::invoke(f, std::forward<Elements>(elements))...};
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename Tuple, typename F>
		void ForEach(Tuple&& tuple, F&& f)
		{
			std::apply(
				[&f](auto&&... elements)
				{
					detail::ForEachElements(f, std::forward<decltype(elements)>(elements)...);
				},
				std::forward<Tuple>(tuple));
		}

		template <typename Tuple, typename F>
		auto Transform(Tuple&& tuple, F&& f)
		{
			return std::apply(
				[&f](auto&&... elements)
				{
					return detail::TransformElements(f,
													 std::forward<decltype(elements)>(elements)...);
				},
				std::forward<Tuple>(tuple));
		}

	} // namespace tuple

} // namespace ket
