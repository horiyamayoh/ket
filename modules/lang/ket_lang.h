#pragma once

/**
 * @file ket_lang.h
 * @brief C++言語の小さい儀式を名前付きAPIにする補助。
 *
 * @details 未使用値の明示破棄、raw配列長の取得、lvalueのconst参照化を小さいAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。AsConstはrvalueを拒否し、
 * 状態やコピー・ムーブ制御は扱わず、C++11/14で冗長になりやすい言語儀式に限定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_lang.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：C++11/14の欠落や冗長な言語儀式を小さいAPIで名前付けできる。
 * 本ライブラリの適用を推奨しない C++ バージョン：API別。
 * 非推奨理由：APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::lang
 * 内部実装：ket::lang::detail
 */

#include <cstddef>
#include <type_traits>

namespace ket
{
	namespace lang
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 未使用値の明示破棄。
		 * @param[in] args 未使用として扱う値。
		 * @retval void 戻り値なし。
		 * @pre 引数式は呼び出し前に評価済み。副作用の有無は呼び出し側の責任。
		 * @post 引数と外部状態の変更なし。参照束縛のみでコピーやムーブなし。
		 * @note C++17 `[[maybe_unused]]` と一部重複するが、式の明示破棄用途として採用。
		 * @code
		 * ket::lang::IgnoreUnused(argc, argv);
		 * @endcode
		 */
		template <typename... Args>
		void IgnoreUnused(const Args&... args) noexcept;

		/**
		 * @brief raw配列の要素数取得。
		 * @param[in] array 要素数を取得するraw配列。
		 * @retval value `array`の要素数。
		 * @pre `array`は有効なraw配列参照。ポインタ、動的配列、空配列は対象外。
		 * @post 引数と外部状態の変更なし。
		 * @note C++17 `std::size` の限定的なC++11代替。
		 * @code
		 * int values[3] = {};
		 * const auto count = ket::lang::ArraySize(values);
		 * // count == 3
		 * @endcode
		 */
		template <typename T, std::size_t N>
		constexpr std::size_t ArraySize(const T (&array)[N]) noexcept;

		// NOLINTBEGIN(modernize-type-traits): C++11 public signature from catalog.
		/**
		 * @brief lvalueのconst参照取得。
		 * @param[in] value const参照として扱うlvalue。
		 * @retval value `value`と同じobjectを参照するconst参照。
		 * @pre `value`は呼び出し後も参照が有効なlvalue。rvalueは受け付けない。
		 * @post 引数と外部状態の変更なし。戻り値は入力objectの寿命に従う。
		 * @note C++17 `std::as_const` のC++11代替。
		 * @code
		 * int value = 42;
		 * const int& view = ket::lang::AsConst(value);
		 * @endcode
		 */
		template <typename T>
		constexpr typename std::add_const<T>::type& AsConst(T& value) noexcept;

		/**
		 * @brief rvalueのconst参照化を禁止。
		 * @param[in] value 参照寿命が呼び出し直後に終わりうるrvalue。
		 * @retval void 使用不可。rvalue入力はcompile error。
		 * @pre 呼び出し不可。danglingしうる参照生成を削除overloadで拒否する。
		 * @post なし。overload resolutionで削除関数として扱われる。
		 * @note C++17 `std::as_const` と同じくconst rvalueも拒否する。
		 * @code
		 * // ket::lang::AsConst(123); // compile error
		 * @endcode
		 */
		template <typename T>
		void AsConst(const T&& value) = delete;
		// NOLINTEND(modernize-type-traits)

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief parameter packの各値を使用済みにする内部helper。
			 * @param[in] args 使用済みにする値。
			 * @retval void 戻り値なし。
			 * @pre 引数式は呼び出し前に評価済み。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename... Args>
			void IgnoreUnusedImpl(const Args&... args) noexcept
			{
				const int unused[] = {0, (static_cast<void>(args), 0)...};
				static_cast<void>(unused);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename... Args>
		void IgnoreUnused(const Args&... args) noexcept
		{
			detail::IgnoreUnusedImpl(args...);
		}

		template <typename T, std::size_t N>
		constexpr std::size_t ArraySize(const T (&array)[N]) noexcept
		{
			return sizeof(array) / sizeof(array[0]);
		}

		// NOLINTBEGIN(modernize-type-traits): C++11 public signature from catalog.
		template <typename T>
		constexpr typename std::add_const<T>::type& AsConst(T& value) noexcept
		{
			return value;
		}
		// NOLINTEND(modernize-type-traits)

	} // namespace lang

} // namespace ket
