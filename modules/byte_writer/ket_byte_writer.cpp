#include "ket_byte_writer.h"

#include <cstdint>
#include <cstring>

namespace
{
	constexpr bool IsValidWriter(const std::uint8_t* data, std::size_t size) noexcept
	{
		return data != nullptr || size == 0U;
	}

	bool CanWrite(const std::uint8_t* data,
				  std::size_t size,
				  std::size_t offset,
				  std::size_t requested_size) noexcept
	{
		const auto writer_is_valid = IsValidWriter(data, size);
		if (!writer_is_valid)
		{
			return false;
		}

		const auto offset_is_valid = offset <= size;
		if (!offset_is_valid)
		{
			return false;
		}

		const auto remaining = size - offset;
		return requested_size <= remaining;
	}

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
			const auto can_write_zero = CanWrite(data_, size_, offset_, 0U);
			if (!can_write_zero)
			{
				return 0U;
			}

			return size_ - offset_;
		}

		bool Writer::Full() const noexcept
		{
			const auto can_write_zero = CanWrite(data_, size_, offset_, 0U);
			if (!can_write_zero)
			{
				return false;
			}

			return offset_ == size_;
		}

		bool Writer::Skip(std::size_t size) noexcept
		{
			const auto can_write = CanWrite(data_, size_, offset_, size);
			if (!can_write)
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
			const auto can_write = CanWrite(data_, size_, offset_, size);
			if (!can_write)
			{
				return false;
			}

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

			std::memmove(data_ + offset_, data, size);
			offset_ += size;
			return true;
		}

	} // namespace byte_writer

} // namespace ket
