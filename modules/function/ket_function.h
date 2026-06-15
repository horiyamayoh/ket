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
 * 内部実装：ket::function::detail
 */

#include <utility>

namespace ket
{
	namespace function
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 複数callableを束ねるoverload set。
		 * @tparam Fs 値として保持するcallable型。
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
			using Fs::operator()...;
		};

		/**
		 * @brief 複数callableからoverload setを生成。
		 * @param[in] fs 値として保持するcallable。
		 * @retval value `fs`を値として保持する`Overload`。
		 * @pre 各callableは継承可能なclass型で、同時に有効なoverload setを構成する。
		 * @post 引数のcopy/move以外の外部状態の変更なし。
		 * @note callableのcopy/move制約は型に従い、呼び出し例外はhandlerから伝播。
		 * @code
		 * const auto visitor = ket::function::MakeOverload(
		 *     [](int value) { return value + 1; },
		 *     [](double value) { return static_cast<int>(value); });
		 * @endcode
		 */
		template <typename... Fs>
		Overload<Fs...> MakeOverload(Fs... fs);

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
			 */
			template <typename... Args>
			void operator()(Args&&... args) const noexcept
			{
				(static_cast<void>(args), ...);
			}
		};

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename... Fs>
		Overload<Fs...> MakeOverload(Fs... fs)
		{
			return Overload<Fs...>{std::move(fs)...};
		}

	} // namespace function

} // namespace ket
