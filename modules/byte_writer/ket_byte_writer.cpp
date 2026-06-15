#include "ket_byte_writer.h"

#include <cstdint>
#include <cstring>

namespace
{
	constexpr std::uint8_t ByteAt(std::uint32_t value, unsigned int shift) noexcept
	{
		return static_cast<std::uint8_t>((value >> shift) & 0xFFU);
	}

} // namespace

namespace ket
{
	namespace byte_writer
	{
		Writer::Writer(std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size) {}

		std::size_t Writer::Size() const noexcept
		{
			return size_;
		}

		std::size_t Writer::Offset() const noexcept
		{
			return offset_;
		}

		std::size_t Writer::Remaining() const noexcept
		{
			const auto offset_is_past_size = offset_ > size_;
			if (offset_is_past_size)
			{
				return 0U;
			}

			return size_ - offset_;
		}

		bool Writer::Full() const noexcept
		{
			return Remaining() == 0U;
		}

		bool Writer::Skip(std::size_t size) noexcept
		{
			const auto requested_is_empty = size == 0U;
			if (requested_is_empty)
			{
				return true;
			}

			const auto destination_is_missing = data_ == nullptr;
			if (destination_is_missing)
			{
				return false;
			}

			const auto remaining = Remaining();
			const auto has_space = size <= remaining;
			if (!has_space)
			{
				return false;
			}

			offset_ += size;
			return true;
		}

		bool Writer::WriteU8(std::uint8_t value) noexcept
		{
			return WriteBytes(&value, 1U);
		}

		bool Writer::WriteBe16(std::uint16_t value) noexcept
		{
			const std::uint8_t bytes[] = {ByteAt(value, 8U), ByteAt(value, 0U)};

			return WriteBytes(bytes, sizeof(bytes));
		}

		bool Writer::WriteBe32(std::uint32_t value) noexcept
		{
			const std::uint8_t bytes[] = {
				ByteAt(value, 24U), ByteAt(value, 16U), ByteAt(value, 8U), ByteAt(value, 0U)};

			return WriteBytes(bytes, sizeof(bytes));
		}

		bool Writer::WriteLe16(std::uint16_t value) noexcept
		{
			const std::uint8_t bytes[] = {ByteAt(value, 0U), ByteAt(value, 8U)};

			return WriteBytes(bytes, sizeof(bytes));
		}

		bool Writer::WriteLe32(std::uint32_t value) noexcept
		{
			const std::uint8_t bytes[] = {
				ByteAt(value, 0U), ByteAt(value, 8U), ByteAt(value, 16U), ByteAt(value, 24U)};

			return WriteBytes(bytes, sizeof(bytes));
		}

		bool Writer::WriteBytes(const std::uint8_t* data, std::size_t size) noexcept
		{
			const auto requested_is_empty = size == 0U;
			if (requested_is_empty)
			{
				return true;
			}

			const auto source_is_missing = data == nullptr;
			if (source_is_missing)
			{
				return false;
			}

			const auto destination_is_missing = data_ == nullptr;
			if (destination_is_missing)
			{
				return false;
			}

			const auto remaining = Remaining();
			const auto has_space = size <= remaining;
			if (!has_space)
			{
				return false;
			}

			std::memmove(data_ + offset_, data, size);
			offset_ += size;
			return true;
		}

	} // namespace byte_writer

} // namespace ket
