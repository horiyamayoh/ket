#pragma once

/**
 * @file ket_math.h
 * @brief 補間、角度変換、byte単位変換の小さい数学補助API。
 *
 * @details 浮動小数点の補間、角度のdegree/radian変換、絶対許容差比較、KiB/MiBとbyteの
 * 変換を短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 * units frameworkやstatisticsへ広げず、単位名とoverflow方針をAPIで固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_math.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：小さい数学処理の丸め、overflow、単位名をAPIで固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::math
 * 内部実装：ket::math::detail
 */

#include <cstdint>
#include <limits>
#include <type_traits>

namespace ket
{
	namespace math
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 2値間の線形補間。
		 * @tparam T 浮動小数点型。非floating-point型はcompile error。
		 * @param[in] a `t == 0`の端点値。
		 * @param[in] b `t == 1`の端点値。
		 * @param[in] t 補間係数。0から1の範囲外では外挿。
		 * @retval value `a + (b - a) * t`で求めた値。
		 * @pre `T`はfloating-point型。
		 * @post 引数と外部状態の変更なし。`a`、`b`、`t`の丸め規則は通常の浮動小数点演算に従う。
		 * @code
		 * const auto value = ket::math::Lerp(10.0, 20.0, 0.25);
		 * // value == 12.5
		 * @endcode
		 */
		template <typename T>
		constexpr T Lerp(T a, T b, T t) noexcept;

		/**
		 * @brief degreeをradianへ変換。
		 * @tparam T 浮動小数点型。非floating-point型はcompile error。
		 * @param[in] degrees degree単位の角度。
		 * @retval value radian単位の角度。
		 * @pre `T`はfloating-point型。
		 * @post 引数と外部状態の変更なし。円周率定数は`T`へ丸めてから演算。
		 * @code
		 * const auto radians = ket::math::ToRadians(180.0);
		 * // radians ~= 3.141592653589793
		 * @endcode
		 */
		template <typename T>
		constexpr T ToRadians(T degrees) noexcept;

		/**
		 * @brief radianをdegreeへ変換。
		 * @tparam T 浮動小数点型。非floating-point型はcompile error。
		 * @param[in] radians radian単位の角度。
		 * @retval value degree単位の角度。
		 * @pre `T`はfloating-point型。
		 * @post 引数と外部状態の変更なし。円周率定数は`T`へ丸めてから演算。
		 * @code
		 * const auto degrees = ket::math::ToDegrees(3.141592653589793);
		 * // degrees ~= 180.0
		 * @endcode
		 */
		template <typename T>
		constexpr T ToDegrees(T radians) noexcept;

		/**
		 * @brief 正の絶対許容差による浮動小数点近似比較。
		 * @tparam T 浮動小数点型。非floating-point型はcompile error。
		 * @param[in] a 比較対象の値。
		 * @param[in] b 比較対象の値。
		 * @param[in] epsilon 正の絶対許容差。
		 * @retval true `epsilon`が正で、NaN入力がなく、`a == b`または絶対差が`epsilon`以下。
		 * @retval false `epsilon <= 0`、NaN入力、または絶対差が`epsilon`を超える値。
		 * @pre `T`はfloating-point型。
		 * @post 引数と外部状態の変更なし。Inf同士は通常の等価比較結果を先に使う。
		 * @code
		 * const auto close = ket::math::NearlyEqual(1.0, 1.001, 0.01);
		 * // close == true
		 * @endcode
		 */
		template <typename T>
		constexpr bool NearlyEqual(T a, T b, T epsilon) noexcept;

		/**
		 * @brief KiB単位の整数値をbyte数へ変換。
		 * @param[in] kib KiB単位の値。1 KiBは1024 bytes。
		 * @param[out] out 変換後のbyte数。成功時だけ更新。
		 * @retval true `kib * 1024`が`std::uint64_t`へ収まった。
		 * @retval false byte数への変換でoverflowする値。
		 * @pre なし。overflowは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * std::uint64_t bytes = 0;
		 * const auto ok = ket::math::TryBytesFromKiB(4, bytes);
		 * // ok == true, bytes == 4096
		 * @endcode
		 */
		inline bool TryBytesFromKiB(std::uint64_t kib, std::uint64_t& out) noexcept;

		/**
		 * @brief MiB単位の整数値をbyte数へ変換。
		 * @param[in] mib MiB単位の値。1 MiBは1048576 bytes。
		 * @param[out] out 変換後のbyte数。成功時だけ更新。
		 * @retval true `mib * 1048576`が`std::uint64_t`へ収まった。
		 * @retval false byte数への変換でoverflowする値。
		 * @pre なし。overflowは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * std::uint64_t bytes = 0;
		 * const auto ok = ket::math::TryBytesFromMiB(2, bytes);
		 * // ok == true, bytes == 2097152
		 * @endcode
		 */
		inline bool TryBytesFromMiB(std::uint64_t mib, std::uint64_t& out) noexcept;

		/**
		 * @brief byte数をKiB単位のdoubleへ変換。
		 * @param[in] bytes byte単位の値。
		 * @retval value `bytes / 1024.0`。
		 * @pre なし。任意の`std::uint64_t`値を受け付ける。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto kib = ket::math::BytesToKiB(1536);
		 * // kib == 1.5
		 * @endcode
		 */
		constexpr double BytesToKiB(std::uint64_t bytes) noexcept;

		/**
		 * @brief byte数をMiB単位のdoubleへ変換。
		 * @param[in] bytes byte単位の値。
		 * @retval value `bytes / 1048576.0`。
		 * @pre なし。任意の`std::uint64_t`値を受け付ける。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto mib = ket::math::BytesToMiB(1572864);
		 * // mib == 1.5
		 * @endcode
		 */
		constexpr double BytesToMiB(std::uint64_t bytes) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			constexpr std::uint64_t kBytesPerKiB = 1024ULL;
			constexpr std::uint64_t kBytesPerMiB = kBytesPerKiB * kBytesPerKiB;

			/**
			 * @brief floating-point型判定。
			 * @tparam T 判定対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsFloatingPoint : std::is_floating_point<T>
			{
			};

			/**
			 * @brief 円周率定数の取得。
			 * @tparam T 浮動小数点型。
			 * @retval value `T`へ丸めた円周率。
			 * @pre `T`はfloating-point型。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			constexpr T Pi() noexcept
			{
				static_assert(detail::IsFloatingPoint<T>::value,
							  "ket::math floating-point APIs accept floating-point types only.");

				return static_cast<T>(3.141592653589793238462643383279502884L);
			}

			/**
			 * @brief 浮動小数点値の絶対値取得。
			 * @tparam T 浮動小数点型。
			 * @param[in] value 対象値。
			 * @retval value `value`が負なら符号反転した値、それ以外は入力値。
			 * @pre `T`はfloating-point型。
			 * @post 引数と外部状態の変更なし。NaNはNaNのまま返る。
			 */
			template <typename T>
			constexpr T Abs(T value) noexcept
			{
				static_assert(detail::IsFloatingPoint<T>::value,
							  "ket::math floating-point APIs accept floating-point types only.");

				return value < static_cast<T>(0) ? -value : value;
			}

			/**
			 * @brief NaN判定。
			 * @tparam T 浮動小数点型。
			 * @param[in] value 判定対象の値。
			 * @retval true `value`がNaN。
			 * @retval false `value`がNaN以外。
			 * @pre `T`はfloating-point型。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			constexpr bool IsNan(T value) noexcept
			{
				static_assert(detail::IsFloatingPoint<T>::value,
							  "ket::math floating-point APIs accept floating-point types only.");

				return value != value; // NOLINT(misc-redundant-expression)
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		constexpr T Lerp(T a, T b, T t) noexcept
		{
			static_assert(detail::IsFloatingPoint<T>::value,
						  "ket::math::Lerp accepts floating-point types only.");

			return a + ((b - a) * t);
		}

		template <typename T>
		constexpr T ToRadians(T degrees) noexcept
		{
			static_assert(detail::IsFloatingPoint<T>::value,
						  "ket::math::ToRadians accepts floating-point types only.");

			return degrees * (detail::Pi<T>() / static_cast<T>(180));
		}

		template <typename T>
		constexpr T ToDegrees(T radians) noexcept
		{
			static_assert(detail::IsFloatingPoint<T>::value,
						  "ket::math::ToDegrees accepts floating-point types only.");

			return radians * (static_cast<T>(180) / detail::Pi<T>());
		}

		template <typename T>
		constexpr bool NearlyEqual(T a, T b, T epsilon) noexcept
		{
			static_assert(detail::IsFloatingPoint<T>::value,
						  "ket::math::NearlyEqual accepts floating-point types only.");

			return epsilon > static_cast<T>(0) && !detail::IsNan(epsilon) && !detail::IsNan(a) &&
				!detail::IsNan(b) && (a == b || detail::Abs(a - b) <= epsilon);
		}

		inline bool TryBytesFromKiB(std::uint64_t kib, std::uint64_t& out) noexcept
		{
			const auto max_kib = std::numeric_limits<std::uint64_t>::max() / detail::kBytesPerKiB;
			const auto overflows = kib > max_kib;
			if (overflows)
			{
				return false;
			}

			out = kib * detail::kBytesPerKiB;
			return true;
		}

		inline bool TryBytesFromMiB(std::uint64_t mib, std::uint64_t& out) noexcept
		{
			const auto max_mib = std::numeric_limits<std::uint64_t>::max() / detail::kBytesPerMiB;
			const auto overflows = mib > max_mib;
			if (overflows)
			{
				return false;
			}

			out = mib * detail::kBytesPerMiB;
			return true;
		}

		constexpr double BytesToKiB(std::uint64_t bytes) noexcept
		{
			return static_cast<double>(bytes) / static_cast<double>(detail::kBytesPerKiB);
		}

		constexpr double BytesToMiB(std::uint64_t bytes) noexcept
		{
			return static_cast<double>(bytes) / static_cast<double>(detail::kBytesPerMiB);
		}

	} // namespace math

} // namespace ket
