#pragma once

/**
 * @file ket_ipv4.h
 * @brief IPv4 dotted decimalとBE 32bit表現の変換API。
 *
 * @details IPv4 addressのdotted decimal parse/formatとnetwork byte orderの32bit整数変換を扱う。
 * drop-in時は宣言と実装を同じ単位で持ち出す。IPv6、CIDR、DNS、port、socket addressは扱わず、
 * dotted decimalの境界条件を小さいAPIへ固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_ipv4.h` と `ket_ipv4.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`と`std::optional`でdotted decimalの失敗条件を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：標準ライブラリにIPv4 dotted decimal parse/formatの直接APIなし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::ipv4
 * 内部実装：ket::ipv4::detail
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace ipv4
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief IPv4 addressの4 octet値。
		 * @note `octets[0]`がdotted decimalの先頭要素、またはBE 32bit表現の最上位octet。
		 */
		struct Address
		{
			std::uint8_t octets[4] = {0, 0, 0, 0};
		};

		/**
		 * @brief dotted decimal文字列のIPv4 address変換。
		 * @param[in] text 変換対象のdotted decimal文字列。
		 * @retval value 変換後のIPv4 address。
		 * @retval std::nullopt octet範囲外、負値、空要素、要素不足/過多、空白、またはleading zero。
		 * @pre なし。不正なdotted decimalは失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note dotted decimalのみを受け付け、IPv6、CIDR、DNS名、port付き表記は扱わない。
		 * @code
		 * const auto value = ket::ipv4::Parse("192.168.0.1");
		 * // value->octets == {192, 168, 0, 1}
		 * @endcode
		 */
		std::optional<Address> Parse(std::string_view text) noexcept;

		/**
		 * @brief IPv4 addressのdotted decimal文字列変換。
		 * @param[in] value 変換対象のIPv4 address。
		 * @retval value dotted decimal文字列。
		 * @pre なし。任意の4 octet値をIPv4 addressとして扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const ket::ipv4::Address address = {{192, 168, 0, 1}};
		 * const auto text = ket::ipv4::Format(address);
		 * // text == "192.168.0.1"
		 * @endcode
		 */
		std::string Format(Address value);

		/**
		 * @brief IPv4 addressのBE 32bit整数変換。
		 * @param[in] value 変換対象のIPv4 address。
		 * @retval value network byte orderの32bit整数表現。
		 * @pre なし。`octets[0]`を最上位octetとして扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const ket::ipv4::Address address = {{192, 168, 0, 1}};
		 * const auto value = ket::ipv4::ToBe32(address);
		 * // value == 0xC0A80001
		 * @endcode
		 */
		std::uint32_t ToBe32(Address value) noexcept;

		/**
		 * @brief BE 32bit整数のIPv4 address変換。
		 * @param[in] value network byte orderの32bit整数表現。
		 * @retval value 変換後のIPv4 address。
		 * @pre なし。最上位byteを`octets[0]`へ展開する。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto address = ket::ipv4::FromBe32(0xC0A80001U);
		 * // address.octets == {192, 168, 0, 1}
		 * @endcode
		 */
		Address FromBe32(std::uint32_t value) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

	} // namespace ipv4

} // namespace ket
