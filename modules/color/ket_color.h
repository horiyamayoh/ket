#pragma once

/**
 * @file ket_color.h
 * @brief RGB小値型と6桁hex表現の変換API。
 *
 * @details RGBの3 byte値と6桁hex文字列の変換を、小さいdrop-in APIとして固定する。
 * 先頭`#`は入力で任意、出力ではFormatOptionsで制御する。alpha、CSS color name、
 * color space変換は扱わない。
 *
 * @par プロジェクトへの適用方法
 * `ket_color.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：RGB小値型の許容表記と不正hexを小さいAPIで固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::color
 * 内部実装：ket::color::detail
 */

#include <cstddef>
#include <cstdint>
#include <string>

namespace ket
{
	namespace color
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief RGBの8 bit channel値。
		 */
		struct Rgb
		{
			std::uint8_t r = 0;
			std::uint8_t g = 0;
			std::uint8_t b = 0;
		};

		/**
		 * @brief RGB hex文字列化の出力設定。
		 */
		struct FormatOptions
		{
			bool with_hash = true;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief ASCII hex digitの数値変換。
			 * @param[in] value 変換対象の文字。
			 * @param[out] out 成功時に0から15のhex digit値を格納。
			 * @retval true `value`がASCII hex digit。
			 * @retval false `value`がASCII hex digitではない。
			 * @pre なし。任意のchar値を判定対象として扱う。
			 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool TryGetHexDigit(char value, std::uint8_t& out) noexcept
			{
				const auto value_is_decimal = value >= '0' && value <= '9';
				if (value_is_decimal)
				{
					out = static_cast<std::uint8_t>(value - '0');
					return true;
				}

				const auto value_is_lower_hex = value >= 'a' && value <= 'f';
				if (value_is_lower_hex)
				{
					out = static_cast<std::uint8_t>((value - 'a') + 10);
					return true;
				}

				const auto value_is_upper_hex = value >= 'A' && value <= 'F';
				if (value_is_upper_hex)
				{
					out = static_cast<std::uint8_t>((value - 'A') + 10);
					return true;
				}

				return false;
			}

			/**
			 * @brief 2桁ASCII hexのbyte変換。
			 * @param[in] text 変換対象の文字列。
			 * @param[in] offset 2桁hexの開始位置。
			 * @param[out] out 成功時に変換後のbyte値を格納。
			 * @retval true `offset`から2文字がASCII hex digit。
			 * @retval false `offset`から2文字のいずれかがASCII hex digitではない。
			 * @pre `offset + 1`は`text`の有効なindex。
			 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool
			TryParseByte(const std::string& text, std::size_t offset, std::uint8_t& out) noexcept
			{
				std::uint8_t high = 0;
				std::uint8_t low = 0;

				const auto high_parsed = TryGetHexDigit(text[offset], high);
				if (!high_parsed)
				{
					return false;
				}

				const auto low_parsed = TryGetHexDigit(text[offset + 1U], low);
				if (!low_parsed)
				{
					return false;
				}

				const auto packed =
					(static_cast<unsigned int>(high) << 4U) | static_cast<unsigned int>(low);
				out = static_cast<std::uint8_t>(packed);
				return true;
			}

			/**
			 * @brief byte値の上位nibble取得。
			 * @param[in] value 対象byte値。
			 * @retval value `value`の上位4 bit。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept
			{
				return static_cast<std::uint8_t>((static_cast<unsigned int>(value) >> 4U) & 0x0FU);
			}

			/**
			 * @brief byte値の下位nibble取得。
			 * @param[in] value 対象byte値。
			 * @retval value `value`の下位4 bit。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept
			{
				return static_cast<std::uint8_t>(static_cast<unsigned int>(value) & 0x0FU);
			}

			/**
			 * @brief nibble値のlower-case hex文字変換。
			 * @param[in] value 0から15のnibble値。
			 * @retval value `value`に対応するlower-case hex文字。
			 * @pre `value <= 15`。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr char HexDigit(std::uint8_t value) noexcept
			{
				return static_cast<char>(
					(static_cast<unsigned int>(value) < 10U)
						? (static_cast<int>('0') + static_cast<int>(value))
						: (static_cast<int>('a') + (static_cast<int>(value) - 10)));
			}

			/**
			 * @brief byte値のlower-case 2桁hex追記。
			 * @param[in,out] destination 追記先の文字列。
			 * @param[in] value 追記対象のbyte値。
			 * @retval void 戻り値なし。
			 * @pre `destination`は有効なstd::string。
			 * @post `destination`の末尾に`value`の2桁lower-case hex表現を追加。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline void AppendHexByte(std::string& destination, std::uint8_t value)
			{
				destination.push_back(HexDigit(HighNibble(value)));
				destination.push_back(HexDigit(LowNibble(value)));
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		/**
		 * @brief RGBの6桁hex文字列解析。
		 * @param[in] text 解析対象の文字列。`RRGGBB`、または`#RRGGBB`を受け付ける。
		 * @param[out] out 成功時に解析後のRGB値を格納。
		 * @retval true 6桁RGB hexとして解析成功。
		 * @retval false 不正長、不正hex、3桁短縮形、またはalpha付き入力。
		 * @pre なし。任意のstd::stringを入力として扱い、不正入力はfalse。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note hex digitの大文字小文字混在は許容。CSS color nameとalphaは対象外。
		 * @code
		 * ket::color::Rgb color;
		 * const auto parsed = ket::color::TryParse("#0aFF10", color);
		 * // parsed == true, color == {0x0a, 0xff, 0x10}
		 * @endcode
		 */
		inline bool TryParse(const std::string& text, Rgb& out) noexcept
		{
			const auto text_size = text.size();
			const auto has_hash = text_size > 0U && text[0] == '#';
			const auto expected_size = has_hash ? 7U : 6U;
			const auto size_matches = text_size == expected_size;
			if (!size_matches)
			{
				return false;
			}

			const auto first_digit = has_hash ? 1U : 0U;
			Rgb result;

			const auto red_parsed = detail::TryParseByte(text, first_digit, result.r);
			if (!red_parsed)
			{
				return false;
			}

			const auto green_parsed = detail::TryParseByte(text, first_digit + 2U, result.g);
			if (!green_parsed)
			{
				return false;
			}

			const auto blue_parsed = detail::TryParseByte(text, first_digit + 4U, result.b);
			if (!blue_parsed)
			{
				return false;
			}

			out = result;
			return true;
		}

		/**
		 * @brief RGB値の6桁lower-case hex文字列化。
		 * @param[in] color 文字列化対象のRGB値。
		 * @param[in] options 出力形式の設定。既定では先頭`#`を付ける。
		 * @retval value `color`を表す6桁lower-case hex文字列。
		 * @pre なし。各channelは0から255のbyte値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @code
		 * ket::color::Rgb color;
		 * color.r = 0x0aU;
		 * color.g = 0xffU;
		 * color.b = 0x10U;
		 * const auto text = ket::color::Format(color);
		 * // text == "#0aff10"
		 * @endcode
		 */
		inline std::string Format(Rgb color, FormatOptions options = FormatOptions())
		{
			std::string result;

			const auto output_size = options.with_hash ? 7U : 6U;
			result.reserve(output_size);

			if (options.with_hash)
			{
				result.push_back('#');
			}

			detail::AppendHexByte(result, color.r);
			detail::AppendHexByte(result, color.g);
			detail::AppendHexByte(result, color.b);

			return result;
		}

	} // namespace color

} // namespace ket
