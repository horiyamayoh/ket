#pragma once

/**
 * @file ket_mac.h
 * @brief MAC addressのparse/format API。
 *
 * @details `AA:BB:CC:DD:EE:FF` または `aa-bb-cc-dd-ee-ff` 形式のMAC addressを、
 * 固定6 byteの値型へ変換し、値型から正準のcolon区切り文字列を生成する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。標準ライブラリにMAC addressの直接APIがないため、
 * 区切り文字とhex byteの検証をmodule内で扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_mac.h` と `ket_mac.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view` と `std::optional` で区切り文字とhex byteの失敗条件を固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::mac
 * 内部実装：.cpp の無名 namespace
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace mac
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief MAC address文字列の出力文字種。
		 */
		enum class LetterCase // NOLINT(performance-enum-size)
		{
			kLower,
			kUpper
		};

		/**
		 * @brief 固定6 byteのMAC address値。
		 */
		struct Address
		{
			std::uint8_t bytes[6] = {0, 0, 0, 0, 0, 0};
		};

		/**
		 * @brief MAC address値の等価比較。
		 * @param[in] lhs 比較対象の左辺値。
		 * @param[in] rhs 比較対象の右辺値。
		 * @retval true 6 byteすべてが一致。
		 * @retval false 1 byte以上が異なる。
		 * @pre なし。任意のAddress値を比較できる。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * ket::mac::Address lhs{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
		 * ket::mac::Address rhs{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
		 * const auto same = lhs == rhs;
		 * // same == true
		 * @endcode
		 */
		bool operator==(Address lhs, Address rhs) noexcept;

		/**
		 * @brief MAC address値の非等価比較。
		 * @param[in] lhs 比較対象の左辺値。
		 * @param[in] rhs 比較対象の右辺値。
		 * @retval true 1 byte以上が異なる。
		 * @retval false 6 byteすべてが一致。
		 * @pre なし。任意のAddress値を比較できる。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * ket::mac::Address lhs{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
		 * ket::mac::Address rhs{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00}};
		 * const auto different = lhs != rhs;
		 * // different == true
		 * @endcode
		 */
		bool operator!=(Address lhs, Address rhs) noexcept;

		/**
		 * @brief 区切り付きMAC address文字列の値変換。
		 * @param[in] text 変換対象のMAC address文字列。
		 * @retval value 変換後のMAC address値。
		 * @retval std::nullopt byte数不足/過多、不正hex、不正区切り、混在区切り、またはCisco形式。
		 * @pre なし。不正な入力表記は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note `:` または `-` のどちらか一方で区切られた6 byte形式のみを受け付ける。
		 * @code
		 * const auto value = ket::mac::Parse("AA:BB:CC:DD:EE:FF");
		 * // value->bytes == {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
		 * @endcode
		 */
		std::optional<Address> Parse(std::string_view text) noexcept;

		/**
		 * @brief MAC address値のcolon区切り文字列変換。
		 * @param[in] value 変換対象のMAC address値。
		 * @param[in] letter_case hex digitの大文字小文字。
		 * @retval value colon区切りのMAC address文字列。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。出力は常に6 byte、colon区切り、2桁hexの正準形。
		 * @note `letter_case` が `LetterCase::kUpper` の場合は大文字、それ以外は小文字で出力。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * ket::mac::Address address{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
		 * const auto text = ket::mac::Format(address);
		 * // text == "aa:bb:cc:dd:ee:ff"
		 * @endcode
		 */
		std::string Format(Address value, LetterCase letter_case = LetterCase::kLower);

	} // namespace mac

} // namespace ket
