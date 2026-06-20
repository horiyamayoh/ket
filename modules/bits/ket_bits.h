#pragma once

/**
 * @file ket_bits.h
 * @brief bit、nibble、mask、rotate の小処理API。
 *
 * @details unsigned integral の小さいbit処理を、範囲外shiftや符号付き演算を避ける
 * 名前付きAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_bits.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：unsigned integral の小さいbit処理を標準ライブラリだけで安全に名前付け可能。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。C++20 `<bit>` と一部重なるが、nibble、mask失敗値、
 * bit index境界処理の直接代替なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::bits
 * 内部実装：ket::bits::detail
 */

#include <cstdint>
#include <limits>
#include <type_traits>

namespace ket
{
	namespace bits
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 4bit nibble 範囲の判定。
		 * @param[in] value 判定対象の値。
		 * @retval true `value`が0x00から0x0F。
		 * @retval false `value`がnibble範囲外。
		 * @pre なし。任意のstd::uint8_t値を判定対象として扱う。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto ok = ket::bits::IsNibble(static_cast<std::uint8_t>(0x0FU));
		 * const auto too_large = ket::bits::IsNibble(static_cast<std::uint8_t>(0x10U));
		 * // ok == true
		 * // too_large == false
		 * @endcode
		 */
		constexpr bool IsNibble(std::uint8_t value) noexcept;

		/**
		 * @brief 上位nibbleの取得。
		 * @param[in] value 取得対象のbyte値。
		 * @retval value `value`のbit 7..4を0..15へ右詰めした値。
		 * @pre なし。任意のstd::uint8_t値を入力可能。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto high = ket::bits::HighNibble(static_cast<std::uint8_t>(0xABU));
		 * // high == 0x0A
		 * @endcode
		 */
		constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept;

		/**
		 * @brief 下位nibbleの取得。
		 * @param[in] value 取得対象のbyte値。
		 * @retval value `value`のbit 3..0を0..15として取り出した値。
		 * @pre なし。任意のstd::uint8_t値を入力可能。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto low = ket::bits::LowNibble(static_cast<std::uint8_t>(0xABU));
		 * // low == 0x0B
		 * @endcode
		 */
		constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept;

		/**
		 * @brief 2つのnibbleからbyte値を生成。
		 * @param[in] high 上位nibbleとして格納する値。
		 * @param[in] low 下位nibbleとして格納する値。
		 * @param[out] out 成功時の生成byte値。失敗時は変更なし。
		 * @retval true `high`と`low`がnibble範囲内で、`out`を更新。
		 * @retval false `high`または`low`がnibble範囲外。`out`は入力時の値を保持。
		 * @pre `out`は有効なstd::uint8_t参照。
		 * @post 成功時だけ`out`を`(high << 4) | low`へ更新。失敗時は引数と外部状態の変更なし。
		 * @code
		 * std::uint8_t value = 0x00U;
		 * const auto ok = ket::bits::TryPackNibbles(0x0AU, 0x0BU, value);
		 * // ok == true
		 * // value == 0xAB
		 * @endcode
		 */
		inline bool TryPackNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept;

		/**
		 * @brief unsigned integral 型のbit幅取得。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @retval value `T`の値bit数。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 外部状態の変更なし。
		 * @code
		 * const auto width = ket::bits::TypeBitWidth<std::uint8_t>();
		 * // width == 8
		 * @endcode
		 */
		template <typename T>
		constexpr unsigned TypeBitWidth() noexcept;

		/**
		 * @brief 指定bitが立っているか判定。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value 判定対象の値。
		 * @param[in] bit_index 判定する0始まりbit index。
		 * @retval true `bit_index`が範囲内で、指定bitが1。
		 * @retval false `bit_index`が範囲外、または指定bitが0。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto high_set = ket::bits::HasBit<std::uint8_t>(0x80U, 7U);
		 * const auto out_of_range = ket::bits::HasBit<std::uint8_t>(0x80U, 8U);
		 * // high_set == true
		 * // out_of_range == false
		 * @endcode
		 */
		template <typename T>
		constexpr bool HasBit(T value, unsigned bit_index) noexcept;

		/**
		 * @brief 指定bitを立てた値の生成。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value 入力値。
		 * @param[in] bit_index 立てる0始まりbit index。
		 * @param[out] out 成功時の生成値。失敗時は変更なし。
		 * @retval true `bit_index`が範囲内で、`out`を更新。
		 * @retval false `bit_index`が範囲外。`out`は入力時の値を保持。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 成功時だけ`out`を`value`の指定bitが1の値へ更新。失敗時は引数と外部状態の変更なし。
		 * @code
		 * std::uint8_t value = 0x00U;
		 * const auto ok = ket::bits::TrySetBit<std::uint8_t>(value, 7U, value);
		 * // ok == true
		 * // value == 0x80
		 * @endcode
		 */
		template <typename T>
		bool TrySetBit(T value, unsigned bit_index, T& out) noexcept;

		/**
		 * @brief 指定bitを落とした値の生成。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value 入力値。
		 * @param[in] bit_index 落とす0始まりbit index。
		 * @param[out] out 成功時の生成値。失敗時は変更なし。
		 * @retval true `bit_index`が範囲内で、`out`を更新。
		 * @retval false `bit_index`が範囲外。`out`は入力時の値を保持。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 成功時だけ`out`を`value`の指定bitが0の値へ更新。失敗時は引数と外部状態の変更なし。
		 * @code
		 * std::uint8_t value = 0xFFU;
		 * const auto ok = ket::bits::TryClearBit<std::uint8_t>(value, 7U, value);
		 * // ok == true
		 * // value == 0x7F
		 * @endcode
		 */
		template <typename T>
		bool TryClearBit(T value, unsigned bit_index, T& out) noexcept;

		/**
		 * @brief 指定bitを反転した値の生成。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value 入力値。
		 * @param[in] bit_index 反転する0始まりbit index。
		 * @param[out] out 成功時の生成値。失敗時は変更なし。
		 * @retval true `bit_index`が範囲内で、`out`を更新。
		 * @retval false `bit_index`が範囲外。`out`は入力時の値を保持。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post
		 * 成功時だけ`out`を`value`の指定bitが反転した値へ更新。失敗時は引数と外部状態の変更なし。
		 * @code
		 * std::uint8_t value = 0x80U;
		 * const auto ok = ket::bits::TryToggleBit<std::uint8_t>(value, 7U, value);
		 * // ok == true
		 * // value == 0x00
		 * @endcode
		 */
		template <typename T>
		bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept;

		/**
		 * @brief 下位`width` bitが1のmask生成。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] width 1にする下位bit数。
		 * @param[out] out 成功時のmask値。失敗時は変更なし。
		 * @retval true `width`が`TypeBitWidth<T>()`以下で、`out`を更新。
		 * @retval false `width`が`TypeBitWidth<T>()`を超過。`out`は入力時の値を保持。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 成功時だけ`out`をmask値へ更新。`width == 0`は0、`width == TypeBitWidth<T>()`は全bit
		 * 1。
		 * @code
		 * std::uint8_t mask = 0xEEU;
		 * const auto ok = ket::bits::TryMask<std::uint8_t>(4U, mask);
		 * // ok == true
		 * // mask == 0x0F
		 * @endcode
		 */
		template <typename T>
		bool TryMask(unsigned width, T& out) noexcept;

		/**
		 * @brief 立っているbit数の取得。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value bit数を数える値。
		 * @retval value `value`内で1のbit数。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto count = ket::bits::PopCount<std::uint8_t>(0x81U);
		 * // count == 2
		 * @endcode
		 */
		template <typename T>
		constexpr unsigned PopCount(T value) noexcept;

		/**
		 * @brief 2の累乗値の判定。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value 判定対象の値。
		 * @retval true `value`が1bitだけ立つ値。
		 * @retval false `value`が0、または複数bitが立つ値。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto single = ket::bits::IsPowerOfTwo<std::uint8_t>(0x80U);
		 * const auto mixed = ket::bits::IsPowerOfTwo<std::uint8_t>(0x81U);
		 * // single == true
		 * // mixed == false
		 * @endcode
		 */
		template <typename T>
		constexpr bool IsPowerOfTwo(T value) noexcept;

		/**
		 * @brief 左rotateした値の取得。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value rotate対象の値。
		 * @param[in] count 左rotateするbit数。bit幅で剰余化。
		 * @retval value `value`を左rotateした値。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::bits::Rotl<std::uint8_t>(0x81U, 1U);
		 * // value == 0x03
		 * @endcode
		 */
		template <typename T>
		constexpr T Rotl(T value, unsigned count) noexcept;

		/**
		 * @brief 右rotateした値の取得。
		 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型のみ対応。
		 * @param[in] value rotate対象の値。
		 * @param[in] count 右rotateするbit数。bit幅で剰余化。
		 * @retval value `value`を右rotateした値。
		 * @pre `T`はbool、char、wchar_t、char16_t、char32_tを除くunsigned
		 * integral型。未対応型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::bits::Rotr<std::uint8_t>(0x81U, 1U);
		 * // value == 0xC0
		 * @endcode
		 */
		template <typename T>
		constexpr T Rotr(T value, unsigned count) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief bits template API が受け付ける型の判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsSupportedUnsignedIntegral
			{
				using ValueType = typename std::remove_cv<T>::type; // NOLINT(modernize-type-traits)

				static const bool kIsPlainCharacter = std::is_same<ValueType, char>() ||
					std::is_same<ValueType, wchar_t>() || std::is_same<ValueType, char16_t>() ||
					std::is_same<ValueType, char32_t>();
				static const bool kValue = std::is_integral<ValueType>() &&
					std::is_unsigned<ValueType>() && !std::is_same<ValueType, bool>() &&
					!kIsPlainCharacter;
			};

			/**
			 * @brief 1bitだけ立つ値の生成。
			 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
			 * integral型のみ対応。
			 * @param[in] bit_index 立てる0始まりbit index。
			 * @retval value 指定bitだけが1の値。
			 * @pre `bit_index < TypeBitWidth<T>()`。未対応型はcompile error。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			constexpr T SingleBit(unsigned bit_index) noexcept
			{
				static_assert(IsSupportedUnsignedIntegral<T>::kValue,
							  "ket::bits requires an unsigned integral type except bool and plain "
							  "character types.");

				return static_cast<T>(T(1) << bit_index);
			}

			/**
			 * @brief 正規化済みcountによる左rotate。
			 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
			 * integral型のみ対応。
			 * @param[in] value rotate対象の値。
			 * @param[in] count `TypeBitWidth<T>()`未満へ正規化済みのrotate数。
			 * @retval value 左rotate後の値。
			 * @pre `count < TypeBitWidth<T>()`。未対応型はcompile error。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			constexpr T RotlNormalized(T value, unsigned count) noexcept
			{
				static_assert(IsSupportedUnsignedIntegral<T>::kValue,
							  "ket::bits requires an unsigned integral type except bool and plain "
							  "character types.");

				return count == 0U
					? value
					: static_cast<T>(static_cast<T>(value << count) |
									 static_cast<T>(value >> (TypeBitWidth<T>() - count)));
			}

			/**
			 * @brief 正規化済みcountによる右rotate。
			 * @tparam T 対象型。bool、char、wchar_t、char16_t、char32_tを除くunsigned
			 * integral型のみ対応。
			 * @param[in] value rotate対象の値。
			 * @param[in] count `TypeBitWidth<T>()`未満へ正規化済みのrotate数。
			 * @retval value 右rotate後の値。
			 * @pre `count < TypeBitWidth<T>()`。未対応型はcompile error。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			constexpr T RotrNormalized(T value, unsigned count) noexcept
			{
				static_assert(IsSupportedUnsignedIntegral<T>::kValue,
							  "ket::bits requires an unsigned integral type except bool and plain "
							  "character types.");

				return count == 0U
					? value
					: static_cast<T>(static_cast<T>(value >> count) |
									 static_cast<T>(value << (TypeBitWidth<T>() - count)));
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr bool IsNibble(std::uint8_t value) noexcept
		{
			return value <= 0x0FU;
		}

		constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept
		{
			return static_cast<std::uint8_t>((value >> 4U) & 0x0FU);
		}

		constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept
		{
			return static_cast<std::uint8_t>(value & 0x0FU);
		}

		inline bool TryPackNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept
		{
			const auto high_is_nibble = IsNibble(high);
			const auto low_is_nibble = IsNibble(low);
			const auto both_are_nibbles = high_is_nibble && low_is_nibble;
			if (!both_are_nibbles)
			{
				return false;
			}

			out = static_cast<std::uint8_t>((high << 4U) | low);
			return true;
		}

		template <typename T>
		constexpr unsigned TypeBitWidth() noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return static_cast<unsigned>(
				std::numeric_limits<
					typename detail::IsSupportedUnsignedIntegral<T>::ValueType>::digits);
		}

		template <typename T>
		constexpr bool HasBit(T value, unsigned bit_index) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return bit_index < TypeBitWidth<T>() &&
				(value & detail::SingleBit<T>(bit_index)) != T(0);
		}

		template <typename T>
		bool TrySetBit(T value, unsigned bit_index, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			const auto bit_width = TypeBitWidth<T>();
			const auto bit_is_in_range = bit_index < bit_width;
			if (!bit_is_in_range)
			{
				return false;
			}

			const auto mask = detail::SingleBit<T>(bit_index);
			out = static_cast<T>(value | mask);
			return true;
		}

		template <typename T>
		bool TryClearBit(T value, unsigned bit_index, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			const auto bit_width = TypeBitWidth<T>();
			const auto bit_is_in_range = bit_index < bit_width;
			if (!bit_is_in_range)
			{
				return false;
			}

			const auto mask = static_cast<T>(~detail::SingleBit<T>(bit_index));
			out = static_cast<T>(value & mask);
			return true;
		}

		template <typename T>
		bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			const auto bit_width = TypeBitWidth<T>();
			const auto bit_is_in_range = bit_index < bit_width;
			if (!bit_is_in_range)
			{
				return false;
			}

			const auto mask = detail::SingleBit<T>(bit_index);
			out = static_cast<T>(value ^ mask);
			return true;
		}

		template <typename T>
		bool TryMask(unsigned width, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			const auto bit_width = TypeBitWidth<T>();
			const auto width_is_too_large = width > bit_width;
			if (width_is_too_large)
			{
				return false;
			}

			const auto width_is_zero = width == 0U;
			if (width_is_zero)
			{
				out = T(0);
				return true;
			}

			const auto width_is_full = width == bit_width;
			if (width_is_full)
			{
				out = static_cast<T>(~T(0));
				return true;
			}

			out = static_cast<T>(detail::SingleBit<T>(width) - T(1));
			return true;
		}

		// NOLINTBEGIN(misc-no-recursion): C++11 constexpr popcount は再帰で表現。
		template <typename T>
		constexpr unsigned PopCount(T value) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return value == T(0) ? 0U
								 : static_cast<unsigned>((value & T(1)) != T(0)) +
					PopCount(static_cast<T>(value >> 1U));
		}
		// NOLINTEND(misc-no-recursion)

		template <typename T>
		constexpr bool IsPowerOfTwo(T value) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return value != T(0) && (value & static_cast<T>(value - T(1))) == T(0);
		}

		template <typename T>
		constexpr T Rotl(T value, unsigned count) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return detail::RotlNormalized(value, count % TypeBitWidth<T>());
		}

		template <typename T>
		constexpr T Rotr(T value, unsigned count) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::kValue,
						  "ket::bits requires an unsigned integral type except bool and plain "
						  "character types.");

			return detail::RotrNormalized(value, count % TypeBitWidth<T>());
		}

	} // namespace bits

} // namespace ket
