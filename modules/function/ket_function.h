#pragma once

/**
 * @file ket_function.h
 * @brief callable/visitorの小さい補助API。
 *
 * @details 複数callableをoverload setとして束ねる処理と、任意引数を破棄するNoop
 * callableを短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_function.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：variadic using declarationでvisitor overload setを小さく表現できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::function
 * 内部実装：なし
 */

#include <type_traits>
#include <utility>

namespace ket
{
	namespace function
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 1個以上のcallableを束ねるoverload set。
		 * @tparam Fs 値として保持する、重複しない継承可能なclass型。
		 * @pre `Fs`は重複しない継承可能なclass型で、有効なoverload setを構成する。
		 * @note 各callableのcopy/move制約と呼び出し例外は元の型に従う。
		 * @code
		 * const auto visitor = ket::function::MakeOverload(
		 *     [](int value) { return value; },
		 *     [](const std::string& value) { return static_cast<int>(value.size()); });
		 * @endcode
		 */
		template <typename... Fs>
		struct Overload : Fs...
		{
			static_assert(sizeof...(Fs) > 0,
						  "ket::function::Overload requires at least one callable.");
			static_assert((std::is_class_v<Fs> && ...),
						  "ket::function::Overload stores callable class types only.");

			using Fs::operator()...;
		};

		/**
		 * @brief `Overload{callable...}`のclass template argument deduction。
		 */
		template <typename... Fs>
		Overload(Fs...) -> Overload<Fs...>;

		/**
		 * @brief 1個以上のcallableからoverload setを生成。
		 * @param[in] fs `Overload`へ値として保持するcallable。
		 * @retval value `std::decay_t<Fs>`を値として保持する`Overload`。
		 * @pre 1つ以上のcallableを渡す。decay後の各callableは継承可能なclass型で、
		 * 型が重複せず、同時に有効なoverload setを構成する。
		 * @post 引数のcopy/move以外の外部状態の変更なし。
		 * @note lvalue callableはcopy、rvalue
		 * callableはmoveして保持。呼び出し例外はhandlerから伝播。
		 * @code
		 * const auto visitor = ket::function::MakeOverload(
		 *     [](int value) { return std::to_string(value); },
		 *     [](const std::string& value) { return value; });
		 * const auto text = std::visit(visitor, std::variant<int, std::string>(42));
		 * // text == "42"
		 * @endcode
		 */
		template <typename... Fs>
		constexpr Overload<std::decay_t<Fs>...> MakeOverload(Fs&&... fs) noexcept(
			noexcept(Overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...}));

		/**
		 * @brief 任意引数を無視するnoexcept callable。
		 */
		struct Noop
		{
			/**
			 * @brief 任意引数を受け取り何もしない。
			 * @param[in] args 無視する引数。
			 * @retval void 戻り値なし。
			 * @pre なし。任意の引数個数と型を受け付ける。
			 * @post 引数と外部状態の変更なし。
			 * @note 引数評価後の破棄のみを行い、operator自体は例外を送出しない。
			 * @code
			 * const auto noop = ket::function::Noop{};
			 * noop(1, "ignored");
			 * @endcode
			 */
			template <typename... Args>
			constexpr void operator()(Args&&... args) const noexcept
			{
				(static_cast<void>(args), ...);
			}
		};

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename... Fs>
		constexpr Overload<std::decay_t<Fs>...> MakeOverload(Fs&&... fs) noexcept(
			noexcept(Overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...}))
		{
			return Overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
		}

	} // namespace function

} // namespace ket
