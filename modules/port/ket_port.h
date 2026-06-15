#pragma once

/**
 * @file ket_port.h
 * @brief TCP/UDP port番号の値型、parse、format API。
 *
 * @details 0から65535までのTCP/UDP port番号を小さい値型として扱い、wide整数からの
 * 範囲確認、10進文字列からのparse、10進文字列へのformatを提供する。drop-in時は宣言と
 * 実装を同じ単位で持ち出す。socket addressやservice name解決は扱わない。
 *
 * @par プロジェクトへの適用方法
 * `ket_port.h` と `ket_port.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`と`std::optional`で範囲外や不正文字を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::port
 * 内部実装：ket::port::detail
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace port
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief TCP/UDP port番号の値型。
		 * @note `value`は0から65535までのport番号。0はOSによる自動割り当て用途の有効値。
		 */
		struct Port
		{
			std::uint16_t value = 0;
		};

		/**
		 * @brief wide整数からport値型への範囲確認付き変換。
		 * @param[in] value 変換対象の整数値。
		 * @param[out] out 成功時の格納先。失敗時は変更なし。
		 * @retval true `value`が0から65535の範囲内で、`out`に変換結果を格納。
		 * @retval false `value`が65535を超過し、`out`は入力時の値を保持。
		 * @pre `out`は有効な参照。
		 * @post 成功時は`out.value == value`。失敗時は`out`を変更しない。外部状態の変更なし。
		 * @code
		 * ket::port::Port port;
		 * const auto ok = ket::port::TryFromUInt(443U, port);
		 * // ok == true, port.value == 443
		 * @endcode
		 */
		constexpr bool TryFromUInt(std::uint32_t value, Port& out) noexcept;

		/**
		 * @brief 10進文字列からport値型へのparse。
		 * @param[in] text 変換対象の10進文字列。
		 * @retval value 変換後のport値。
		 * @retval std::nullopt 空文字列、空白、符号、不正文字、範囲外、または複数桁のleading zero。
		 * @pre `text`が参照する範囲は呼び出し中読み取り可能。
		 * @post 引数と外部状態の変更なし。
		 * @note 入力はtrimしない。10進数字だけを完全消費し、`"0"`以外のleading zeroは失敗値。
		 * @code
		 * const auto port = ket::port::Parse("8080");
		 * // port == std::optional<ket::port::Port>({8080})
		 * @endcode
		 */
		std::optional<Port> Parse(std::string_view text) noexcept;

		/**
		 * @brief port値型の10進文字列format。
		 * @param[in] port 変換対象のport値。
		 * @retval value leading zeroなしの10進文字列。
		 * @pre なし。`Port::value`は`std::uint16_t`の全値を有効なport番号として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note `std::string`の生成がallocationを伴うためnoexceptなし。
		 * @code
		 * const auto text = ket::port::Format(ket::port::Port{443U});
		 * // text == "443"
		 * @endcode
		 */
		std::string Format(Port port);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief port番号として表現できる最大値。
			 * @note detail配下の値は公開APIではない。
			 */
			constexpr std::uint32_t kMaxPortValue = 65535U;

			/**
			 * @brief wide整数がport番号の範囲内か判定。
			 * @param[in] value 判定対象の整数値。
			 * @retval true 0から65535の範囲内。
			 * @retval false 65535超過。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsValidPortValue(std::uint32_t value) noexcept
			{
				return value <= kMaxPortValue;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr bool TryFromUInt(std::uint32_t value, Port& out) noexcept
		{
			const auto value_is_valid = detail::IsValidPortValue(value);
			if (!value_is_valid)
			{
				return false;
			}

			out.value = static_cast<std::uint16_t>(value);
			return true;
		}

	} // namespace port

} // namespace ket
