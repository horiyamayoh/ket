#include <cstdint>

#include "ket_memory.h"

namespace
{
	struct SampleObject
	{
		std::uint32_t value;
		std::uint8_t tag;
	};

} // namespace

bool KetMemoryCxx11CompileCheck()
{
	alignas(8) unsigned char buffer[16] = {};
	const void* aligned_up = nullptr;
	const void* aligned_down = nullptr;
	void* mutable_aligned_up = nullptr;
	void* mutable_aligned_down = nullptr;
	unsigned char* typed_aligned_up = nullptr;
	unsigned char* typed_aligned_down = nullptr;
	const auto sample = SampleObject{std::uint32_t{0x12345678U}, std::uint8_t{0x9AU}};

	const auto aligned = ket::memory::IsAligned(buffer, 8U);
	const auto align_up_succeeded = ket::memory::TryAlignUp(buffer + 1U, 8U, aligned_up);
	const auto align_down_succeeded = ket::memory::TryAlignDown(buffer + 1U, 8U, aligned_down);
	const auto mutable_align_up_succeeded =
		ket::memory::TryAlignUp(static_cast<void*>(buffer + 1U), 8U, mutable_aligned_up);
	const auto mutable_align_down_succeeded =
		ket::memory::TryAlignDown(static_cast<void*>(buffer + 1U), 8U, mutable_aligned_down);
	const auto typed_align_up_succeeded =
		ket::memory::TryAlignUp(buffer + 1U, 8U, typed_aligned_up);
	const auto typed_align_down_succeeded =
		ket::memory::TryAlignDown(buffer + 1U, 8U, typed_aligned_down);
	ket::memory::Zero(buffer, sizeof(buffer));
	ket::memory::SecureZero(buffer, sizeof(buffer));
	const auto* const bytes = ket::memory::ObjectBytes(sample);
	const auto byte_size = ket::memory::ObjectByteSize(sample);

	return aligned && align_up_succeeded && align_down_succeeded && mutable_align_up_succeeded &&
		mutable_align_down_succeeded && typed_align_up_succeeded && typed_align_down_succeeded &&
		bytes != nullptr && byte_size == sizeof(sample);
}
