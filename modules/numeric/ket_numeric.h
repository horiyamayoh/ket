#pragma once

/**
 * @file ket_numeric.h
 * @brief 整数算術の境界確認API。
 *
 * @details alignment、rounding、overflow、narrowing cast の小さい境界処理を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。算術式を評価する前に範囲を確認し、
 * signed overflowや意図しないwraparoundを公開仕様にしない。
 *
 * @par プロジェクトへの適用方法
 * `ket_numeric.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：overflowと範囲外を戻り値で固定し、手書き算術の未定義動作を避けられる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::numeric
 * 内部実装：ket::numeric::detail
 */

#include <cstdint>
#include <limits>
#include <type_traits>

namespace ket
{
	namespace numeric
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 値が指定変換先整数型の範囲内かの判定。
		 * @tparam To 変換先整数型。
		 * @tparam From 変換元整数型。
		 * @param[in] value 判定対象値。
		 * @retval true `value`が`To`の表現範囲内。
		 * @retval false `value`が`To`の表現範囲外。
		 * @pre `To`と`From`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @note signed/unsignedの組み合わせでも比較前に十分広い整数表現へ変換。
		 * @code
		 * const auto ok = ket::numeric::InRange<int>(42U);
		 * const auto ng = ket::numeric::InRange<unsigned>(-1);
		 * // ok == true, ng == false
		 * @endcode
		 */
		template <typename To, typename From>
		constexpr bool InRange(From value) noexcept;

		/**
		 * @brief 値を指定範囲へ丸める。
		 * @tparam T 対象整数型。
		 * @param[in] value 丸め対象値。
		 * @param[in] min_value 下限値。
		 * @param[in] max_value 上限値。
		 * @retval value `min_value`以上`max_value`以下へ丸めた値。
		 * @pre `T`はboolとcharacter型を除くintegral型。`min_value <= max_value`。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::numeric::Clamp(10, 0, 7);
		 * // value == 7
		 * @endcode
		 */
		template <typename T>
		constexpr T Clamp(T value, T min_value, T max_value) noexcept;

		// NOLINTBEGIN(modernize-type-traits)
		/**
		 * @brief 2値の差の絶対値をunsigned整数で取得。
		 * @tparam T 対象整数型。
		 * @param[in] a 1つ目の値。
		 * @param[in] b 2つ目の値。
		 * @retval value `a`と`b`の数学的な差の絶対値。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @note signed最小値を単純に符号反転せず、unsigned変換後の差で表現。
		 * @code
		 * const auto diff = ket::numeric::AbsDiff(-3, 2);
		 * // diff == 5U
		 * @endcode
		 */
		template <typename T>
		constexpr typename std::make_unsigned<T>::type AbsDiff(T a, T b) noexcept;
		// NOLINTEND(modernize-type-traits)

		/**
		 * @brief unsigned整数の切り上げ除算。
		 * @tparam T 対象unsigned整数型。
		 * @param[in] value 割られる値。
		 * @param[in] divisor 除数。
		 * @param[out] out 成功時の切り上げ除算結果。
		 * @retval true 成功。
		 * @retval false `divisor == 0`。
		 * @pre `T`はboolとcharacter型を除くunsigned integral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * unsigned out = 0;
		 * const auto ok = ket::numeric::TryDivideRoundUp(10U, 4U, out);
		 * // ok == true, out == 3U
		 * @endcode
		 */
		template <typename T>
		bool TryDivideRoundUp(T value, T divisor, T& out) noexcept;

		/**
		 * @brief unsigned整数を指定alignmentの倍数へ切り上げる。
		 * @tparam T 対象unsigned整数型。
		 * @param[in] value 丸め対象値。
		 * @param[in] alignment alignment値。
		 * @param[out] out 成功時の切り上げ後の値。
		 * @retval true 成功。
		 * @retval false `alignment == 0`、または切り上げ結果のoverflow。
		 * @pre `T`はboolとcharacter型を除くunsigned integral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * unsigned out = 0;
		 * const auto ok = ket::numeric::TryAlignUp(13U, 8U, out);
		 * // ok == true, out == 16U
		 * @endcode
		 */
		template <typename T>
		bool TryAlignUp(T value, T alignment, T& out) noexcept;

		/**
		 * @brief unsigned整数を指定alignmentの倍数へ切り下げる。
		 * @tparam T 対象unsigned整数型。
		 * @param[in] value 丸め対象値。
		 * @param[in] alignment alignment値。
		 * @param[out] out 成功時の切り下げ後の値。
		 * @retval true 成功。
		 * @retval false `alignment == 0`。
		 * @pre `T`はboolとcharacter型を除くunsigned integral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * unsigned out = 0;
		 * const auto ok = ket::numeric::TryAlignDown(13U, 8U, out);
		 * // ok == true, out == 8U
		 * @endcode
		 */
		template <typename T>
		bool TryAlignDown(T value, T alignment, T& out) noexcept;

		/**
		 * @brief overflowを検出する整数加算。
		 * @tparam T 対象整数型。
		 * @param[in] a 左辺値。
		 * @param[in] b 右辺値。
		 * @param[out] out 成功時の加算結果。
		 * @retval true 成功。
		 * @retval false 加算結果のoverflow。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * int out = 0;
		 * const auto ok = ket::numeric::TryCheckedAdd(40, 2, out);
		 * // ok == true, out == 42
		 * @endcode
		 */
		template <typename T>
		bool TryCheckedAdd(T a, T b, T& out) noexcept;

		/**
		 * @brief overflowを検出する整数減算。
		 * @tparam T 対象整数型。
		 * @param[in] a 左辺値。
		 * @param[in] b 右辺値。
		 * @param[out] out 成功時の減算結果。
		 * @retval true 成功。
		 * @retval false 減算結果のoverflow。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * int out = 0;
		 * const auto ok = ket::numeric::TryCheckedSub(40, 2, out);
		 * // ok == true, out == 38
		 * @endcode
		 */
		template <typename T>
		bool TryCheckedSub(T a, T b, T& out) noexcept;

		/**
		 * @brief overflowを検出する整数乗算。
		 * @tparam T 対象整数型。
		 * @param[in] a 左辺値。
		 * @param[in] b 右辺値。
		 * @param[out] out 成功時の乗算結果。
		 * @retval true 成功。
		 * @retval false 乗算結果のoverflow。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * int out = 0;
		 * const auto ok = ket::numeric::TryCheckedMul(6, 7, out);
		 * // ok == true, out == 42
		 * @endcode
		 */
		template <typename T>
		bool TryCheckedMul(T a, T b, T& out) noexcept;

		/**
		 * @brief 整数加算を上下限で飽和させる。
		 * @tparam T 対象整数型。
		 * @param[in] a 左辺値。
		 * @param[in] b 右辺値。
		 * @retval value 加算結果、または`T`の上下限へ飽和した値。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::numeric::SaturatingAdd(std::numeric_limits<int>::max(), 1);
		 * // value == std::numeric_limits<int>::max()
		 * @endcode
		 */
		template <typename T>
		constexpr T SaturatingAdd(T a, T b) noexcept;

		/**
		 * @brief 整数減算を上下限で飽和させる。
		 * @tparam T 対象整数型。
		 * @param[in] a 左辺値。
		 * @param[in] b 右辺値。
		 * @retval value 減算結果、または`T`の上下限へ飽和した値。
		 * @pre `T`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::numeric::SaturatingSub(std::numeric_limits<int>::min(), 1);
		 * // value == std::numeric_limits<int>::min()
		 * @endcode
		 */
		template <typename T>
		constexpr T SaturatingSub(T a, T b) noexcept;

		/**
		 * @brief 範囲外を検出する整数cast。
		 * @tparam To 変換先整数型。
		 * @tparam From 変換元整数型。
		 * @param[in] value 変換対象値。
		 * @param[out] out 成功時のcast結果。
		 * @retval true 成功。
		 * @retval false `value`が`To`の表現範囲外。
		 * @pre `To`と`From`はboolとcharacter型を除くintegral型。対象外型はcompile error。
		 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
		 * @code
		 * unsigned out = 0;
		 * const auto ok = ket::numeric::TryCheckedCast(42, out);
		 * // ok == true, out == 42U
		 * @endcode
		 */
		template <typename To, typename From>
		bool TryCheckedCast(From value, To& out) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief cv修飾を外した型の取得。
			 * @tparam T 対象型。
			 * @note C++11互換のため、`std::remove_cv_t`は使わない。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct RemoveCv
			{
				using Type = typename std::remove_cv<T>::type; // NOLINT(modernize-type-traits)
			};

			/**
			 * @brief cv修飾を外して同一型か判定。
			 * @tparam T 判定対象型。
			 * @tparam U 比較対象型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, typename U>
			struct IsSame : std::is_same<typename RemoveCv<T>::Type, U>
			{
			};

			/**
			 * @brief cv修飾を外してintegral型か判定。
			 * @tparam T 判定対象型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsIntegral : std::is_integral<typename RemoveCv<T>::Type>
			{
			};

			/**
			 * @brief signed整数型か判定。
			 * @tparam T 判定対象型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsSigned : std::is_signed<T>
			{
			};

			/**
			 * @brief unsigned版整数型の取得。
			 * @tparam T 対象整数型。
			 * @note C++11互換のため、`std::make_unsigned_t`は使わない。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct MakeUnsigned
			{
				using Type = typename std::make_unsigned<T>::type; // NOLINT(modernize-type-traits)
			};

			/**
			 * @brief character型かの判定。
			 * @tparam T 判定対象型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsCharacterIntegral
				: std::integral_constant<
					  bool,
					  IsSame<T, char>::value || IsSame<T, signed char>::value ||
						  IsSame<T, unsigned char>::value || IsSame<T, wchar_t>::value ||
						  IsSame<T, char16_t>::value || IsSame<T, char32_t>::value>
			{
			};

			/**
			 * @brief numeric APIが受け付ける整数型かの判定。
			 * @tparam T 判定対象型。
			 * @note boolとcharacter型は対象外。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsSupportedIntegral
				: std::integral_constant<bool,
										 IsIntegral<T>::value && !IsSame<T, bool>::value &&
											 !IsCharacterIntegral<T>::value>
			{
			};

			/**
			 * @brief numeric APIが受け付けるunsigned整数型かの判定。
			 * @tparam T 判定対象型。
			 * @note alignmentと切り上げ除算用。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsSupportedUnsignedIntegral
				: std::integral_constant<bool, IsSupportedIntegral<T>::value && !IsSigned<T>::value>
			{
			};

			/**
			 * @brief 上限側のclamp。
			 * @param[in] value 丸め対象値。
			 * @param[in] max_value 上限値。
			 * @retval value `max_value`以下へ丸めた値。
			 * @pre `T`はboolとcharacter型を除くintegral型。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			constexpr T ClampUpper(T value, T max_value) noexcept
			{
				return max_value < value ? max_value : value;
			}

			/**
			 * @brief signed/unsigned別の範囲判定実装。
			 * @tparam To 変換先整数型。
			 * @tparam From 変換元整数型。
			 * @tparam ToSigned `To`がsignedか。
			 * @tparam FromSigned `From`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename To, typename From, bool ToSigned, bool FromSigned>
			struct InRangeImpl;

			/**
			 * @brief signedからsignedへの範囲判定実装。
			 * @tparam To 変換先整数型。
			 * @tparam From 変換元整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename To, typename From>
			struct InRangeImpl<To, From, true, true>
			{
				/**
				 * @brief signed値がsigned変換先の範囲内かの判定。
				 * @param[in] value 判定対象値。
				 * @retval true 範囲内。
				 * @retval false 範囲外。
				 * @pre `To`と`From`はsigned integral型。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr bool Check(From value) noexcept
				{
					return static_cast<std::intmax_t>(value) >=
						static_cast<std::intmax_t>(std::numeric_limits<To>::min()) &&
						static_cast<std::intmax_t>(value) <=
						static_cast<std::intmax_t>(std::numeric_limits<To>::max());
				}
			};

			/**
			 * @brief unsignedからunsignedへの範囲判定実装。
			 * @tparam To 変換先整数型。
			 * @tparam From 変換元整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename To, typename From>
			struct InRangeImpl<To, From, false, false>
			{
				static constexpr bool Check(From value) noexcept
				{
					return static_cast<std::uintmax_t>(value) <=
						static_cast<std::uintmax_t>(std::numeric_limits<To>::max());
				}
			};

			/**
			 * @brief unsignedからsignedへの範囲判定実装。
			 * @tparam To 変換先整数型。
			 * @tparam From 変換元整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename To, typename From>
			struct InRangeImpl<To, From, true, false>
			{
				static constexpr bool Check(From value) noexcept
				{
					return static_cast<std::uintmax_t>(value) <=
						static_cast<std::uintmax_t>(std::numeric_limits<To>::max());
				}
			};

			/**
			 * @brief signedからunsignedへの範囲判定実装。
			 * @tparam To 変換先整数型。
			 * @tparam From 変換元整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename To, typename From>
			struct InRangeImpl<To, From, false, true>
			{
				static constexpr bool Check(From value) noexcept
				{
					return value >= From{0} &&
						static_cast<std::uintmax_t>(value) <=
						static_cast<std::uintmax_t>(std::numeric_limits<To>::max());
				}
			};

			/**
			 * @brief unsigned差分による絶対差実装。
			 * @tparam T 対象整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct AbsDiffImpl
			{
				/**
				 * @brief 2値の差の絶対値をunsigned整数で取得。
				 * @param[in] a 1つ目の値。
				 * @param[in] b 2つ目の値。
				 * @retval value `a`と`b`の数学的な差の絶対値。
				 * @pre `T`はboolとcharacter型を除くintegral型。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr typename MakeUnsigned<T>::Type Run(T a, T b) noexcept
				{
					return a >= b ? static_cast<typename MakeUnsigned<T>::Type>(
										static_cast<typename MakeUnsigned<T>::Type>(a) -
										static_cast<typename MakeUnsigned<T>::Type>(b))
								  : static_cast<typename MakeUnsigned<T>::Type>(
										static_cast<typename MakeUnsigned<T>::Type>(b) -
										static_cast<typename MakeUnsigned<T>::Type>(a));
				}
			};

			/**
			 * @brief checked add のsigned/unsigned別実装。
			 * @tparam T 対象整数型。
			 * @tparam IsSigned `T`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, bool IsSigned>
			struct CheckedAddImpl;

			/**
			 * @brief signed整数checked add実装。
			 * @tparam T 対象signed整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedAddImpl<T, true>
			{
				/**
				 * @brief signed整数加算のoverflow検出。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @param[out] out 成功時の加算結果。
				 * @retval true 成功。
				 * @retval false overflow。
				 * @pre `T`はsigned integral型。
				 * @post 成功時のみ`out`を変更。失敗時は`out`を変更しない。
				 */
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto max_value = std::numeric_limits<T>::max();
					const auto min_value = std::numeric_limits<T>::min();
					const auto zero = T{0};

					const auto positive_overflows = b > zero && a > static_cast<T>(max_value - b);
					if (positive_overflows)
					{
						return false;
					}

					const auto negative_overflows = b < zero && a < static_cast<T>(min_value - b);
					if (negative_overflows)
					{
						return false;
					}

					out = static_cast<T>(a + b);
					return true;
				}
			};

			/**
			 * @brief unsigned整数checked add実装。
			 * @tparam T 対象unsigned整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedAddImpl<T, false>
			{
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto max_value = std::numeric_limits<T>::max();
					const auto overflows = a > static_cast<T>(max_value - b);
					if (overflows)
					{
						return false;
					}

					out = static_cast<T>(a + b);
					return true;
				}
			};

			/**
			 * @brief checked sub のsigned/unsigned別実装。
			 * @tparam T 対象整数型。
			 * @tparam IsSigned `T`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, bool IsSigned>
			struct CheckedSubImpl;

			/**
			 * @brief signed整数checked sub実装。
			 * @tparam T 対象signed整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedSubImpl<T, true>
			{
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto max_value = std::numeric_limits<T>::max();
					const auto min_value = std::numeric_limits<T>::min();
					const auto zero = T{0};

					const auto positive_overflows = b > zero && a < static_cast<T>(min_value + b);
					if (positive_overflows)
					{
						return false;
					}

					const auto negative_overflows = b < zero && a > static_cast<T>(max_value + b);
					if (negative_overflows)
					{
						return false;
					}

					out = static_cast<T>(a - b);
					return true;
				}
			};

			/**
			 * @brief unsigned整数checked sub実装。
			 * @tparam T 対象unsigned整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedSubImpl<T, false>
			{
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto underflows = a < b;
					if (underflows)
					{
						return false;
					}

					out = static_cast<T>(a - b);
					return true;
				}
			};

			/**
			 * @brief checked mul のsigned/unsigned別実装。
			 * @tparam T 対象整数型。
			 * @tparam IsSigned `T`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, bool IsSigned>
			struct CheckedMulImpl;

			/**
			 * @brief signed整数checked mul実装。
			 * @tparam T 対象signed整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedMulImpl<T, true>
			{
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto max_value = std::numeric_limits<T>::max();
					const auto min_value = std::numeric_limits<T>::min();
					const auto zero = T{0};

					const auto has_zero = a == zero || b == zero;
					if (has_zero)
					{
						out = zero;
						return true;
					}

					const auto a_is_positive = a > zero;
					if (a_is_positive)
					{
						const auto b_is_positive = b > zero;
						if (b_is_positive)
						{
							const auto overflows = a > static_cast<T>(max_value / b);
							if (overflows)
							{
								return false;
							}
						}
						else
						{
							const auto overflows = b < static_cast<T>(min_value / a);
							if (overflows)
							{
								return false;
							}
						}
					}
					else
					{
						const auto b_is_positive = b > zero;
						if (b_is_positive)
						{
							const auto overflows = a < static_cast<T>(min_value / b);
							if (overflows)
							{
								return false;
							}
						}
						else
						{
							const auto overflows = b < static_cast<T>(max_value / a);
							if (overflows)
							{
								return false;
							}
						}
					}

					out = static_cast<T>(a * b);
					return true;
				}
			};

			/**
			 * @brief unsigned整数checked mul実装。
			 * @tparam T 対象unsigned整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct CheckedMulImpl<T, false>
			{
				static bool Run(T a, T b, T& out) noexcept
				{
					const auto max_value = std::numeric_limits<T>::max();
					const auto zero = T{0};

					const auto has_zero = a == zero || b == zero;
					if (has_zero)
					{
						out = zero;
						return true;
					}

					const auto overflows = b > static_cast<T>(max_value / a);
					if (overflows)
					{
						return false;
					}

					out = static_cast<T>(a * b);
					return true;
				}
			};

			/**
			 * @brief saturating add のsigned/unsigned別実装。
			 * @tparam T 対象整数型。
			 * @tparam IsSigned `T`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, bool IsSigned>
			struct SaturatingAddImpl;

			/**
			 * @brief signed整数saturating add実装。
			 * @tparam T 対象signed整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct SaturatingAddImpl<T, true>
			{
				/**
				 * @brief signed整数型の上限値取得。
				 * @retval value `T`の上限値。
				 * @pre `T`はsigned integral型。
				 * @post 外部状態の変更なし。
				 */
				static constexpr T MaxValue() noexcept
				{
					return std::numeric_limits<T>::max();
				}

				/**
				 * @brief signed整数加算の上限overflow判定。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @retval true 上限overflow。
				 * @retval false 上限overflowなし。
				 * @pre `T`はsigned integral型。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr bool AddOverflowsUpper(T a, T b) noexcept
				{
					return b > T{0} && a > static_cast<T>(MaxValue() - b);
				}

				/**
				 * @brief signed整数加算を下限側で飽和させる。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @retval value 加算結果、または下限へ飽和した値。
				 * @pre `T`はsigned integral型。上限overflowは呼び出し側で判定済み。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr T AddOrLowerBound(T a, T b) noexcept
				{
					return b < T{0} && a < static_cast<T>(std::numeric_limits<T>::min() - b)
						? std::numeric_limits<T>::min()
						: static_cast<T>(a + b);
				}

				/**
				 * @brief signed整数加算を上下限で飽和させる。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @retval value 加算結果、または上下限へ飽和した値。
				 * @pre `T`はsigned integral型。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr T Run(T a, T b) noexcept
				{
					return AddOverflowsUpper(a, b) ? MaxValue() : AddOrLowerBound(a, b);
				}
			};

			/**
			 * @brief unsigned整数saturating add実装。
			 * @tparam T 対象unsigned整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct SaturatingAddImpl<T, false>
			{
				static constexpr T Run(T a, T b) noexcept
				{
					return a > static_cast<T>(std::numeric_limits<T>::max() - b)
						? std::numeric_limits<T>::max()
						: static_cast<T>(a + b);
				}
			};

			/**
			 * @brief saturating sub のsigned/unsigned別実装。
			 * @tparam T 対象整数型。
			 * @tparam IsSigned `T`がsignedか。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T, bool IsSigned>
			struct SaturatingSubImpl;

			/**
			 * @brief signed整数saturating sub実装。
			 * @tparam T 対象signed整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct SaturatingSubImpl<T, true>
			{
				/**
				 * @brief signed整数型の下限値取得。
				 * @retval value `T`の下限値。
				 * @pre `T`はsigned integral型。
				 * @post 外部状態の変更なし。
				 */
				static constexpr T MinValue() noexcept
				{
					return std::numeric_limits<T>::min();
				}

				/**
				 * @brief signed整数減算の下限overflow判定。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @retval true 下限overflow。
				 * @retval false 下限overflowなし。
				 * @pre `T`はsigned integral型。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr bool SubOverflowsLower(T a, T b) noexcept
				{
					return b > T{0} && a < static_cast<T>(MinValue() + b);
				}

				/**
				 * @brief signed整数減算を上限側で飽和させる。
				 * @param[in] a 左辺値。
				 * @param[in] b 右辺値。
				 * @retval value 減算結果、または上限へ飽和した値。
				 * @pre `T`はsigned integral型。下限overflowは呼び出し側で判定済み。
				 * @post 引数と外部状態の変更なし。
				 */
				static constexpr T SubOrUpperBound(T a, T b) noexcept
				{
					return b < T{0} && a > static_cast<T>(std::numeric_limits<T>::max() + b)
						? std::numeric_limits<T>::max()
						: static_cast<T>(a - b);
				}

				static constexpr T Run(T a, T b) noexcept
				{
					return SubOverflowsLower(a, b) ? MinValue() : SubOrUpperBound(a, b);
				}
			};

			/**
			 * @brief unsigned整数saturating sub実装。
			 * @tparam T 対象unsigned整数型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct SaturatingSubImpl<T, false>
			{
				static constexpr T Run(T a, T b) noexcept
				{
					return a < b ? T{0} : static_cast<T>(a - b);
				}
			};

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename To, typename From>
		constexpr bool InRange(From value) noexcept
		{
			static_assert(detail::IsSupportedIntegral<To>::value,
						  "ket::numeric::InRange requires an integral destination type except "
						  "bool and character types.");
			static_assert(detail::IsSupportedIntegral<From>::value,
						  "ket::numeric::InRange requires an integral source type except bool "
						  "and character types.");

			return detail::InRangeImpl<To,
									   From,
									   detail::IsSigned<To>::value,
									   detail::IsSigned<From>::value>::Check(value);
		}

		template <typename T>
		constexpr T Clamp(T value, T min_value, T max_value) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::Clamp requires an integral type except bool and "
						  "character types.");

			return value < min_value ? min_value : detail::ClampUpper(value, max_value);
		}

		// NOLINTBEGIN(modernize-type-traits)
		template <typename T>
		constexpr typename std::make_unsigned<T>::type AbsDiff(T a, T b) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::AbsDiff requires an integral type except bool and "
						  "character types.");

			return detail::AbsDiffImpl<T>::Run(a, b);
		}
		// NOLINTEND(modernize-type-traits)

		template <typename T>
		bool TryDivideRoundUp(T value, T divisor, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::value,
						  "ket::numeric::TryDivideRoundUp requires an unsigned integral type "
						  "except bool and character types.");

			const auto zero = T{0};
			const auto divisor_is_zero = divisor == zero;
			if (divisor_is_zero)
			{
				return false;
			}

			const auto quotient = static_cast<T>(value / divisor);
			const auto remainder = static_cast<T>(value % divisor);
			const auto has_remainder = remainder != zero;
			if (!has_remainder)
			{
				out = quotient;
				return true;
			}

			out = static_cast<T>(quotient + T{1});
			return true;
		}

		template <typename T>
		bool TryAlignUp(T value, T alignment, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::value,
						  "ket::numeric::TryAlignUp requires an unsigned integral type except "
						  "bool and character types.");

			const auto zero = T{0};
			const auto alignment_is_zero = alignment == zero;
			if (alignment_is_zero)
			{
				return false;
			}

			const auto remainder = static_cast<T>(value % alignment);
			const auto already_aligned = remainder == zero;
			if (already_aligned)
			{
				out = value;
				return true;
			}

			const auto max_value = std::numeric_limits<T>::max();
			const auto adjustment = static_cast<T>(alignment - remainder);
			const auto overflows = value > static_cast<T>(max_value - adjustment);
			if (overflows)
			{
				return false;
			}

			out = static_cast<T>(value + adjustment);
			return true;
		}

		template <typename T>
		bool TryAlignDown(T value, T alignment, T& out) noexcept
		{
			static_assert(detail::IsSupportedUnsignedIntegral<T>::value,
						  "ket::numeric::TryAlignDown requires an unsigned integral type except "
						  "bool and character types.");

			const auto zero = T{0};
			const auto alignment_is_zero = alignment == zero;
			if (alignment_is_zero)
			{
				return false;
			}

			const auto remainder = static_cast<T>(value % alignment);
			out = static_cast<T>(value - remainder);
			return true;
		}

		template <typename T>
		bool TryCheckedAdd(T a, T b, T& out) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::TryCheckedAdd requires an integral type except bool "
						  "and character types.");

			return detail::CheckedAddImpl<T, detail::IsSigned<T>::value>::Run(a, b, out);
		}

		template <typename T>
		bool TryCheckedSub(T a, T b, T& out) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::TryCheckedSub requires an integral type except bool "
						  "and character types.");

			return detail::CheckedSubImpl<T, detail::IsSigned<T>::value>::Run(a, b, out);
		}

		template <typename T>
		bool TryCheckedMul(T a, T b, T& out) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::TryCheckedMul requires an integral type except bool "
						  "and character types.");

			return detail::CheckedMulImpl<T, detail::IsSigned<T>::value>::Run(a, b, out);
		}

		template <typename T>
		constexpr T SaturatingAdd(T a, T b) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::SaturatingAdd requires an integral type except bool "
						  "and character types.");

			return detail::SaturatingAddImpl<T, detail::IsSigned<T>::value>::Run(a, b);
		}

		template <typename T>
		constexpr T SaturatingSub(T a, T b) noexcept
		{
			static_assert(detail::IsSupportedIntegral<T>::value,
						  "ket::numeric::SaturatingSub requires an integral type except bool "
						  "and character types.");

			return detail::SaturatingSubImpl<T, detail::IsSigned<T>::value>::Run(a, b);
		}

		template <typename To, typename From>
		bool TryCheckedCast(From value, To& out) noexcept
		{
			static_assert(detail::IsSupportedIntegral<To>::value,
						  "ket::numeric::TryCheckedCast requires an integral destination type "
						  "except bool and character types.");
			static_assert(detail::IsSupportedIntegral<From>::value,
						  "ket::numeric::TryCheckedCast requires an integral source type except "
						  "bool and character types.");

			const auto value_is_in_range = InRange<To>(value);
			if (!value_is_in_range)
			{
				return false;
			}

			out = static_cast<To>(value);
			return true;
		}

	} // namespace numeric

} // namespace ket
