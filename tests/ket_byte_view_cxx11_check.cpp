#include <cstdint>

#include "ket_byte_view.h"

namespace ket_byte_view_cxx11_check
{
	constexpr ket::byte_view::View kEmptyView;
	constexpr ket::byte_view::MutableView kEmptyMutableView;

	static_assert(kEmptyView.Data() == nullptr, "default View data is constexpr null");
	static_assert(kEmptyView.Size() == 0U, "default View size is constexpr zero");
	static_assert(kEmptyView.Empty(), "default View empty state is constexpr");
	static_assert(kEmptyMutableView.Data() == nullptr,
				  "default MutableView data is constexpr null");
	static_assert(kEmptyMutableView.Size() == 0U, "default MutableView size is constexpr zero");
	static_assert(kEmptyMutableView.Empty(), "default MutableView empty state is constexpr");

	void Run()
	{
		std::uint8_t data[3] = {0x01U, 0x02U, 0x03U};

		const ket::byte_view::View view(data, 3U);
		std::uint8_t read_value = 0U;
		ket::byte_view::View slice;
		const bool read_ok = view.TryAt(1U, read_value);
		const bool slice_ok = view.TrySlice(1U, 2U, slice);

		ket::byte_view::MutableView mutable_view(data, 3U);
		ket::byte_view::MutableView mutable_slice;
		const bool set_ok = mutable_view.TrySet(2U, 0x04U);
		const bool mutable_slice_ok = mutable_view.TrySlice(0U, 1U, mutable_slice);

		(void)read_ok;
		(void)slice_ok;
		(void)set_ok;
		(void)mutable_slice_ok;
		(void)read_value;
	}

} // namespace ket_byte_view_cxx11_check
