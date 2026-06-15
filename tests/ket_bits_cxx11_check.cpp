#include <cstdint>

#include "ket_bits.h"

static_assert(ket::bits::IsNibble(static_cast<std::uint8_t>(0x0FU)), "IsNibble is C++11 constexpr");
static_assert(!ket::bits::IsNibble(static_cast<std::uint8_t>(0x10U)),
			  "IsNibble rejects out-of-range values in C++11 constexpr");
static_assert(ket::bits::HighNibble(static_cast<std::uint8_t>(0xABU)) ==
				  static_cast<std::uint8_t>(0x0AU),
			  "HighNibble is C++11 constexpr");
static_assert(ket::bits::LowNibble(static_cast<std::uint8_t>(0xABU)) ==
				  static_cast<std::uint8_t>(0x0BU),
			  "LowNibble is C++11 constexpr");
static_assert(ket::bits::BitWidth<std::uint8_t>() == 8U, "BitWidth is C++11 constexpr");
static_assert(ket::bits::HasBit<std::uint8_t>(static_cast<std::uint8_t>(0x80U), 7U),
			  "HasBit is C++11 constexpr");
static_assert(!ket::bits::HasBit<std::uint8_t>(static_cast<std::uint8_t>(0x80U), 8U),
			  "HasBit rejects out-of-range index in C++11 constexpr");
static_assert(ket::bits::PopCount<std::uint8_t>(static_cast<std::uint8_t>(0xFFU)) == 8U,
			  "PopCount is C++11 constexpr");
static_assert(ket::bits::IsPowerOfTwo<std::uint8_t>(static_cast<std::uint8_t>(0x80U)),
			  "IsPowerOfTwo is C++11 constexpr");
static_assert(ket::bits::Rotl<std::uint8_t>(static_cast<std::uint8_t>(0x81U), 9U) ==
				  static_cast<std::uint8_t>(0x03U),
			  "Rotl normalizes count in C++11 constexpr");
static_assert(ket::bits::Rotr<std::uint8_t>(static_cast<std::uint8_t>(0x81U), 9U) ==
				  static_cast<std::uint8_t>(0xC0U),
			  "Rotr normalizes count in C++11 constexpr");
static_assert(!ket::bits::detail::IsSupportedUnsignedIntegral<int>::kValue,
			  "signed integral types are rejected");
static_assert(!ket::bits::detail::IsSupportedUnsignedIntegral<bool>::kValue, "bool is rejected");
static_assert(ket::bits::detail::IsSupportedUnsignedIntegral<std::uint64_t>::kValue,
			  "unsigned integral types are accepted");
