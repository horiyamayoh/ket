// clang-format off
#include "ket_tlv.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// clang-format on

namespace ket_tlv_cxx11_check
{
	void CheckEncodeAppendDecode()
	{
		const std::uint8_t value[] = {std::uint8_t{0xAAU}, std::uint8_t{0xBBU}};
		const auto record = ket::tlv::Encode(0x1234U, value, static_cast<std::size_t>(2U));

		std::vector<std::uint8_t> output;
		ket::tlv::Append(output, 0x1234U, value, static_cast<std::size_t>(2U));

		ket::tlv::DecodeResult decoded;
		const auto succeeded = ket::tlv::TryDecode(record.data(), record.size(), decoded);

		static_cast<void>(output);
		static_cast<void>(decoded);
		static_cast<void>(succeeded);
	}

	static_assert(std::is_same<decltype(ket::tlv::View().type), std::uint16_t>::value,
				  "View::type must keep the documented wire type.");
	static_assert(std::is_same<decltype(ket::tlv::View().value), const std::uint8_t*>::value,
				  "View::value must stay a non-owning pointer.");
	static_assert(std::is_same<decltype(ket::tlv::View().value_size), std::uint32_t>::value,
				  "View::value_size must keep the documented wire length type.");
	static_assert(std::is_same<decltype(ket::tlv::DecodeResult().consumed), std::size_t>::value,
				  "DecodeResult::consumed must stay a host size.");
	static_assert(noexcept(ket::tlv::TryDecode(static_cast<const std::uint8_t*>(nullptr),
											   std::size_t(),
											   std::declval<ket::tlv::DecodeResult&>())),
				  "TryDecode must stay noexcept.");

} // namespace ket_tlv_cxx11_check
