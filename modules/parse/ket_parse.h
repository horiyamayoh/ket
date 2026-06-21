#pragma once

/**
 * @file ket_parse.h
 * @brief 文字列から基本値への小さいparse API。
 *
 * @details `std::from_chars` 周辺の完全消費、空白拒否、overflow失敗を固定し、
 * 数値parseとbool parseを短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時は
 * ヘッダ単体で持ち出す。locale、trim、浮動小数点parseは扱わない。
 *
 * @par プロジェクトへの適用方法
 * `ket_parse.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::from_chars`を利用でき、完全消費やoverflow失敗の方針を標準ライブラリのみで固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::parse
 * 内部実装：ket::parse::detail
 */

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace ket
{
	namespace parse
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 10進文字列の符号付き整数変換。
		 * @tparam T 変換後のcv修飾なし符号付き整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @param[out] out 変換後の値。成功時だけ更新。
		 * @retval true `text`を完全消費し、`T`の範囲内で変換成功。
		 * @retval false 空文字列、空白、部分消費、不正文字、先頭の`+`、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 成功時は`out`を変換結果へ更新。失敗時は`out`を入力時の値に保持。
		 * @code
		 * int value = 0;
		 * const auto ok = ket::parse::TryInt("-42", value);
		 * // ok == true, value == -42
		 * @endcode
		 */
		template <typename T>
		bool TryInt(std::string_view text, T& out) noexcept;

		/**
		 * @brief 10進文字列の符号なし整数変換。
		 * @tparam T 変換後のcv修飾なし符号なし整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @param[out] out 変換後の値。成功時だけ更新。
		 * @retval true `text`を完全消費し、`T`の範囲内で変換成功。
		 * @retval false 空文字列、空白、部分消費、不正文字、符号文字、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 成功時は`out`を変換結果へ更新。失敗時は`out`を入力時の値に保持。
		 * @code
		 * std::uint32_t value = 0;
		 * const auto ok = ket::parse::TryUInt("42", value);
		 * // ok == true, value == 42
		 * @endcode
		 */
		template <typename T>
		bool TryUInt(std::string_view text, T& out) noexcept;

		/**
		 * @brief 16進文字列の整数変換。
		 * @tparam T 変換後のcv修飾なし整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の16進文字列。`0x`または`0X` prefixを許可。
		 * @param[out] out 変換後の値。成功時だけ更新。
		 * @retval true prefix除去後の文字列を完全消費し、`T`の範囲内で変換成功。
		 * @retval false 空文字列、prefixのみ、空白、符号文字、部分消費、不正hex、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 成功時は`out`を変換結果へ更新。失敗時は`out`を入力時の値に保持。
		 * @note 16進表記は符号文字を扱わない。signed `T`でも非負のhex桁列だけを変換対象とする。
		 * @code
		 * std::uint32_t value = 0;
		 * const auto ok = ket::parse::TryHex("0xff", value);
		 * // ok == true, value == 255
		 * @endcode
		 */
		template <typename T>
		bool TryHex(std::string_view text, T& out) noexcept;

		/**
		 * @brief bool文字列の変換。
		 * @param[in] text 変換対象の文字列。
		 * @param[out] out 変換後の値。成功時だけ更新。
		 * @retval true `true`、`false`、`1`、`0`のいずれかと完全一致。
		 * @retval false 空文字列、空白、大文字小文字違い、または未採用のbool表現。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 成功時は`out`を変換結果へ更新。失敗時は`out`を入力時の値に保持。
		 * @note bool文字列はcase-sensitive。`yes`、`no`、`on`、`off`は扱わない。
		 * @code
		 * bool value = false;
		 * const auto ok = ket::parse::TryBool("true", value);
		 * // ok == true, value == true
		 * @endcode
		 */
		constexpr bool TryBool(std::string_view text, bool& out) noexcept;

		/**
		 * @brief 10進文字列の符号付き整数変換。
		 * @tparam T 変換後のcv修飾なし符号付き整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @retval value 変換後の値。
		 * @retval std::nullopt 空文字列、空白、部分消費、不正文字、先頭の`+`、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::parse::Int<int>("-42");
		 * // value == std::optional<int>(-42)
		 * @endcode
		 */
		template <typename T>
		std::optional<T> Int(std::string_view text) noexcept;

		/**
		 * @brief 10進文字列の符号なし整数変換。
		 * @tparam T 変換後のcv修飾なし符号なし整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @retval value 変換後の値。
		 * @retval std::nullopt 空文字列、空白、部分消費、不正文字、符号文字、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::parse::UInt<unsigned>("42");
		 * // value == std::optional<unsigned>(42U)
		 * @endcode
		 */
		template <typename T>
		std::optional<T> UInt(std::string_view text) noexcept;

		/**
		 * @brief 16進文字列の整数変換。
		 * @tparam T 変換後のcv修飾なし整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の16進文字列。`0x`または`0X` prefixを許可。
		 * @retval value 変換後の値。
		 * @retval std::nullopt
		 * 空文字列、prefixのみ、空白、符号文字、部分消費、不正hex、または範囲外。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note 16進表記は符号文字を扱わない。signed `T`でも非負のhex桁列だけを変換対象とする。
		 * @code
		 * const auto value = ket::parse::Hex<unsigned>("0xff");
		 * // value == std::optional<unsigned>(255U)
		 * @endcode
		 */
		template <typename T>
		std::optional<T> Hex(std::string_view text) noexcept;

		/**
		 * @brief bool文字列の変換。
		 * @param[in] text 変換対象の文字列。
		 * @retval value 変換後の値。
		 * @retval std::nullopt `true`、`false`、`1`、`0`のいずれにも完全一致しない入力。
		 * @pre なし。失敗条件は戻り値で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note bool文字列はcase-sensitive。`yes`、`no`、`on`、`off`は扱わない。
		 * @code
		 * const auto value = ket::parse::Bool("false");
		 * // value == std::optional<bool>(false)
		 * @endcode
		 */
		constexpr std::optional<bool> Bool(std::string_view text) noexcept;

		/**
		 * @brief 10進文字列の符号付き整数変換とfallback返却。
		 * @tparam T 変換後のcv修飾なし符号付き整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @param[in] fallback 変換失敗時に返す値。
		 * @retval value 変換成功時は変換後の値、失敗時は`fallback`。
		 * @pre なし。失敗条件はfallback返却で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::parse::IntOr<int>("bad", -1);
		 * // value == -1
		 * @endcode
		 */
		template <typename T>
		T IntOr(std::string_view text, T fallback) noexcept;

		/**
		 * @brief 10進文字列の符号なし整数変換とfallback返却。
		 * @tparam T 変換後のcv修飾なし符号なし整数型。boolとplain character型は対象外。
		 * @param[in] text 変換対象の文字列。
		 * @param[in] fallback 変換失敗時に返す値。
		 * @retval value 変換成功時は変換後の値、失敗時は`fallback`。
		 * @pre なし。失敗条件はfallback返却で扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::parse::UIntOr<unsigned>("bad", 7U);
		 * // value == 7U
		 * @endcode
		 */
		template <typename T>
		T UIntOr(std::string_view text, T fallback) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief plain character型の判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の値は公開APIではない。
			 */
			template <typename T>
			inline constexpr bool kIsPlainCharacter =
				std::is_same_v<T, char> || std::is_same_v<T, wchar_t> ||
				std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

			/**
			 * @brief parse template API が受け付ける整数型の判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の値は公開APIではない。
			 */
			template <typename T>
			inline constexpr bool kIsIntegral =
				std::is_integral_v<T> && std::is_same_v<T, std::remove_cv_t<T>> &&
				!std::is_same_v<T, bool> && !kIsPlainCharacter<T>;

			/**
			 * @brief boolとplain character型を除く符号付き整数型の判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の値は公開APIではない。
			 */
			template <typename T>
			inline constexpr bool kIsSignedIntegral = kIsIntegral<T> && std::is_signed_v<T>;

			/**
			 * @brief boolとplain character型を除く符号なし整数型の判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の値は公開APIではない。
			 */
			template <typename T>
			inline constexpr bool kIsUnsignedIntegral = kIsIntegral<T> && std::is_unsigned_v<T>;

			/**
			 * @brief from_chars結果の成功判定。
			 * @param[in] result 判定対象のfrom_chars結果。
			 * @param[in] last 期待する消費終端。
			 * @retval true エラーなし、かつ入力を完全消費。
			 * @retval false from_chars失敗、または部分消費。
			 * @pre `last`は`result.ptr`と同じ入力範囲の終端。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool IsCompleteParse(std::from_chars_result result, const char* last) noexcept
			{
				const auto has_no_error = result.ec == std::errc();
				const auto consumed_all = result.ptr == last;

				return has_no_error && consumed_all;
			}

			/**
			 * @brief `0x`または`0X` prefix除去。
			 * @param[in] text prefix確認対象の文字列。
			 * @retval value prefix除去後の文字列。
			 * @pre なし。空文字列や1文字入力はそのまま返す。
			 * @post 引数と外部状態の変更なし。
			 */
			inline std::string_view StripHexPrefix(std::string_view text) noexcept
			{
				const auto has_prefix =
					text.size() >= 2U && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
				if (!has_prefix)
				{
					return text;
				}

				text.remove_prefix(2U);
				return text;
			}

			/**
			 * @brief 先頭の符号文字判定。
			 * @param[in] text 判定対象の文字列。
			 * @retval true 先頭文字が`+`または`-`。
			 * @retval false 空文字列、または先頭文字が符号文字ではない。
			 * @pre なし。空文字列はfalseとして扱う。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr bool HasLeadingSign(std::string_view text) noexcept
			{
				const auto text_is_empty = text.empty();
				if (text_is_empty)
				{
					return false;
				}

				return text[0] == '+' || text[0] == '-';
			}

			/**
			 * @brief 整数文字列のfrom_chars変換。
			 * @tparam T 変換後の整数型。
			 * @tparam Base 変換時の基数。
			 * @param[in] text 変換対象の文字列。
			 * @param[out] out 変換後の値。成功時だけ更新。
			 * @retval true `text`を完全消費し、`T`の範囲内で変換成功。
			 * @retval false 空文字列、部分消費、不正文字、または範囲外。
			 * @pre `T`はcv修飾なしでboolとplain
			 * character型を除く整数型。`Base`は`std::from_chars`が受け付ける基数。
			 * @post 成功時は`out`を変換結果へ更新。失敗時は`out`を入力時の値に保持。
			 */
			template <typename T, int Base>
			bool TryInteger(std::string_view text, T& out) noexcept
			{
				static_assert(kIsIntegral<T>,
							  "ket::parse integer APIs require an integral T except bool and plain "
							  "character types.");

				const auto text_is_empty = text.empty();
				if (text_is_empty)
				{
					return false;
				}

				T value{};
				const char* const first = text.data();
				const char* const last = first + text.size();
				const auto result = std::from_chars(first, last, value, Base);
				const auto parsed_completely = IsCompleteParse(result, last);
				if (!parsed_completely)
				{
					return false;
				}

				out = value;
				return true;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		bool TryInt(std::string_view text, T& out) noexcept
		{
			static_assert(detail::kIsSignedIntegral<T>,
						  "ket::parse::TryInt requires a signed integral T except bool and plain "
						  "character types.");

			return detail::TryInteger<T, 10>(text, out);
		}

		template <typename T>
		bool TryUInt(std::string_view text, T& out) noexcept
		{
			static_assert(
				detail::kIsUnsignedIntegral<T>,
				"ket::parse::TryUInt requires an unsigned integral T except bool and plain "
				"character types.");

			return detail::TryInteger<T, 10>(text, out);
		}

		template <typename T>
		bool TryHex(std::string_view text, T& out) noexcept
		{
			static_assert(detail::kIsIntegral<T>,
						  "ket::parse::TryHex requires an integral T except bool and plain "
						  "character types.");

			const auto text_has_sign = detail::HasLeadingSign(text);
			if (text_has_sign)
			{
				return false;
			}

			const auto body = detail::StripHexPrefix(text);
			const auto body_has_sign = detail::HasLeadingSign(body);
			if (body_has_sign)
			{
				return false;
			}

			return detail::TryInteger<T, 16>(body, out);
		}

		constexpr bool TryBool(std::string_view text, bool& out) noexcept
		{
			if (text == "true" || text == "1")
			{
				out = true;
				return true;
			}

			if (text == "false" || text == "0")
			{
				out = false;
				return true;
			}

			return false;
		}

		template <typename T>
		std::optional<T> Int(std::string_view text) noexcept
		{
			T value{};
			const auto parsed = TryInt(text, value);
			if (!parsed)
			{
				return std::nullopt;
			}

			return value;
		}

		template <typename T>
		std::optional<T> UInt(std::string_view text) noexcept
		{
			T value{};
			const auto parsed = TryUInt(text, value);
			if (!parsed)
			{
				return std::nullopt;
			}

			return value;
		}

		template <typename T>
		std::optional<T> Hex(std::string_view text) noexcept
		{
			T value{};
			const auto parsed = TryHex(text, value);
			if (!parsed)
			{
				return std::nullopt;
			}

			return value;
		}

		constexpr std::optional<bool> Bool(std::string_view text) noexcept
		{
			bool value = false;
			const auto parsed = TryBool(text, value);
			if (!parsed)
			{
				return std::nullopt;
			}

			return value;
		}

		template <typename T>
		T IntOr(std::string_view text, T fallback) noexcept
		{
			T value{};
			const auto parsed = TryInt(text, value);
			if (!parsed)
			{
				return fallback;
			}

			return value;
		}

		template <typename T>
		T UIntOr(std::string_view text, T fallback) noexcept
		{
			T value{};
			const auto parsed = TryUInt(text, value);
			if (!parsed)
			{
				return fallback;
			}

			return value;
		}

	} // namespace parse

} // namespace ket
