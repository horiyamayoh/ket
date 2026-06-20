#include <limits>

#include "ket_numeric.h"

namespace
{
	static_assert(ket::numeric::detail::IsSupportedIntegral<int>::value,
				  "int is a supported numeric type");
	static_assert(ket::numeric::detail::IsSupportedUnsignedIntegral<unsigned>::value,
				  "unsigned is a supported unsigned numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<bool>::value,
				  "bool is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<char>::value,
				  "char is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<signed char>::value,
				  "signed char is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<unsigned char>::value,
				  "unsigned char is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<wchar_t>::value,
				  "wchar_t is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<char16_t>::value,
				  "char16_t is not a supported numeric type");
	static_assert(!ket::numeric::detail::IsSupportedIntegral<char32_t>::value,
				  "char32_t is not a supported numeric type");

	static_assert(ket::numeric::InRange<int>(0U), "InRange is constexpr in C++11");
	static_assert(!ket::numeric::InRange<unsigned>(-1), "negative value is outside unsigned range");
	static_assert(ket::numeric::Clamp(10, 0, 7) == 7, "Clamp is constexpr in C++11");
	static_assert(ket::numeric::AbsDiff(-3, 2) == 5U, "AbsDiff is constexpr in C++11");
	static_assert(ket::numeric::AbsDiff(std::numeric_limits<int>::min(),
										std::numeric_limits<int>::max()) ==
					  std::numeric_limits<unsigned>::max(),
				  "AbsDiff covers the signed int span in C++11");
	static_assert(ket::numeric::SaturatingAdd(std::numeric_limits<int>::max(), 1) ==
					  std::numeric_limits<int>::max(),
				  "SaturatingAdd is constexpr in C++11");
	static_assert(ket::numeric::SaturatingSub(std::numeric_limits<int>::min(), 1) ==
					  std::numeric_limits<int>::min(),
				  "SaturatingSub is constexpr in C++11");

} // namespace

void KetNumericCxx11Check()
{
	unsigned unsigned_out = 0U;
	int int_out = 0;
	short short_out = 0;

	const bool divided = ket::numeric::TryDivideRoundUp(7U, 3U, unsigned_out);
	const bool aligned_up = ket::numeric::TryAlignUp(7U, 4U, unsigned_out);
	const bool aligned_down = ket::numeric::TryAlignDown(7U, 4U, unsigned_out);
	const bool added = ket::numeric::TryAdd(1, 2, int_out);
	const bool subtracted = ket::numeric::TrySub(3, 1, int_out);
	const bool multiplied = ket::numeric::TryMul(3, 4, int_out);
	const bool casted = ket::numeric::TryCast(42, short_out);

	static_cast<void>(divided);
	static_cast<void>(aligned_up);
	static_cast<void>(aligned_down);
	static_cast<void>(added);
	static_cast<void>(subtracted);
	static_cast<void>(multiplied);
	static_cast<void>(casted);
	static_cast<void>(unsigned_out);
	static_cast<void>(int_out);
	static_cast<void>(short_out);
}
