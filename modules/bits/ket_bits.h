#pragma once

/**
 * @file ket_bits.h
 * @brief bit、nibble、mask、rotateの小処理API。
 *
 * @details unsigned integralに限定したbit処理を、shift幅や範囲外bit indexの事故を避ける
 * 小さい名前付きAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 * nibble抽出、失敗値付きmask生成、popcount、rotateを標準ライブラリのみで扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_bits.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：unsigned integralの小さいbit処理を標準ライブラリだけで安全に名前付けできる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：C++20 `<bit>` と一部重なるが、nibble、失敗値付きmask、
 * bit index境界処理の直接代替ではない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket
 * 内部実装：ket::detail
 */

#include <cstdint>
#include <limits>
#include <type_traits>

namespace ket
{
	// -----------------------------------------------------------------------------
	// Public API declarations
	// -----------------------------------------------------------------------------

	/**
	 * @brief 4bit nibble範囲の判定。
	 * @param[in] value 判定対象値。
	 * @retval true `value`が0x00から0x0Fの範囲内。
	 * @retval false `value`が4bit nibble範囲外。
	 * @pre なし。任意のstd::uint8_t値を判定対象にする。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto valid = ket::IsNibble(0x0FU);
	 * // valid == true
	 * @endcode
	 */
	constexpr bool IsNibble(std::uint8_t value) noexcept;

	/**
	 * @brief byte上位nibbleの取得。
	 * @param[in] value 取得対象byte。
	 * @retval value `value`の上位4bitを0x00から0x0Fの範囲へ右詰めした値。
	 * @pre なし。任意のstd::uint8_t値を入力できる。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto high = ket::HighNibble(0xABU);
	 * // high == 0x0A
	 * @endcode
	 */
	constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept;

	/**
	 * @brief byte下位nibbleの取得。
	 * @param[in] value 取得対象byte。
	 * @retval value `value`の下位4bitを0x00から0x0Fの範囲へ残した値。
	 * @pre なし。任意のstd::uint8_t値を入力できる。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto low = ket::LowNibble(0xABU);
	 * // low == 0x0B
	 * @endcode
	 */
	constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept;

	/**
	 * @brief 2つのnibbleからbyteを生成。
	 * @param[in] high 上位nibbleとして配置する値。
	 * @param[in] low 下位nibbleとして配置する値。
	 * @param[out] out 生成したbyte。失敗時は変更なし。
	 * @retval true `high`と`low`がnibble範囲内で、`out`を更新。
	 * @retval false `high`または`low`がnibble範囲外で、`out`は不変。
	 * @pre `out`は有効なstd::uint8_tオブジェクトへの参照。
	 * @post 成功時だけ`out`を更新。失敗時は入力時の`out`値を保持。
	 * @code
	 * std::uint8_t value = 0U;
	 * const auto ok = ket::TryMakeByteFromNibbles(0x0AU, 0x0BU, value);
	 * // ok == true, value == 0xAB
	 * @endcode
	 */
	inline bool
	TryMakeByteFromNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept;

	/**
	 * @brief unsigned integral型のbit幅取得。
	 * @tparam T 対象のunsigned integral型。
	 * @retval value `T`の値表現に含まれるbit数。
	 * @pre `T`はunsigned integral型。
	 * @post 外部状態の変更なし。
	 * @code
	 * const auto width = ket::BitWidth<std::uint8_t>();
	 * // width == 8
	 * @endcode
	 */
	template <typename T>
	constexpr unsigned BitWidth() noexcept;

	/**
	 * @brief 指定bitが1かどうかの判定。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 判定対象値。
	 * @param[in] bit_index 判定対象bit index。最下位bitを0とする。
	 * @retval true `bit_index`が範囲内で、該当bitが1。
	 * @retval false `bit_index`が範囲外、または該当bitが0。
	 * @pre `T`はunsigned integral型。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto has_bit = ket::HasBit<std::uint8_t>(0x80U, 7U);
	 * // has_bit == true
	 * @endcode
	 */
	template <typename T>
	constexpr bool HasBit(T value, unsigned bit_index) noexcept;

	/**
	 * @brief 指定bitを1にした値の生成。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 変換元の値。
	 * @param[in] bit_index 1にするbit index。最下位bitを0とする。
	 * @param[out] out 生成した値。失敗時は変更なし。
	 * @retval true `bit_index`が範囲内で、`out`を更新。
	 * @retval false `bit_index`が範囲外で、`out`は不変。
	 * @pre `T`はunsigned integral型。`out`は有効な`T`オブジェクトへの参照。
	 * @post 成功時だけ`out`を更新。失敗時は入力時の`out`値を保持。
	 * @code
	 * std::uint8_t value = 0U;
	 * const auto ok = ket::TrySetBit<std::uint8_t>(0U, 7U, value);
	 * // ok == true, value == 0x80
	 * @endcode
	 */
	template <typename T>
	bool TrySetBit(T value, unsigned bit_index, T& out) noexcept;

	/**
	 * @brief 指定bitを0にした値の生成。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 変換元の値。
	 * @param[in] bit_index 0にするbit index。最下位bitを0とする。
	 * @param[out] out 生成した値。失敗時は変更なし。
	 * @retval true `bit_index`が範囲内で、`out`を更新。
	 * @retval false `bit_index`が範囲外で、`out`は不変。
	 * @pre `T`はunsigned integral型。`out`は有効な`T`オブジェクトへの参照。
	 * @post 成功時だけ`out`を更新。失敗時は入力時の`out`値を保持。
	 * @code
	 * std::uint8_t value = 0U;
	 * const auto ok = ket::TryClearBit<std::uint8_t>(0xFFU, 7U, value);
	 * // ok == true, value == 0x7F
	 * @endcode
	 */
	template <typename T>
	bool TryClearBit(T value, unsigned bit_index, T& out) noexcept;

	/**
	 * @brief 指定bitを反転した値の生成。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 変換元の値。
	 * @param[in] bit_index 反転するbit index。最下位bitを0とする。
	 * @param[out] out 生成した値。失敗時は変更なし。
	 * @retval true `bit_index`が範囲内で、`out`を更新。
	 * @retval false `bit_index`が範囲外で、`out`は不変。
	 * @pre `T`はunsigned integral型。`out`は有効な`T`オブジェクトへの参照。
	 * @post 成功時だけ`out`を更新。失敗時は入力時の`out`値を保持。
	 * @code
	 * std::uint8_t value = 0U;
	 * const auto ok = ket::TryToggleBit<std::uint8_t>(0x03U, 1U, value);
	 * // ok == true, value == 0x01
	 * @endcode
	 */
	template <typename T>
	bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept;

	/**
	 * @brief 下位`width` bitを1にしたmaskの生成。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] width 1にする下位bit数。
	 * @param[out] out 生成したmask。失敗時は変更なし。
	 * @retval true `width <= BitWidth<T>()`で、`out`を更新。
	 * @retval false `width > BitWidth<T>()`で、`out`は不変。
	 * @pre `T`はunsigned integral型。`out`は有効な`T`オブジェクトへの参照。
	 * @post 成功時だけ`out`を更新。失敗時は入力時の`out`値を保持。
	 * @note `width == 0`は0、`width == BitWidth<T>()`は全bit 1。
	 * @code
	 * std::uint8_t mask = 0U;
	 * const auto ok = ket::TryMask<std::uint8_t>(4U, mask);
	 * // ok == true, mask == 0x0F
	 * @endcode
	 */
	template <typename T>
	bool TryMask(unsigned width, T& out) noexcept;

	/**
	 * @brief 1のbit数の取得。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 計数対象値。
	 * @retval value `value`に含まれる1のbit数。
	 * @pre `T`はunsigned integral型。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto count = ket::PopCount<std::uint8_t>(0x81U);
	 * // count == 2
	 * @endcode
	 */
	template <typename T>
	constexpr unsigned PopCount(T value) noexcept;

	/**
	 * @brief 2の冪かどうかの判定。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value 判定対象値。
	 * @retval true `value`が2の冪。
	 * @retval false `value`が0、または2の冪ではない値。
	 * @pre `T`はunsigned integral型。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto power = ket::IsPowerOfTwo<std::uint8_t>(0x80U);
	 * // power == true
	 * @endcode
	 */
	template <typename T>
	constexpr bool IsPowerOfTwo(T value) noexcept;

	/**
	 * @brief 左rotate。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value rotate対象値。
	 * @param[in] count rotate数。bit幅で剰余化して扱う。
	 * @retval value 左rotate後の値。
	 * @pre `T`はunsigned integral型。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto value = ket::Rotl<std::uint8_t>(0x81U, 1U);
	 * // value == 0x03
	 * @endcode
	 */
	template <typename T>
	constexpr T Rotl(T value, unsigned count) noexcept;

	/**
	 * @brief 右rotate。
	 * @tparam T 対象のunsigned integral型。
	 * @param[in] value rotate対象値。
	 * @param[in] count rotate数。bit幅で剰余化して扱う。
	 * @retval value 右rotate後の値。
	 * @pre `T`はunsigned integral型。
	 * @post 引数と外部状態の変更なし。
	 * @code
	 * const auto value = ket::Rotr<std::uint8_t>(0x81U, 1U);
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
		 * @brief unsigned integral型の判定。
		 * @tparam T 判定対象型。
		 * @note detail配下の型は公開APIではない。
		 */
		template <typename T>
		struct IsUnsignedIntegral : std::integral_constant<bool,
														   std::numeric_limits<T>::is_integer &&
															   !std::numeric_limits<T>::is_signed>
		{
		};

		/**
		 * @brief `T`のbit演算対象制約をコンパイル時に確認。
		 * @tparam T 確認対象型。
		 * @retval true `T`がunsigned integral型。
		 * @pre なし。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		template <typename T>
		constexpr bool IsSupportedBitType() noexcept
		{
			return IsUnsignedIntegral<T>::value;
		}

		/**
		 * @brief 指定bitだけを1にしたmaskの生成。
		 * @tparam T 対象のunsigned integral型。
		 * @param[in] bit_index 1にするbit index。
		 * @retval value 指定bitだけを1にしたmask。
		 * @pre `bit_index < BitWidth<T>()`。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		template <typename T>
		constexpr T SingleBitMask(unsigned bit_index) noexcept
		{
			return static_cast<T>(T(1) << bit_index);
		}

		/**
		 * @brief 交互bit maskの取得。
		 * @retval value bit列`0101...`のstd::uintmax_t mask。
		 * @pre なし。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountPairMask() noexcept
		{
			return std::numeric_limits<std::uintmax_t>::max() / 3U;
		}

		/**
		 * @brief 2bit group maskの取得。
		 * @retval value bit列`0011...`のstd::uintmax_t mask。
		 * @pre なし。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountTwoBitMask() noexcept
		{
			return std::numeric_limits<std::uintmax_t>::max() / 5U;
		}

		/**
		 * @brief 4bit group maskの取得。
		 * @retval value bit列`00001111...`のstd::uintmax_t mask。
		 * @pre なし。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountNibbleMask() noexcept
		{
			return std::numeric_limits<std::uintmax_t>::max() / 17U;
		}

		/**
		 * @brief byte単位の合計係数取得。
		 * @retval value bit列`00000001...`のstd::uintmax_t係数。
		 * @pre なし。
		 * @post 外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountByteSumFactor() noexcept
		{
			return std::numeric_limits<std::uintmax_t>::max() / 255U;
		}

		/**
		 * @brief 1bit pairごとのcount加算。
		 * @param[in] value 計数対象値。
		 * @retval value 2bit単位の部分popcount。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountSubtractPairs(std::uintmax_t value) noexcept
		{
			return value - ((value >> 1U) & PopCountPairMask());
		}

		/**
		 * @brief 2bit groupごとのcount加算。
		 * @param[in] value 2bit単位の部分popcount。
		 * @retval value 4bit単位の部分popcount。
		 * @pre `value`はPopCountSubtractPairsの結果。
		 * @post 引数と外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountPairSums(std::uintmax_t value) noexcept
		{
			return (value & PopCountTwoBitMask()) + ((value >> 2U) & PopCountTwoBitMask());
		}

		/**
		 * @brief 4bit groupごとのcount加算。
		 * @param[in] value 4bit単位へ集約前の部分popcount。
		 * @retval value 4bit単位の部分popcount。
		 * @pre `value`はPopCountPairSumsの結果。
		 * @post 引数と外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		constexpr std::uintmax_t PopCountNibbleSums(std::uintmax_t value) noexcept
		{
			return (value + (value >> 4U)) & PopCountNibbleMask();
		}

		/**
		 * @brief std::uintmax_t値のpopcount。
		 * @param[in] value 計数対象値。
		 * @retval value `value`に含まれる1のbit数。
		 * @pre std::uintmax_tのbit幅はstd::uint8_t単位で表現可能。
		 * @post 引数と外部状態の変更なし。
		 * @note C++11 constexpr制約に収めるため、分岐とloopを使わないSWAR計算。
		 */
		constexpr unsigned PopCountUintmax(std::uintmax_t value) noexcept
		{
			return static_cast<unsigned>(
				(PopCountNibbleSums(PopCountPairSums(PopCountSubtractPairs(value))) *
				 PopCountByteSumFactor()) >>
				((sizeof(std::uintmax_t) - 1U) * std::numeric_limits<std::uint8_t>::digits));
		}

		/**
		 * @brief 正規化済みcountによる左rotate。
		 * @tparam T 対象のunsigned integral型。
		 * @param[in] value rotate対象値。
		 * @param[in] count `0 <= count < BitWidth<T>()`のrotate数。
		 * @retval value 左rotate後の値。
		 * @pre `T`はunsigned integral型。`count < BitWidth<T>()`。
		 * @post 引数と外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		template <typename T>
		constexpr T RotlNormalized(T value, unsigned count) noexcept
		{
			return count == 0U
				? value
				: static_cast<T>((value << count) | (value >> (BitWidth<T>() - count)));
		}

		/**
		 * @brief 正規化済みcountによる右rotate。
		 * @tparam T 対象のunsigned integral型。
		 * @param[in] value rotate対象値。
		 * @param[in] count `0 <= count < BitWidth<T>()`のrotate数。
		 * @retval value 右rotate後の値。
		 * @pre `T`はunsigned integral型。`count < BitWidth<T>()`。
		 * @post 引数と外部状態の変更なし。
		 * @note detail配下の関数は公開APIではない。
		 */
		template <typename T>
		constexpr T RotrNormalized(T value, unsigned count) noexcept
		{
			return count == 0U
				? value
				: static_cast<T>((value >> count) | (value << (BitWidth<T>() - count)));
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

	inline bool
	TryMakeByteFromNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept
	{
		const auto high_is_nibble = IsNibble(high);
		const auto low_is_nibble = IsNibble(low);
		const auto nibbles_are_valid = high_is_nibble && low_is_nibble;
		if (!nibbles_are_valid)
		{
			return false;
		}

		out = static_cast<std::uint8_t>((high << 4U) | low);
		return true;
	}

	template <typename T>
	constexpr unsigned BitWidth() noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::BitWidth requires unsigned integral T.");
		return static_cast<unsigned>(std::numeric_limits<T>::digits);
	}

	template <typename T>
	constexpr bool HasBit(T value, unsigned bit_index) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(), "ket::HasBit requires unsigned integral T.");
		return bit_index < BitWidth<T>() && (value & detail::SingleBitMask<T>(bit_index)) != T(0);
	}

	template <typename T>
	bool TrySetBit(T value, unsigned bit_index, T& out) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::TrySetBit requires unsigned integral T.");

		const auto bit_index_is_valid = bit_index < BitWidth<T>();
		if (!bit_index_is_valid)
		{
			return false;
		}

		const auto mask = detail::SingleBitMask<T>(bit_index);
		out = static_cast<T>(value | mask);
		return true;
	}

	template <typename T>
	bool TryClearBit(T value, unsigned bit_index, T& out) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::TryClearBit requires unsigned integral T.");

		const auto bit_index_is_valid = bit_index < BitWidth<T>();
		if (!bit_index_is_valid)
		{
			return false;
		}

		const auto mask = detail::SingleBitMask<T>(bit_index);
		out = static_cast<T>(value & static_cast<T>(~mask));
		return true;
	}

	template <typename T>
	bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::TryToggleBit requires unsigned integral T.");

		const auto bit_index_is_valid = bit_index < BitWidth<T>();
		if (!bit_index_is_valid)
		{
			return false;
		}

		const auto mask = detail::SingleBitMask<T>(bit_index);
		out = static_cast<T>(value ^ mask);
		return true;
	}

	template <typename T>
	bool TryMask(unsigned width, T& out) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::TryMask requires unsigned integral T.");

		const auto bit_width = BitWidth<T>();
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
			out = std::numeric_limits<T>::max();
			return true;
		}

		out = static_cast<T>((T(1) << width) - T(1));
		return true;
	}

	template <typename T>
	constexpr unsigned PopCount(T value) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::PopCount requires unsigned integral T.");
		return detail::PopCountUintmax(static_cast<std::uintmax_t>(value));
	}

	template <typename T>
	constexpr bool IsPowerOfTwo(T value) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(),
					  "ket::IsPowerOfTwo requires unsigned integral T.");
		return value != T(0) && (value & static_cast<T>(value - T(1))) == T(0);
	}

	template <typename T>
	constexpr T Rotl(T value, unsigned count) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(), "ket::Rotl requires unsigned integral T.");
		return detail::RotlNormalized(value, count % BitWidth<T>());
	}

	template <typename T>
	constexpr T Rotr(T value, unsigned count) noexcept
	{
		static_assert(detail::IsSupportedBitType<T>(), "ket::Rotr requires unsigned integral T.");
		return detail::RotrNormalized(value, count % BitWidth<T>());
	}

} // namespace ket
