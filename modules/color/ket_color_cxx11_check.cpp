// clang-format off
#include "ket_color.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
// clang-format on

namespace ket_color_cxx11_check
{
	void CheckParseAndFormat()
	{
		ket::color::Rgb color;
		const auto parsed = ket::color::TryParse("#0aFF10", 7U, color);
		if (!parsed)
		{
			return;
		}

		const auto formatted = ket::color::Format(color, ket::color::FormatOptions(false));
		static_cast<void>(formatted);
	}

	static_assert(ket::color::Rgb().r == 0U, "Rgb default constructor must create black.");
	static_assert(ket::color::Rgb(static_cast<std::uint8_t>(0x0aU),
								  static_cast<std::uint8_t>(0xffU),
								  static_cast<std::uint8_t>(0x10U))
						  .g == 0xffU,
				  "Rgb channel constructor must be constexpr in C++11.");
	static_assert(ket::color::FormatOptions().with_hash,
				  "FormatOptions default constructor must keep hash enabled.");
	static_assert(!ket::color::FormatOptions(false).with_hash,
				  "FormatOptions value constructor must be constexpr in C++11.");
	static_assert(ket::color::Rgb() == ket::color::Rgb(),
				  "Rgb equality must be constexpr in C++11.");
	static_assert(ket::color::Rgb() !=
					  ket::color::Rgb(static_cast<std::uint8_t>(1U),
									  static_cast<std::uint8_t>(0U),
									  static_cast<std::uint8_t>(0U)),
				  "Rgb inequality must be constexpr in C++11.");
	static_assert(std::is_same<decltype(ket::color::Rgb().r), std::uint8_t>::value,
				  "Rgb::r must keep the documented channel type.");
	static_assert(noexcept(ket::color::Rgb() == ket::color::Rgb()),
				  "Rgb equality must stay noexcept.");
	static_assert(noexcept(ket::color::Rgb() != ket::color::Rgb()),
				  "Rgb inequality must stay noexcept.");
	static_assert(noexcept(ket::color::TryParse(std::declval<const char*>(),
												std::declval<std::size_t>(),
												std::declval<ket::color::Rgb&>())),
				  "TryParse pointer overload must stay noexcept.");
	static_assert(noexcept(ket::color::TryParse(std::declval<const std::string&>(),
												std::declval<ket::color::Rgb&>())),
				  "TryParse must stay noexcept.");
	static_assert(std::is_same<decltype(ket::color::Format(ket::color::Rgb())), std::string>::value,
				  "Format must return std::string.");

} // namespace ket_color_cxx11_check
