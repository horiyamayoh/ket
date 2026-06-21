// clang-format off
#include "ket_math.h"

#include <cstdint>
#include <limits>
// clang-format on

namespace
{
	static_assert(ket::math::Lerp(10.0, 20.0, 0.25) == 12.5, "Lerp is C++11 constexpr");
	static_assert(ket::math::Lerp(10.0, 20.0, 1.5) == 25.0, "Lerp extrapolates in C++11");
	static_assert(ket::math::NearlyEqual(1.0, 1.001, 0.01), "NearlyEqual is C++11 constexpr");
	static_assert(!ket::math::NearlyEqual(1.0, 1.0, 0.0),
				  "NearlyEqual rejects zero epsilon in C++11 constexpr");
	static_assert(!ket::math::NearlyEqual(1.0, 2.0, std::numeric_limits<double>::infinity()),
				  "NearlyEqual rejects infinite epsilon in C++11 constexpr");
	static_assert(ket::math::NearlyEqual(ket::math::ToDegrees(ket::math::ToRadians(90.0)),
										 90.0,
										 0.000000000001),
				  "angle conversion is C++11 constexpr");
	static_assert(ket::math::BytesToKiB(std::uint64_t{2048U}) == 2.0,
				  "BytesToKiB is C++11 constexpr");
	static_assert(ket::math::BytesToMiB(std::uint64_t{2097152U}) == 2.0,
				  "BytesToMiB is C++11 constexpr");

} // namespace

void KetMathCxx11Check()
{
	std::uint64_t bytes = 0U;

	const bool kib_ok = ket::math::TryBytesFromKiB(4U, bytes);
	const bool mib_ok = ket::math::TryBytesFromMiB(2U, bytes);

	static_cast<void>(kib_ok);
	static_cast<void>(mib_ok);
	static_cast<void>(bytes);
}
