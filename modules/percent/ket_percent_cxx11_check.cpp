// clang-format off
#include "ket_percent.h"
// clang-format on

namespace
{
	constexpr ket::percent::Percent kDefaultPercent;

	static_assert(kDefaultPercent.BasisPoints() == 0U, "default percent is C++11 constexpr");
	static_assert(kDefaultPercent.ToPercent() == 0.0, "percent conversion is C++11 constexpr");
	static_assert(kDefaultPercent.ToRatio() == 0.0, "ratio conversion is C++11 constexpr");
	static_assert(kDefaultPercent == ket::percent::Percent{}, "comparison is C++11 constexpr");

} // namespace

void KetPercentCxx11Check()
{
	ket::percent::Percent value;

	const bool from_basis_points = ket::percent::Percent::TryFromBasisPoints(10000U, value);
	const bool from_percent = ket::percent::Percent::TryFromPercent(12.345, value);
	const bool from_ratio = ket::percent::Percent::TryFromRatio(0.12345, value);
	const auto clamped = ket::percent::Percent::FromPercentClamped(120.0);

	static_cast<void>(from_basis_points);
	static_cast<void>(from_percent);
	static_cast<void>(from_ratio);
	static_cast<void>(clamped);
}
