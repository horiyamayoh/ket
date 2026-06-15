#pragma once

/**
 * @file ket_meta.h
 * @brief C++11/14欠落type traitsの小補助。
 *
 * @details template errorを読みやすく保つ小さいalias/typeを提供する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。評価時の実行動作は持たず、
 * C++11/14で不足するtraitsを局所的に補う。
 *
 * @par プロジェクトへの適用方法
 * `ket_meta.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：C++11/14で不足する小さいtraitsを局所的に補える。
 * 本ライブラリの適用を推奨しない C++ バージョン：API別。
 * 非推奨理由：APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::meta
 * 内部実装：ket::meta::detail
 */

#include <type_traits>

namespace ket
{
	namespace meta
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief `T`から参照とtop-level cv修飾を取り除いた型。
		 * @tparam T 変換対象の型。
		 * @note C++20 `std::remove_cvref` の小さいC++11/14向け補助。
		 */
		template <typename T>
		// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件の公開signature。
		using RemoveCvref = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

		/**
		 * @brief `T`を`type`としてそのまま返すidentity type trait。
		 * @tparam T 保持する型。
		 * @note C++20 `std::type_identity` の小さいC++11/14向け補助。
		 */
		template <typename T>
		struct TypeIdentity
		{
			using type = T;
		};

		/**
		 * @brief 任意のtemplate引数に対して常にfalseとなるtype trait。
		 * @tparam Types 診断対象のtemplate引数列。
		 * @note dependent falseが必要なstatic_assertでtemplate diagnosticsを保つための補助。
		 */
		template <typename... Types>
		struct AlwaysFalse : std::false_type
		{
		};

		/**
		 * @brief 任意のtemplate引数列を`void`へ写像するalias。
		 * @tparam Types SFINAEで検査する型列。
		 * @note C++17 `std::void_t` のC++11/14向け補助。
		 */
		template <typename... Types>
		using VoidT = void;

	} // namespace meta

} // namespace ket
