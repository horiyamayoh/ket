#pragma once

/**
 * @file ket_version.h
 * @brief numeric version tripletのparse/format/compare API。
 *
 * @details `major.minor.patch`形式の数値3要素だけを扱い、full SemVerの
 * prerelease、build metadata、range syntaxには踏み込まない。drop-in時は宣言と実装を
 * 同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_version.h` と `ket_version.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`と`std::optional`でparse失敗と比較を小さく固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::version
 * 内部実装：ket::version::detail
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace version
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief `major.minor.patch`の数値3要素。
		 * @note 各要素は`std::uint32_t`として保持。SemVerのprereleaseやbuild metadataは保持なし。
		 */
		struct Triplet
		{
			std::uint32_t major = 0;
			std::uint32_t minor = 0;
			std::uint32_t patch = 0;
		};

		/**
		 * @brief `major.minor.patch`形式のnumeric version triplet parse。
		 * @param[in] text parse対象の文字列。
		 * @retval value parse済みの数値3要素。
		 * @retval std::nullopt 空、要素不足/過多、非数字、overflow、または複数桁のleading zero。
		 * @pre なし。不正入力は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note 各要素は`std::uint32_t`範囲の10進ASCII数字列。`0`単体は許可。
		 * @code
		 * const auto value = ket::version::Parse("1.20.300");
		 * // value == std::optional<ket::version::Triplet>({1, 20, 300})
		 * @endcode
		 */
		std::optional<Triplet> Parse(std::string_view text) noexcept;

		/**
		 * @brief numeric version tripletの文字列化。
		 * @param[in] version 文字列化対象の数値3要素。
		 * @retval value `major.minor.patch`形式の文字列。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const auto text = ket::version::Format({1U, 20U, 300U});
		 * // text == "1.20.300"
		 * @endcode
		 */
		std::string Format(Triplet version);

		/**
		 * @brief numeric version tripletの辞書順比較。
		 * @param[in] a 比較左辺の数値3要素。
		 * @param[in] b 比較右辺の数値3要素。
		 * @retval negative `a`が`b`より小さい。
		 * @retval 0 `a`と`b`が等しい。
		 * @retval positive `a`が`b`より大きい。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note 比較順はmajor、minor、patch。
		 * @code
		 * const auto order = ket::version::Compare({1U, 2U, 3U}, {1U, 3U, 0U});
		 * // order < 0
		 * @endcode
		 */
		int Compare(Triplet a, Triplet b) noexcept;

	} // namespace version

} // namespace ket
