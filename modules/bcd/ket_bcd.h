#pragma once

/**
 * @file ket_bcd.h
 * @brief packed BCDと10進表現の変換API。
 *
 * @details 固定幅packed BCDと任意バイト長packed BCDを、整数または10進文字列へ相互変換する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。標準ライブラリにpacked BCDの直接APIがないため、
 * BCD固有のnibble検証と桁保持をmodule内で扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_bcd.h` と `ket_bcd.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：packed BCDの直接代替が標準ライブラリになく、`std::optional`で失敗値を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket
 * 内部実装：ket::detail
 */

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ket
{
	// -----------------------------------------------------------------------------
	// Public API declarations
	// -----------------------------------------------------------------------------

	/**
	 * @brief 2桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 * @code
	 * const auto value = ket::ParseBcd(static_cast<std::uint8_t>(0x42U));
	 * // value == std::optional<int>(42)
	 * @endcode
	 */
	constexpr std::optional<int> ParseBcd(std::uint8_t value) noexcept;

	/**
	 * @brief 4桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 * @code
	 * const auto value = ket::ParseBcd(static_cast<std::uint16_t>(0x1234U));
	 * // value == std::optional<int>(1234)
	 * @endcode
	 */
	constexpr std::optional<int> ParseBcd(std::uint16_t value) noexcept;

	/**
	 * @brief 8桁固定幅パックBCDの10進整数変換。
	 * @param[in] value 変換対象のパックBCD値。
	 * @retval value 変換後の10進整数。
	 * @retval std::nullopt 不正nibble、またはint範囲外。
	 * @pre なし。
	 * @post 引数と外部状態の変更なし。
	 * @note 整数変換のため、先頭ゼロは保持なし。
	 * @code
	 * const auto value = ket::ParseBcd(static_cast<std::uint32_t>(0x20260613U));
	 * // value == std::optional<int>(20260613)
	 * @endcode
	 */
	constexpr std::optional<int> ParseBcd(std::uint32_t value) noexcept;

	/**
	 * @brief 2桁固定幅パックBCDへの10進整数変換。
	 * @param[in] value 変換対象の10進整数。
	 * @retval value 変換後のパックBCD値。
	 * @retval std::nullopt 負数、または2桁を超える値。
	 * @pre なし。負数と桁数超過は失敗値として扱う。
	 * @post 引数と外部状態の変更なし。
	 * @note 2桁固定幅のため、1桁値は上位nibbleを0として表現。
	 * @code
	 * const auto value = ket::ToBcd8(42);
	 * // value == std::optional<std::uint8_t>(0x42)
	 * @endcode
	 */
	constexpr std::optional<std::uint8_t> ToBcd8(int value) noexcept;

	/**
	 * @brief 4桁固定幅パックBCDへの10進整数変換。
	 * @param[in] value 変換対象の10進整数。
	 * @retval value 変換後のパックBCD値。
	 * @retval std::nullopt 負数、または4桁を超える値。
	 * @pre なし。負数と桁数超過は失敗値として扱う。
	 * @post 引数と外部状態の変更なし。
	 * @note 4桁固定幅のため、短い値は上位nibbleを0として表現。
	 * @code
	 * const auto value = ket::ToBcd16(42);
	 * // value == std::optional<std::uint16_t>(0x0042)
	 * @endcode
	 */
	constexpr std::optional<std::uint16_t> ToBcd16(int value) noexcept;

	/**
	 * @brief 8桁固定幅パックBCDへの10進整数変換。
	 * @param[in] value 変換対象の10進整数。
	 * @retval value 変換後のパックBCD値。
	 * @retval std::nullopt 負数、または8桁を超える値。
	 * @pre なし。負数と桁数超過は失敗値として扱う。
	 * @post 引数と外部状態の変更なし。
	 * @note 8桁固定幅のため、短い値は上位nibbleを0として表現。
	 * @code
	 * const auto value = ket::ToBcd32(20260613);
	 * // value == std::optional<std::uint32_t>(0x20260613)
	 * @endcode
	 */
	constexpr std::optional<std::uint32_t> ToBcd32(int value) noexcept;

	/**
	 * @brief 任意バイト長パックBCDの10進文字列変換。
	 * @param[in] data 変換対象のパックBCD列。
	 * @param[in] size `data`のバイト数。
	 * @retval value 変換後の10進文字列。
	 * @retval std::nullopt `nullptr`、空入力、または不正nibble。
	 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr` と空入力は失敗値として扱う。
	 * @post 引数と外部状態の変更なし。
	 * @note 入力BCDの桁数を保ち、先頭の0も文字'0'として出力。
	 * @note std::stringの確保があるためnoexceptなし。
	 * @code
	 * const std::uint8_t data[] = {0x00U, 0x42U};
	 * const auto value = ket::BcdToDecimalString(data, 2U);
	 * // value == std::optional<std::string>("0042")
	 * @endcode
	 */
	std::optional<std::string> BcdToDecimalString(const std::uint8_t* data, std::size_t size);

	/**
	 * @brief 10進文字列の任意バイト長パックBCD変換。
	 * @param[in] text 変換対象の10進文字列。
	 * @retval value 変換後のパックBCD列。
	 * @retval std::nullopt 空入力、または10進数字以外を含む入力。
	 * @pre なし。空入力と10進数字以外は失敗値として扱う。
	 * @post 引数と外部状態の変更なし。
	 * @note 偶数桁は2桁ずつpacked BCDへ変換し、奇数桁は先頭に0を補って変換。
	 * @note std::vectorの確保があるためnoexceptなし。
	 * @code
	 * const auto value = ket::DecimalStringToBcd("123");
	 * // value == std::optional<std::vector<std::uint8_t>>({0x01, 0x23})
	 * @endcode
	 */
	std::optional<std::vector<std::uint8_t>> DecimalStringToBcd(std::string_view text);

	// -----------------------------------------------------------------------------
	// Internal implementation details
	// -----------------------------------------------------------------------------

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
			const auto digit_is_bcd = IsBcdNibble(digit);
			if (!digit_is_bcd)
			{
				return std::nullopt;
			}

			const auto digit_value = static_cast<int>(digit);
			const auto max_int = std::numeric_limits<int>::max();
			const auto max_value_before_append = (max_int - digit_value) / 10;
			const auto value_overflows = value > max_value_before_append;
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
				const auto digit = (value >> (index * 4)) & 0x0FU;
				const auto next = AppendBcdDigit(result, digit);
				const auto next_has_value = next.has_value();
				if (!next_has_value)
				{
					// BCDではないnibble、またはintへ収まらない値だった
					return std::nullopt;
				}

				result = *next;
			}

			return result;
		}

		/**
		 * @brief 固定nibble数に収まる10進整数上限の取得。
		 * @param[in] nibble_count 変換対象nibble数。
		 * @retval value `10 ^ nibble_count`。
		 * @pre `0 <= nibble_count <= 8`。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr int DecimalLimitForBcdNibbles(int nibble_count) noexcept
		{
			int limit = 1;

			for (int index = 0; index < nibble_count; ++index)
			{
				limit *= 10;
			}

			return limit;
		}

		/**
		 * @brief 固定nibble数パックBCDへの10進整数変換。
		 * @param[in] value 変換対象の10進整数。
		 * @param[in] nibble_count 出力BCDのnibble数。
		 * @retval value 変換後のパックBCD値。
		 * @retval std::nullopt 負数、または指定nibble数の桁数超過。
		 * @pre `0 <= nibble_count <= 8`。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr std::optional<std::uint32_t> ToBcdNibbles(int value, int nibble_count) noexcept
		{
			const auto value_is_negative = value < 0;
			if (value_is_negative)
			{
				return std::nullopt;
			}

			const auto decimal_limit = DecimalLimitForBcdNibbles(nibble_count);
			const auto value_too_large = value >= decimal_limit;
			if (value_too_large)
			{
				return std::nullopt;
			}

			std::uint32_t result = 0;
			auto remaining = value;

			for (int index = 0; index < nibble_count; ++index)
			{
				const auto digit = static_cast<std::uint32_t>(remaining % 10);
				const auto shift = static_cast<std::uint32_t>(index * 4);
				result |= digit << shift;
				remaining /= 10;
			}

			return result;
		}

	} // namespace detail

	// -----------------------------------------------------------------------------
	// Public API definitions
	// -----------------------------------------------------------------------------

	constexpr std::optional<int> ParseBcd(std::uint8_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 2);
	}

	constexpr std::optional<int> ParseBcd(std::uint16_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 4);
	}

	constexpr std::optional<int> ParseBcd(std::uint32_t value) noexcept
	{
		return detail::ParseBcdNibbles(value, 8);
	}

	constexpr std::optional<std::uint8_t> ToBcd8(int value) noexcept
	{
		const auto result = detail::ToBcdNibbles(value, 2);
		const auto result_has_value = result.has_value();
		if (!result_has_value)
		{
			return std::nullopt;
		}

		return static_cast<std::uint8_t>(*result);
	}

	constexpr std::optional<std::uint16_t> ToBcd16(int value) noexcept
	{
		const auto result = detail::ToBcdNibbles(value, 4);
		const auto result_has_value = result.has_value();
		if (!result_has_value)
		{
			return std::nullopt;
		}

		return static_cast<std::uint16_t>(*result);
	}

	constexpr std::optional<std::uint32_t> ToBcd32(int value) noexcept
	{
		return detail::ToBcdNibbles(value, 8);
	}

} // namespace ket
