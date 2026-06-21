#pragma once

/**
 * @file ket_hex.h
 * @brief byte列、整数、診断用hexdumpのhex文字列化API。
 *
 * @details byte列とhex文字列の相互変換、整数の最小幅hex表記、固定形式hexdumpを扱う。
 * drop-in時は宣言と実装を同じ単位で持ち出す。標準ライブラリにbyte列hex parse/dumpの
 * 直接APIがないため、ASCII whitespace、不正hex、dump表示幅の扱いをmodule内で固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_hex.h` と `ket_hex.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view` と `std::optional` で失敗と入力範囲を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::hex
 * 内部実装：.cpp の無名 namespace
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ket
{
	namespace hex
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief hex英字の大文字小文字指定。
		 */
		enum class LetterCase // NOLINT(performance-enum-size)
		{
			kLower,
			kUpper
		};

		/**
		 * @brief byte列hex文字列化の表記option。
		 */
		struct FormatOptions
		{
			LetterCase letter_case = LetterCase::kUpper;
			char separator = '\0';
		};

		/**
		 * @brief 整数のhex文字列化。
		 * @param[in] value 文字列化対象の整数。
		 * @param[in] min_width 出力する最小hex桁数。桁数不足分は左側を0で埋める。
		 * @param[in] letter_case 10以上のhex digitに使う英字の大文字小文字。
		 * @retval value hex文字列。`value`の実桁が`min_width`を超える場合は実桁を保持。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note `value == 0` では `min_width == 0` でも1桁の`"0"`を返す。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const auto text = ket::hex::Format(0x2aU, 4U, ket::hex::LetterCase::kLower);
		 * // text == "002a"
		 * @endcode
		 */
		std::string Format(std::uint64_t value,
						   unsigned min_width = 0,
						   LetterCase letter_case = LetterCase::kUpper);

		/**
		 * @brief byte列のhex文字列化。
		 * @param[in] data 文字列化対象のbyte列。
		 * @param[in] size `data`のバイト数。
		 * @param[in] options 大文字小文字とbyte間separatorの指定。
		 * @retval value hex文字列。`size == 0` の場合は空文字列。
		 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr` は `size == 0`
		 * の場合だけ許容し、`nullptr` かつ非0 sizeはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note `options.separator == '\0'` の場合はseparatorを挿入しない。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const std::uint8_t data[] = {0xDEU, 0xADU};
		 * const auto text = ket::hex::Encode(data, 2U);
		 * // text == "DEAD"
		 * @endcode
		 */
		std::string Encode(const std::uint8_t* data, std::size_t size, FormatOptions options = {});

		/**
		 * @brief hex文字列のbyte列変換。
		 * @param[in] text 変換対象のhex文字列。
		 * @retval value 変換後のbyte列。空入力またはASCII whitespaceのみの入力では空vector。
		 * @retval std::nullopt 奇数桁、不正hex文字、またはASCII whitespace以外の非hex文字。
		 * @pre なし。空入力と不正入力は戻り値で区別。
		 * @post 引数と外部状態の変更なし。
		 * @note ASCII whitespaceは`0x09`から`0x0d`と`0x20`のみ許容。
		 * @note separatorつきhexの自動parseは行わない。separatorは不正文字として扱う。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * const auto bytes = ket::hex::Decode("de ad BE EF");
		 * // bytes == std::optional<std::vector<std::uint8_t>>({0xde, 0xad, 0xbe, 0xef})
		 * @endcode
		 */
		std::optional<std::vector<std::uint8_t>> Decode(std::string_view text);

		/**
		 * @brief byte列の固定形式hexdump生成。
		 * @param[in] data dump対象のbyte列。
		 * @param[in] size `data`のバイト数。
		 * @retval value 固定形式hexdump文字列。`size == 0` の場合は空文字列。
		 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr` は `size == 0`
		 * の場合だけ許容し、`nullptr` かつ非0 sizeはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 16 bytes/row、最小8桁zero-pad lower hex offset、lower hex byte、8 byteごとの
		 * 追加space、`|...|`で囲むASCII previewの固定形式。ASCII previewは`0x20`から`0x7e`
		 * をそのまま表示し、それ以外は`.`。最終行のhex欠落分はspace
		 * padding。行末のnewlineは付けない。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const std::uint8_t data[] = {0x00U, 0x41U};
		 * const auto text = ket::hex::Dump(data, 2U);
		 * // text == "00000000  00 41                                             |.A|"
		 * @endcode
		 */
		std::string Dump(const std::uint8_t* data, std::size_t size);

		/**
		 * @brief 型なしメモリ範囲の固定形式hexdump生成。
		 * @param[in] data dump対象のメモリ範囲。
		 * @param[in] size `data`のバイト数。
		 * @retval value 固定形式hexdump文字列。`size == 0` の場合は空文字列。
		 * @pre `data`は`size`バイト以上読み取り可能なメモリ範囲を指す。`nullptr` は
		 * `size == 0` の場合だけ許容し、`nullptr` かつ非0 sizeはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note C API境界の`void*` bufferをbyte列としてdumpするための明示API。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const std::uint32_t value = 0x12345678U;
		 * const auto text = ket::hex::DumpMemory(&value, sizeof(value));
		 * // textは`value`のobject representationをdumpした文字列
		 * @endcode
		 */
		std::string DumpMemory(const void* data, std::size_t size);

	} // namespace hex

} // namespace ket
