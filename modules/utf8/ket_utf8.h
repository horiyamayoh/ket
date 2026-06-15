#pragma once

/**
 * @file ket_utf8.h
 * @brief UTF-8 byte列の検査API。
 *
 * @details UTF-8の妥当性、ASCII限定判定、code point数取得を小さいAPIへ集約する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。normalization、grapheme cluster、
 * encoding conversionは扱わず、UTF-8 byte列検査の境界条件だけをmodule内で固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_utf8.h` と `ket_utf8.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view` と `std::optional` でUTF-8検査結果と失敗位置を小さく扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::utf8
 * 内部実装：ket::utf8::detail
 */

#include <cstddef>
#include <optional>
#include <string_view>

namespace ket
{
	namespace utf8
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief UTF-8検査結果。
		 *
		 * @details `valid`がtrueの場合、入力byte列は妥当なUTF-8。
		 * `valid`がfalseの場合、`error_offset`は最初に不正と判定したbyte位置。
		 */
		struct ValidationResult
		{
			bool valid = false;
			std::size_t error_offset = 0;
		};

		/**
		 * @brief ASCII限定byte列の判定。
		 * @param[in] text 判定対象のbyte列。
		 * @retval true `text`の全byteが`0x00..0x7F`。
		 * @retval false `text`が`0x80`以上のbyteを含む。
		 * @pre なし。任意のbyte列を判定対象として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note 空文字列はASCII限定byte列として扱う。
		 */
		bool IsAscii(std::string_view text) noexcept;

		/**
		 * @brief UTF-8 byte列の妥当性検査。
		 * @param[in] text 検査対象のbyte列。
		 * @retval result UTF-8検査結果。妥当な場合は`valid == true`。
		 * @pre なし。任意のbyte列を検査対象として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note truncated sequenceは、そのsequenceの先頭byteを`error_offset`として返す。
		 * @note overlong、surrogate、範囲外code point、単独continuation byteはinvalid。
		 */
		ValidationResult Validate(std::string_view text) noexcept;

		/**
		 * @brief UTF-8 byte列の妥当性判定。
		 * @param[in] text 判定対象のbyte列。
		 * @retval true `text`が妥当なUTF-8。
		 * @retval false `text`が不正なUTF-8。
		 * @pre なし。任意のbyte列を判定対象として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note `Validate(text).valid`と同義。
		 */
		bool IsValid(std::string_view text) noexcept;

		/**
		 * @brief UTF-8 byte列のcode point数取得。
		 * @param[in] text 数える対象のbyte列。
		 * @retval value UTF-8 code point数。
		 * @retval std::nullopt `text`が不正なUTF-8。
		 * @pre なし。任意のbyte列を入力として扱い、不正UTF-8は失敗値として返す。
		 * @post 引数と外部状態の変更なし。
		 * @note grapheme cluster数ではなくUnicode code point数。
		 */
		std::optional<std::size_t> CountCodePoints(std::string_view text) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

	} // namespace utf8

} // namespace ket
