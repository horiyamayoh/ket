#pragma once

/**
 * @file ket_variant.h
 * @brief std::variant visitor補助API。
 *
 * @details `std::visit`へ渡すoverload helperをmodule内に閉じ、variantのalternativeへ短く
 * dispatchする。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 * `std::variant`の標準動作を置き換えず、visitor束ね処理だけを薄く包む。lvalue handlerは
 * 内部visitorへcopyし、rvalue handlerはmoveする。
 *
 * @par プロジェクトへの適用方法
 * `ket_variant.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::variant`とvisitor補助を標準ライブラリのみで薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::variant
 * 内部実装：ket::variant::detail
 */

#include <type_traits>
#include <utility>
#include <variant>

namespace ket
{
	namespace variant
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief variant値を複数handlerへdispatch。
		 * @tparam Variant dispatch対象variant型。
		 * @tparam Handlers alternativeごとの呼び出し可能型。
		 * @param[in,out] variant dispatch対象のvariant値。constnessとvalue
		 * categoryはstd::visitへ保持。
		 * @param[in] handlers alternativeごとのhandler。lvalue handlerは内部visitorへcopyされ、
		 * rvalue handlerはmoveされる。
		 * @retval value 選択されたhandlerの戻り値。
		 * @retval void 選択されたhandlerがvoidを返す場合は戻り値なし。
		 * @pre `variant`はstd::visitへ渡せるvariant値。handler集合は全alternativeを受け付け、
		 * 戻り型はstd::visitの要件を満たす。
		 * @post handlerの副作用を除き、外部状態の追加変更なし。handler例外は呼び出し元へ伝播。
		 * @note valueless_by_exceptionではstd::visitと同じくstd::bad_variant_accessを送出。
		 * @note lvalue handlerはcopyされるため、handler自身の内部状態への参照を返す用途には
		 * 適さない。variant alternativeや外部objectへの参照返却は通常のlifetime規則に従う。
		 * @code
		 * std::variant<int, std::string> value = 42;
		 * const auto text = ket::variant::Match(
		 *     value,
		 *     [](int number) { return std::to_string(number); },
		 *     [](const std::string& string_value) { return string_value; });
		 * // text == "42"
		 * @endcode
		 */
		template <typename Variant, typename... Handlers>
		constexpr decltype(auto) Match(Variant&& variant, Handlers&&... handlers);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 複数handlerを1つのvisitorに束ねる内部型。
			 * @tparam Bases handlerを値として保持するbase型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename... Bases>
			struct Overload : Bases...
			{
				using Bases::operator()...;

				template <typename... Handlers>
				/**
				 * @brief handler baseを保持する内部visitor生成。
				 * @param[in] handlers visitorへ保持するhandler群。
				 * @pre 各handlerは対応するbase型を構築できる。
				 * @post 各baseは対応handlerから構築される。
				 */
				constexpr explicit Overload(Handlers&&... handlers)
					: Bases(std::forward<Handlers>(handlers))...
				{
				}
			};

			/**
			 * @brief handler群から内部visitorを生成。
			 * @param[in] handlers alternativeごとのhandler。
			 * @retval value handlerを値として保持する内部visitor。
			 * @pre 各handlerはdecay後の型へcopyまたはmove可能。
			 * @post rvalue handlerは内部visitorへmoveされ、lvalue handlerはcopyされる。
			 */
			template <typename... Handlers>
			constexpr Overload<std::decay_t<Handlers>...> MakeOverload(Handlers&&... handlers)
			{
				return Overload<std::decay_t<Handlers>...>(std::forward<Handlers>(handlers)...);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename Variant, typename... Handlers>
		constexpr decltype(auto) Match(Variant&& variant, Handlers&&... handlers)
		{
			auto visitor = detail::MakeOverload(std::forward<Handlers>(handlers)...);
			return std::visit(visitor, std::forward<Variant>(variant));
		}

	} // namespace variant

} // namespace ket
