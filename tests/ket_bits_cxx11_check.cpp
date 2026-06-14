#include <cstdint>

#include "ket_bits.h"

static_assert(ket::IsNibble(0x0FU), "IsNibble is C++11 constexpr");
static_assert(!ket::IsNibble(0x10U), "IsNibble rejects out-of-range values");
static_assert(ket::HighNibble(0xABU) == 0x0AU, "HighNibble is C++11 constexpr");
static_assert(ket::LowNibble(0xABU) == 0x0BU, "LowNibble is C++11 constexpr");
static_assert(ket::BitWidth<std::uint8_t>() == 8U, "BitWidth is C++11 constexpr");
static_assert(ket::HasBit<std::uint8_t>(0x80U, 7U), "HasBit is C++11 constexpr");
static_assert(!ket::HasBit<std::uint8_t>(0x80U, 8U), "HasBit rejects out-of-range bits");
static_assert(ket::PopCount<std::uint8_t>(0x81U) == 2U, "PopCount is C++11 constexpr");
static_assert(ket::IsPowerOfTwo<std::uint8_t>(0x80U), "IsPowerOfTwo is C++11 constexpr");
static_assert(ket::Rotl<std::uint8_t>(0x81U, 1U) == 0x03U, "Rotl is C++11 constexpr");
static_assert(ket::Rotr<std::uint8_t>(0x81U, 1U) == 0xC0U, "Rotr is C++11 constexpr");

void KetBitsCxx11CompileCheck() noexcept
{
	std::uint8_t byte = 0U;
	const auto made_byte = ket::TryMakeByteFromNibbles(0x0AU, 0x0BU, byte);
	static_cast<void>(made_byte);

	std::uint8_t bit_value = 0U;
	const auto set_bit = ket::TrySetBit<std::uint8_t>(0U, 7U, bit_value);
	const auto clear_bit = ket::TryClearBit<std::uint8_t>(0xFFU, 7U, bit_value);
	const auto toggle_bit = ket::TryToggleBit<std::uint8_t>(0x03U, 1U, bit_value);
	static_cast<void>(set_bit);
	static_cast<void>(clear_bit);
	static_cast<void>(toggle_bit);

	std::uint8_t mask = 0U;
	const auto made_mask = ket::TryMask<std::uint8_t>(8U, mask);
	static_cast<void>(made_mask);
}
