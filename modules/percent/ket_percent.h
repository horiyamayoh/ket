#pragma once

/**
 * @file ket_percent.h
 * @brief 0〜100%をbasis pointsで保持する小さい値型。
 *
 * @details percent単位、ratio単位、basis points単位から同じ保持範囲へ正規化する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。丸め、NaN、Inf、
 * 範囲外入力の扱いをmodule内で固定する。
 *
 * @par プロジェクトへの適用方法
 * `ket_percent.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：percent小値型の範囲、丸め、NaN方針を局所的に固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::percent
 * 内部実装：ket::percent::detail
 */

#include <cmath>
#include <cstdint>

namespace ket
{
	namespace percent
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 0〜100%を1 basis point単位で保持する値型。
		 * @note 保持範囲は0から10000 basis points。1 basis pointは0.01%。
		 */
		class Percent
		{
		  public:
			/**
			 * @brief 0%を表す値の構築。
			 * @retval value 0%のPercent値。
			 * @pre なし。
			 * @post 構築後のBasisPoints()は0。
			 */
			constexpr Percent() noexcept;

			/**
			 * @brief basis points単位からPercent値を構築。
			 * @param[in] basis_points 入力basis points。0から10000のみ成功。
			 * @param[out] out 成功時に構築結果を書き込む出力先。
			 * @retval true `basis_points <= 10000`。
			 * @retval false `basis_points > 10000`。`out`は変更なし。
			 * @pre `out`は有効なPercentオブジェクト。
			 * @post 成功時のみ`out`を`basis_points`相当へ更新。失敗時は`out`を保持。
			 * @code
			 * ket::percent::Percent value;
			 * const auto ok = ket::percent::Percent::TryFromBasisPoints(1234U, value);
			 * // ok == true, value.ToPercent() == 12.34
			 * @endcode
			 */
			static bool TryFromBasisPoints(std::uint32_t basis_points, Percent& out) noexcept;

			/**
			 * @brief percent単位の小数からPercent値を構築。
			 * @param[in] percent 入力percent値。0.0から100.0のみ成功。
			 * @param[out] out 成功時に構築結果を書き込む出力先。
			 * @retval true 有限かつ0.0から100.0の入力。
			 * @retval false 範囲外、NaN、Inf。`out`は変更なし。
			 * @pre `out`は有効なPercentオブジェクト。
			 * @post 成功時のみ`out`をnearest basis pointへ丸めた値に更新。失敗時は`out`を保持。
			 * @note 丸めは非負値に対する`floor(value + 0.5)`相当。
			 * @code
			 * ket::percent::Percent value;
			 * const auto ok = ket::percent::Percent::TryFromPercent(12.345, value);
			 * // ok == true, value.BasisPoints() == 1235
			 * @endcode
			 */
			static bool TryFromPercent(double percent, Percent& out) noexcept;

			/**
			 * @brief ratio単位の小数からPercent値を構築。
			 * @param[in] ratio 入力ratio値。0.0から1.0のみ成功。
			 * @param[out] out 成功時に構築結果を書き込む出力先。
			 * @retval true 有限かつ0.0から1.0の入力。
			 * @retval false 範囲外、NaN、Inf。`out`は変更なし。
			 * @pre `out`は有効なPercentオブジェクト。
			 * @post 成功時のみ`out`をnearest basis pointへ丸めた値に更新。失敗時は`out`を保持。
			 * @note 丸めは非負値に対する`floor(value + 0.5)`相当。
			 * @code
			 * ket::percent::Percent value;
			 * const auto ok = ket::percent::Percent::TryFromRatio(0.12345, value);
			 * // ok == true, value.BasisPoints() == 1235
			 * @endcode
			 */
			static bool TryFromRatio(double ratio, Percent& out) noexcept;

			/**
			 * @brief percent単位の小数を範囲内へ丸め込んでPercent値を構築。
			 * @param[in] percent 入力percent値。
			 * @retval value 0.0から100.0へclamp後、nearest basis pointへ丸めたPercent値。
			 * @pre なし。NaNは0%、-Infは0%、+Infは100%として扱う。
			 * @post 引数と外部状態の変更なし。
			 * @note 有限の範囲外入力は0.0から100.0へclamp。
			 * @code
			 * const auto value = ket::percent::Percent::FromPercentClamped(120.0);
			 * // value.BasisPoints() == 10000
			 * @endcode
			 */
			static Percent FromPercentClamped(double percent) noexcept;

			/**
			 * @brief 保持中のbasis points取得。
			 * @retval value 0から10000のbasis points値。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr std::uint16_t BasisPoints() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 保持値のpercent単位変換。
			 * @retval value `BasisPoints() / 100.0`。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr double ToPercent() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 保持値のratio単位変換。
			 * @retval value `BasisPoints() / 10000.0`。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr double ToRatio() const noexcept; // NOLINT(modernize-use-nodiscard)

		  private:
			/**
			 * @brief 検証済みbasis pointsによる内部構築。
			 * @param[in] basis_points 保持するbasis points。
			 * @retval value `basis_points`を保持するPercent値。
			 * @pre `basis_points <= 10000`。
			 * @post 構築後のBasisPoints()は`basis_points`。
			 */
			explicit constexpr Percent(std::uint16_t basis_points) noexcept;

			std::uint16_t basis_points_;
		};

		/**
		 * @brief Percent値の等価比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 両辺のbasis pointsが一致。
		 * @retval false 両辺のbasis pointsが不一致。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator==(Percent a, Percent b) noexcept;

		/**
		 * @brief Percent値の非等価比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 両辺のbasis pointsが不一致。
		 * @retval false 両辺のbasis pointsが一致。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator!=(Percent a, Percent b) noexcept;

		/**
		 * @brief Percent値の小なり比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 左辺のbasis pointsが右辺より小さい。
		 * @retval false 左辺のbasis pointsが右辺以上。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator<(Percent a, Percent b) noexcept;

		/**
		 * @brief Percent値の小なり等価比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 左辺のbasis pointsが右辺以下。
		 * @retval false 左辺のbasis pointsが右辺より大きい。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator<=(Percent a, Percent b) noexcept;

		/**
		 * @brief Percent値の大なり比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 左辺のbasis pointsが右辺より大きい。
		 * @retval false 左辺のbasis pointsが右辺以下。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator>(Percent a, Percent b) noexcept;

		/**
		 * @brief Percent値の大なり等価比較。
		 * @param[in] a 左辺のPercent値。
		 * @param[in] b 右辺のPercent値。
		 * @retval true 左辺のbasis pointsが右辺以上。
		 * @retval false 左辺のbasis pointsが右辺より小さい。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr bool operator>=(Percent a, Percent b) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			constexpr std::uint16_t kMaxBasisPoints = 10000U;
			constexpr double kMaxPercent = 100.0;
			constexpr double kMaxRatio = 1.0;

			/**
			 * @brief 有限な浮動小数点値の判定。
			 * @param[in] value 判定対象の値。
			 * @retval true NaNでもInfでもない有限値。
			 * @retval false NaNまたはInf。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool IsFinite(double value) noexcept
			{
				return std::isfinite(value);
			}

			/**
			 * @brief 非負basis points値の最近傍整数丸め。
			 * @param[in] basis_points 丸め対象の非負basis points値。
			 * @retval value `floor(basis_points + 0.5)`相当の値。
			 * @pre `0.0 <= basis_points <= 10000.0`。
			 * @post 引数と外部状態の変更なし。
			 */
			inline std::uint16_t RoundNonNegativeBasisPoints(double basis_points) noexcept
			{
				return static_cast<std::uint16_t>(std::floor(basis_points + 0.5));
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr Percent::Percent() noexcept : basis_points_(0U) {}

		/**
		 * @brief 検証済みbasis pointsによる内部構築。
		 * @param[in] basis_points 保持するbasis points。
		 * @retval value `basis_points`を保持するPercent値。
		 * @pre `basis_points <= 10000`。
		 * @post 構築後のBasisPoints()は`basis_points`。
		 */
		constexpr Percent::Percent(std::uint16_t basis_points) noexcept
			: basis_points_(basis_points)
		{
		}

		/**
		 * @brief basis points単位からPercent値を構築。
		 * @param[in] basis_points 入力basis points。0から10000のみ成功。
		 * @param[out] out 成功時に構築結果を書き込む出力先。
		 * @retval true `basis_points <= 10000`。
		 * @retval false `basis_points > 10000`。`out`は変更なし。
		 * @pre `out`は有効なPercentオブジェクト。
		 * @post 成功時のみ`out`を`basis_points`相当へ更新。失敗時は`out`を保持。
		 */
		inline bool Percent::TryFromBasisPoints(std::uint32_t basis_points, Percent& out) noexcept
		{
			const auto basis_points_too_large = basis_points > detail::kMaxBasisPoints;
			if (basis_points_too_large)
			{
				return false;
			}

			out = Percent(static_cast<std::uint16_t>(basis_points));
			return true;
		}

		/**
		 * @brief percent単位の小数からPercent値を構築。
		 * @param[in] percent 入力percent値。0.0から100.0のみ成功。
		 * @param[out] out 成功時に構築結果を書き込む出力先。
		 * @retval true 有限かつ0.0から100.0の入力。
		 * @retval false 範囲外、NaN、Inf。`out`は変更なし。
		 * @pre `out`は有効なPercentオブジェクト。
		 * @post 成功時のみ`out`をnearest basis pointへ丸めた値に更新。失敗時は`out`を保持。
		 */
		inline bool Percent::TryFromPercent(double percent, Percent& out) noexcept
		{
			const auto percent_is_finite = detail::IsFinite(percent);
			const auto percent_is_in_range = percent >= 0.0 && percent <= detail::kMaxPercent;
			if (!percent_is_finite || !percent_is_in_range)
			{
				return false;
			}

			const auto basis_points = detail::RoundNonNegativeBasisPoints(percent * 100.0);
			out = Percent(basis_points);
			return true;
		}

		/**
		 * @brief ratio単位の小数からPercent値を構築。
		 * @param[in] ratio 入力ratio値。0.0から1.0のみ成功。
		 * @param[out] out 成功時に構築結果を書き込む出力先。
		 * @retval true 有限かつ0.0から1.0の入力。
		 * @retval false 範囲外、NaN、Inf。`out`は変更なし。
		 * @pre `out`は有効なPercentオブジェクト。
		 * @post 成功時のみ`out`をnearest basis pointへ丸めた値に更新。失敗時は`out`を保持。
		 */
		inline bool Percent::TryFromRatio(double ratio, Percent& out) noexcept
		{
			const auto ratio_is_finite = detail::IsFinite(ratio);
			const auto ratio_is_in_range = ratio >= 0.0 && ratio <= detail::kMaxRatio;
			if (!ratio_is_finite || !ratio_is_in_range)
			{
				return false;
			}

			const auto basis_points = detail::RoundNonNegativeBasisPoints(ratio * 10000.0);
			out = Percent(basis_points);
			return true;
		}

		/**
		 * @brief percent単位の小数を範囲内へ丸め込んでPercent値を構築。
		 * @param[in] percent 入力percent値。
		 * @retval value 0.0から100.0へclamp後、nearest basis pointへ丸めたPercent値。
		 * @pre なし。NaNは0%、-Infは0%、+Infは100%として扱う。
		 * @post 引数と外部状態の変更なし。
		 */
		inline Percent Percent::FromPercentClamped(double percent) noexcept
		{
			const auto percent_is_nan = std::isnan(percent);
			if (percent_is_nan)
			{
				return {};
			}

			const auto percent_is_zero_or_lower = percent <= 0.0;
			if (percent_is_zero_or_lower)
			{
				return {};
			}

			const auto percent_is_hundred_or_higher = percent >= detail::kMaxPercent;
			if (percent_is_hundred_or_higher)
			{
				return Percent(detail::kMaxBasisPoints);
			}

			const auto basis_points = detail::RoundNonNegativeBasisPoints(percent * 100.0);
			return Percent(basis_points);
		}

		/**
		 * @brief 保持中のbasis points取得。
		 * @retval value 0から10000のbasis points値。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr std::uint16_t Percent::BasisPoints() const noexcept
		{
			return basis_points_;
		}

		/**
		 * @brief 保持値のpercent単位変換。
		 * @retval value `BasisPoints() / 100.0`。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr double Percent::ToPercent() const noexcept
		{
			return static_cast<double>(BasisPoints()) / 100.0;
		}

		/**
		 * @brief 保持値のratio単位変換。
		 * @retval value `BasisPoints() / 10000.0`。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 */
		constexpr double Percent::ToRatio() const noexcept
		{
			return static_cast<double>(BasisPoints()) / 10000.0;
		}

		constexpr bool operator==(Percent a, Percent b) noexcept
		{
			return a.BasisPoints() == b.BasisPoints();
		}

		constexpr bool operator!=(Percent a, Percent b) noexcept
		{
			return !(a == b);
		}

		constexpr bool operator<(Percent a, Percent b) noexcept
		{
			return a.BasisPoints() < b.BasisPoints();
		}

		constexpr bool operator<=(Percent a, Percent b) noexcept
		{
			return !(b < a);
		}

		constexpr bool operator>(Percent a, Percent b) noexcept
		{
			return b < a;
		}

		constexpr bool operator>=(Percent a, Percent b) noexcept
		{
			return !(a < b);
		}

	} // namespace percent

} // namespace ket
