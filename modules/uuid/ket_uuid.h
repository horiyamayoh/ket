#pragma once

/**
 * @file ket_uuid.h
 * @brief UUID の parse/format API。
 *
 * @details canonical hyphen形式の UUID 文字列と16 byte表現の相互変換を短いAPIへ集約する。
 * generation、URN形式、version/variant validation は扱わない。drop-in時は宣言と実装を同じ単位で
 * 持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_uuid.h` と `ket_uuid.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`と`std::optional`でcanonical UUIDの失敗条件を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::uuid
 * 内部実装：.cpp の無名 namespace
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace uuid
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief UUID の16 byte表現。
		 * @note `bytes[0]`はcanonical文字列の先頭2桁、`bytes[15]`は末尾2桁に対応。
		 */
		struct Uuid
		{
			std::array<std::uint8_t, 16U> bytes = {};
		};

		/**
		 * @brief canonical hyphen形式 UUID 文字列のparse。
		 * @param[in] text parse対象の文字列。
		 * @retval value parse後の UUID。
		 * @retval std::nullopt 長さ不一致、hyphen位置不一致、不正hex、brace/URN形式、
		 * またはcanonical hyphen形式以外。
		 * @pre なし。canonical hyphen形式以外は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。version/variant bit の妥当性検証なし。
		 * @note hex は upper/lower の両方を受け付ける。
		 * @code
		 * const auto value = ket::uuid::Parse("00112233-4455-6677-8899-aabbccddeeff");
		 * // value.has_value() == true
		 * @endcode
		 */
		std::optional<Uuid> Parse(std::string_view text) noexcept;

		/**
		 * @brief UUID のcanonical hyphen形式文字列化。
		 * @param[in] value 文字列化対象の UUID。
		 * @retval value lower-case 固定のcanonical hyphen形式文字列。
		 * @pre `value.bytes` は16 byteの UUID 表現。
		 * @post 引数と外部状態の変更なし。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * const auto value = ket::uuid::Uuid{{0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U,
		 *                                    0x66U, 0x77U, 0x88U, 0x99U, 0xaaU, 0xbbU,
		 *                                    0xccU, 0xddU, 0xeeU, 0xffU}};
		 * const auto text = ket::uuid::Format(value);
		 * // text == "00112233-4455-6677-8899-aabbccddeeff"
		 * @endcode
		 */
		std::string Format(const Uuid& value);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

	} // namespace uuid

} // namespace ket
