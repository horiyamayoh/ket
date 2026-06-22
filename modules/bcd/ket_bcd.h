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
 * 公開API：ket::bcd
 * 内部実装：ket::bcd::detail
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
	namespace bcd
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief packed BCD validation failure kind。
		 */
		enum class ValidationError : std::uint8_t
		{
			kNone,
			kInvalidStorage,
			kInvalidNibble
		};

		// NOLINTBEGIN(misc-non-private-member-variables-in-classes)

		/**
		 * @brief packed BCD validation result。
		 */
		struct ValidationResult
		{
			/** @brief validation failure kind。 */
			ValidationError error = ValidationError::kNone;

			/** @brief 不正nibbleを含むbyte offset。successとinvalid storageでは0。 */
			std::size_t byte_offset = 0U;

			/** @brief 不正nibble値。successとinvalid storageでは0。 */
			std::uint8_t actual = 0U;

			/** @brief 不正nibbleが上位nibbleならtrue、下位nibbleならfalse。 */
			bool high_nibble = false;

			/**
			 * @brief validation成功判定。
			 * @retval true `error == ValidationError::kNone`。
			 * @retval false validation failureを保持。
			 * @pre なし。
			 * @post `*this`と外部状態の変更なし。
			 * @code
			 * ket::bcd::ValidationResult result;
			 * const auto ok = result.Ok();
			 * // ok == true
			 * @endcode
			 */
			[[nodiscard]] constexpr bool Ok() const noexcept
			{
				return error == ValidationError::kNone;
			}
		};

		// NOLINTEND(misc-non-private-member-variables-in-classes)

		/**
		 * @brief 2桁固定幅パックBCDの10進整数変換。
		 * @param[in] value 変換対象のパックBCD値。
		 * @retval value 変換後の10進整数。
		 * @retval std::nullopt 不正nibble、またはint範囲外。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note 整数変換のため、先頭ゼロは保持なし。
		 * @code
		 * const auto value = ket::bcd::ToInt(static_cast<std::uint8_t>(0x42U));
		 * // value == std::optional<int>(42)
		 * @endcode
		 */
		constexpr std::optional<int> ToInt(std::uint8_t value) noexcept;

		/**
		 * @brief 4桁固定幅パックBCDの10進整数変換。
		 * @param[in] value 変換対象のパックBCD値。
		 * @retval value 変換後の10進整数。
		 * @retval std::nullopt 不正nibble、またはint範囲外。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note 整数変換のため、先頭ゼロは保持なし。
		 * @code
		 * const auto value = ket::bcd::ToInt(static_cast<std::uint16_t>(0x1234U));
		 * // value == std::optional<int>(1234)
		 * @endcode
		 */
		constexpr std::optional<int> ToInt(std::uint16_t value) noexcept;

		/**
		 * @brief 8桁固定幅パックBCDの10進整数変換。
		 * @param[in] value 変換対象のパックBCD値。
		 * @retval value 変換後の10進整数。
		 * @retval std::nullopt 不正nibble、またはint範囲外。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note 整数変換のため、先頭ゼロは保持なし。
		 * @code
		 * const auto value = ket::bcd::ToInt(static_cast<std::uint32_t>(0x20260613U));
		 * // value == std::optional<int>(20260613)
		 * @endcode
		 */
		constexpr std::optional<int> ToInt(std::uint32_t value) noexcept;

		/**
		 * @brief 固定幅パックBCDへの10進整数変換。
		 * @tparam Packed 変換後のパックBCD格納型。`std::uint8_t`、`std::uint16_t`、
		 * `std::uint32_t`のみ対応。
		 * @param[in] value 変換対象の10進整数。
		 * @retval value 変換後のパックBCD値。
		 * @retval std::nullopt 負数、または`Packed`の固定幅桁数を超える値。
		 * @pre なし。負数と桁数超過は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note `std::uint8_t`は2桁、`std::uint16_t`は4桁、`std::uint32_t`は8桁の
		 * fixed-width packed BCDとして表現。
		 * @code
		 * const auto value = ket::bcd::FromInt<std::uint8_t>(42);
		 * // value == std::optional<std::uint8_t>(0x42)
		 * @endcode
		 */
		template <typename Packed>
		constexpr std::optional<Packed> FromInt(int value) noexcept;

		/**
		 * @brief 2桁固定幅パックBCDの妥当性判定。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval true 全nibbleが0から9。
		 * @retval false 不正nibbleを含む。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto ok = ket::bcd::IsBcdByte(static_cast<std::uint8_t>(0x42U));
		 * // ok == true
		 * @endcode
		 */
		constexpr bool IsBcdByte(std::uint8_t value) noexcept;

		/**
		 * @brief 4桁固定幅パックBCDの妥当性判定。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval true 全nibbleが0から9。
		 * @retval false 不正nibbleを含む。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto ok = ket::bcd::IsBcd16(static_cast<std::uint16_t>(0x1234U));
		 * // ok == true
		 * @endcode
		 */
		constexpr bool IsBcd16(std::uint16_t value) noexcept;

		/**
		 * @brief 8桁固定幅パックBCDの妥当性判定。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval true 全nibbleが0から9。
		 * @retval false 不正nibbleを含む。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto ok = ket::bcd::IsBcd32(std::uint32_t{0x20260613U});
		 * // ok == true
		 * @endcode
		 */
		constexpr bool IsBcd32(std::uint32_t value) noexcept;

		/**
		 * @brief 2桁固定幅パックBCDの診断付き妥当性検査。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval value validation result。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note `byte_offset`は常に0。
		 * @code
		 * const auto result = ket::bcd::Validate(static_cast<std::uint8_t>(0x1AU));
		 * // result.Ok() == false, result.actual == 0x0A
		 * @endcode
		 */
		constexpr ValidationResult Validate(std::uint8_t value) noexcept;

		/**
		 * @brief 4桁固定幅パックBCDの診断付き妥当性検査。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval value validation result。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note `byte_offset`はpacked BCD値を上位byteから数えた位置。
		 * @code
		 * const auto result = ket::bcd::Validate(static_cast<std::uint16_t>(0x12A4U));
		 * // result.Ok() == false, result.actual == 0x0A
		 * @endcode
		 */
		constexpr ValidationResult Validate(std::uint16_t value) noexcept;

		/**
		 * @brief 8桁固定幅パックBCDの診断付き妥当性検査。
		 * @param[in] value 判定対象のパックBCD値。
		 * @retval value validation result。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note `byte_offset`はpacked BCD値を上位byteから数えた位置。
		 * @code
		 * const auto result = ket::bcd::Validate(std::uint32_t{0x20260A13U});
		 * // result.Ok() == false, result.actual == 0x0A
		 * @endcode
		 */
		constexpr ValidationResult Validate(std::uint32_t value) noexcept;

		/**
		 * @brief 任意byte列のpacked BCD診断付き妥当性検査。
		 * @param[in] data 判定対象byte列先頭。`size == 0`の場合だけnullptr可。
		 * @param[in] size 判定対象byte数。
		 * @retval value validation result。
		 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr + 非0`は
		 * invalid storageとして失敗値を返す。
		 * @post 引数と外部状態の変更なし。
		 * @note `byte_offset`は入力byte列先頭から数えた位置。
		 * @note empty byte sequenceはvalidなBCD列として扱う。文字列化する`Format`の空入力失敗とは
		 * 責務が異なる。
		 * @code
		 * const std::uint8_t data[] = {0x00U, 0x4AU};
		 * const auto result = ket::bcd::Validate(data, 2U);
		 * // result.Ok() == false, result.byte_offset == 1
		 * @endcode
		 */
		ValidationResult Validate(const std::uint8_t* data, std::size_t size) noexcept;

		/**
		 * @brief 固定幅packed BCDに収まる10進整数最大値取得。
		 * @tparam Packed `std::uint8_t`、`std::uint16_t`、`std::uint32_t`のいずれか。
		 * @retval value `Packed`のBCD桁数で表現可能な最大10進整数。
		 * @pre `Packed`は対応する固定幅packed BCD格納型。対象外型はcompile error。
		 * @post 外部状態の変更なし。
		 * @code
		 * const auto max_value = ket::bcd::MaxInt<std::uint8_t>();
		 * // max_value == 99
		 * @endcode
		 */
		template <typename Packed>
		constexpr int MaxInt() noexcept;

		/**
		 * @brief 任意バイト長パックBCDの10進文字列変換。
		 * @param[in] data 変換対象のパックBCD列。
		 * @param[in] size `data`のバイト数。
		 * @retval value 変換後の10進文字列。
		 * @retval std::nullopt `nullptr`、空入力、または不正nibble。
		 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr`
		 * と空入力は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note 入力BCDの桁数を保ち、先頭の0も文字'0'として出力。
		 * @note std::stringの確保があるためnoexceptなし。
		 * @note 任意長入力を線形走査し、出力文字列を確保する。untrusted
		 * inputではcaller側で最大byte数を 確認してから呼ぶ。
		 * @code
		 * const std::uint8_t data[] = {0x00U, 0x42U};
		 * const auto value = ket::bcd::Format(data, 2U);
		 * // value == std::optional<std::string>("0042")
		 * @endcode
		 */
		std::optional<std::string> Format(const std::uint8_t* data, std::size_t size);

		/**
		 * @brief 10進文字列の任意バイト長パックBCD変換。
		 * @param[in] text 変換対象の10進文字列。
		 * @retval value 変換後のパックBCD列。
		 * @retval std::nullopt 空入力、または10進数字以外を含む入力。
		 * @pre なし。空入力と10進数字以外は失敗値として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note 偶数桁は2桁ずつpacked BCDへ変換し、奇数桁は先頭に0を補って変換。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @note 任意長入力を線形走査し、出力vectorを確保する。untrusted
		 * inputではcaller側で最大文字数を 確認してから呼ぶ。
		 * @code
		 * const auto value = ket::bcd::Parse("123");
		 * // value == std::optional<std::vector<std::uint8_t>>({0x01, 0x23})
		 * @endcode
		 */
		std::optional<std::vector<std::uint8_t>> Parse(std::string_view text);

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
			constexpr std::optional<int> ParseBcdNibbles(std::uint32_t value,
														 int nibble_count) noexcept
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
			constexpr std::optional<std::uint32_t> ToBcdNibbles(int value,
																int nibble_count) noexcept
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

			/**
			 * @brief successful validation result生成。
			 * @retval value success result。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			[[nodiscard]] constexpr ValidationResult ValidResult() noexcept
			{
				return ValidationResult{};
			}

			/**
			 * @brief invalid storage result生成。
			 * @retval value invalid storage result。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			[[nodiscard]] constexpr ValidationResult InvalidStorageResult() noexcept
			{
				return ValidationResult{ValidationError::kInvalidStorage, 0U, 0U, false};
			}

			/**
			 * @brief invalid nibble result生成。
			 * @param[in] byte_offset invalid nibbleを含むbyte offset。
			 * @param[in] actual invalid nibble値。
			 * @param[in] high_nibble high nibbleでの失敗か。
			 * @retval value invalid nibble result。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			[[nodiscard]] constexpr ValidationResult InvalidNibbleResult(std::size_t byte_offset,
																		 std::uint8_t actual,
																		 bool high_nibble) noexcept
			{
				return ValidationResult{
					ValidationError::kInvalidNibble, byte_offset, actual, high_nibble};
			}

			/**
			 * @brief 1byte packed BCDのvalidation。
			 * @param[in] value validation対象。
			 * @param[in] byte_offset resultに載せるbyte offset。
			 * @retval value validation result。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			[[nodiscard]] constexpr ValidationResult ValidateByte(std::uint8_t value,
																  std::size_t byte_offset) noexcept
			{
				const auto high = static_cast<std::uint8_t>((value >> 4U) & 0x0FU);
				const auto high_is_valid = IsBcdNibble(high);
				if (!high_is_valid)
				{
					return InvalidNibbleResult(byte_offset, high, true);
				}

				const auto low = static_cast<std::uint8_t>(value & 0x0FU);
				const auto low_is_valid = IsBcdNibble(low);
				if (!low_is_valid)
				{
					return InvalidNibbleResult(byte_offset, low, false);
				}

				return ValidResult();
			}

			/**
			 * @brief fixed-width packed BCDのvalidation。
			 * @param[in] value validation対象。
			 * @param[in] byte_count validationするbyte数。
			 * @retval value validation result。
			 * @pre `byte_count`は1以上4以下。
			 * @post 外部状態の変更なし。
			 */
			[[nodiscard]] constexpr ValidationResult ValidatePacked(std::uint32_t value,
																	int byte_count) noexcept
			{
				for (int index = byte_count - 1; index >= 0; --index)
				{
					const auto shift = index * 8;
					const auto byte = static_cast<std::uint8_t>((value >> shift) & 0xFFU);
					const auto byte_offset = static_cast<std::size_t>(byte_count - index - 1);
					const auto result = ValidateByte(byte, byte_offset);
					const auto result_ok = result.Ok();
					if (!result_ok)
					{
						return result;
					}
				}

				return ValidResult();
			}

			/**
			 * @brief 固定幅パックBCD出力型のtraits。
			 * @tparam Packed 判定対象の固定幅パックBCD格納型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Packed>
			struct PackedBcdTraits
			{
				static constexpr bool kSupported = false;
				static constexpr int kNibbleCount = 0;
			};

			template <>
			/**
			 * @brief 2桁固定幅パックBCD出力型のtraits。
			 * @note detail配下の型は公開APIではない。
			 */
			struct PackedBcdTraits<std::uint8_t>
			{
				static constexpr bool kSupported = true;
				static constexpr int kNibbleCount = 2;
			};

			template <>
			/**
			 * @brief 4桁固定幅パックBCD出力型のtraits。
			 * @note detail配下の型は公開APIではない。
			 */
			struct PackedBcdTraits<std::uint16_t>
			{
				static constexpr bool kSupported = true;
				static constexpr int kNibbleCount = 4;
			};

			template <>
			/**
			 * @brief 8桁固定幅パックBCD出力型のtraits。
			 * @note detail配下の型は公開APIではない。
			 */
			struct PackedBcdTraits<std::uint32_t>
			{
				static constexpr bool kSupported = true;
				static constexpr int kNibbleCount = 8;
			};

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr std::optional<int> ToInt(std::uint8_t value) noexcept
		{
			return detail::ParseBcdNibbles(value, 2);
		}

		constexpr std::optional<int> ToInt(std::uint16_t value) noexcept
		{
			return detail::ParseBcdNibbles(value, 4);
		}

		constexpr std::optional<int> ToInt(std::uint32_t value) noexcept
		{
			return detail::ParseBcdNibbles(value, 8);
		}

		template <typename Packed>
		constexpr std::optional<Packed> FromInt(int value) noexcept
		{
			static_assert(detail::PackedBcdTraits<Packed>::kSupported,
						  "ket::bcd::FromInt supports std::uint8_t, std::uint16_t, and "
						  "std::uint32_t only.");

			const auto result =
				detail::ToBcdNibbles(value, detail::PackedBcdTraits<Packed>::kNibbleCount);
			const auto result_has_value = result.has_value();
			if (!result_has_value)
			{
				return std::nullopt;
			}

			return static_cast<Packed>(*result);
		}

		constexpr bool IsBcdByte(std::uint8_t value) noexcept
		{
			return Validate(value).Ok();
		}

		constexpr bool IsBcd16(std::uint16_t value) noexcept
		{
			return Validate(value).Ok();
		}

		constexpr bool IsBcd32(std::uint32_t value) noexcept
		{
			return Validate(value).Ok();
		}

		constexpr ValidationResult Validate(std::uint8_t value) noexcept
		{
			return detail::ValidateByte(value, 0U);
		}

		constexpr ValidationResult Validate(std::uint16_t value) noexcept
		{
			return detail::ValidatePacked(value, 2);
		}

		constexpr ValidationResult Validate(std::uint32_t value) noexcept
		{
			return detail::ValidatePacked(value, 4);
		}

		template <typename Packed>
		constexpr int MaxInt() noexcept
		{
			static_assert(detail::PackedBcdTraits<Packed>::kSupported,
						  "ket::bcd::MaxInt supports std::uint8_t, std::uint16_t, and "
						  "std::uint32_t only.");
			return detail::DecimalLimitForBcdNibbles(
					   detail::PackedBcdTraits<Packed>::kNibbleCount) -
				1;
		}

	} // namespace bcd

} // namespace ket
