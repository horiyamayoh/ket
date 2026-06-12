#pragma once

// 持ち出し条件:
//   ket_bcd.h と ket_bcd.cpp を対象プロジェクトへコピー。
//
// 依存:
//   C++17以降
//   標準ライブラリのみ
//   他のket moduleへの依存なし
//
// Namespace:
//   ket

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace ket
{
	namespace detail
	{
		/**
		 * @brief BCD nibble妥当性判定。
		 * @param[in] value 判定対象のnibble値。
		 * @retval true 0から9のBCD nibble。
		 * @retval false BCD nibble範囲外。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note constexprな公開APIを保つため、整数BCD用の内部処理はヘッダ内に配置。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr bool IsBcdNibble(std::uint32_t value) noexcept
		{
			return value <= 9U;
		}

		/**
		 * @brief BCD digitの10進整数への追加。
		 * @param[in] value 追加前の10進整数。
		 * @param[in] digit 追加対象のBCD digit。
		 * @retval value 追加後の10進整数。
		 * @retval std::nullopt 不正nibble、またはint範囲外。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr std::optional<int> AppendBcdDigit(int value, std::uint32_t digit) noexcept
		{
			auto const digit_is_bcd = IsBcdNibble(digit);
			if (!digit_is_bcd)
			{
				return std::nullopt;
			}

			auto const digit_value = static_cast<int>(digit);
			auto const max_int = std::numeric_limits<int>::max();
			auto const max_value_before_append = (max_int - digit_value) / 10;
			auto const value_overflows = value > max_value_before_append;
			if (value_overflows)
			{
				return std::nullopt;
			}

			return (value * 10) + digit_value;
		}

		/**
		 * @brief 固定nibble数のパックBCDの10進整数変換。
		 * @param[in] value 変換対象のパックBCD値。
		 * @param[in] nibble_count 変換対象nibble数。
		 * @retval value 変換後の10進整数。
		 * @retval std::nullopt 不正nibble、またはint範囲外。
		 * @pre `0 <= nibble_count <= 8`。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr std::optional<int> ParseBcdNibbles(std::uint32_t value, int nibble_count) noexcept
		{
			int result = 0;

			for (int index = nibble_count - 1; index >= 0; --index)
			{
				auto const digit = (value >> (index * 4)) & 0x0FU;
				auto const next = AppendBcdDigit(result, digit);
				auto const next_has_value = next.has_value();
				if (!next_has_value)
				{
					// BCDではないnibble、またはintへ収まらない値だった
					return std::nullopt;
				}

				result = *next;
			}

			return result;
		}

	} // namespace detail

	/**
	 * @brief 2桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 */
	constexpr std::optional<int> ParseBcd(std::uint8_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 2);
	}

	/**
	 * @brief 4桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 */
	constexpr std::optional<int> ParseBcd(std::uint16_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 4);
	}

	/**
	 * @brief 8桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 */
	constexpr std::optional<int> ParseBcd(std::uint32_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 8);
	}

	/**
	 * @brief 任意バイト長パックBCDの10進文字列変換。
	 * @param[in] data 変換対象のパックBCD列。
	 * @param[in] size `data`のバイト数。
	 * @retval value 変換後の10進文字列。
	 * @retval std::nullopt `nullptr`、空入力、または不正nibble。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 先頭ゼロを保持。
	 * @note std::stringの確保があるためnoexceptなし。
	 */
	std::optional<std::string> BcdToDecimalString(std::uint8_t const* data, std::size_t size);

} // namespace ket
