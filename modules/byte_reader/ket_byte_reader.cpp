#include "ket_byte_reader.h"

#include <cstddef>
#include <cstdint>

namespace
{
	constexpr bool IsValidReader(const std::uint8_t* data, std::size_t size) noexcept
	{
		return data != nullptr || size == 0U;
	}

	bool CanConsume(const std::uint8_t* data,
					std::size_t size,
					std::size_t offset,
					std::size_t requested_size) noexcept
	{
		const auto reader_is_valid = IsValidReader(data, size);
		if (!reader_is_valid)
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

	std::uint16_t BuildBe16(const std::uint8_t* data) noexcept
	{
		return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[0]) << 8U) |
										  static_cast<std::uint16_t>(data[1]));
	}

	std::uint32_t BuildBe32(const std::uint8_t* data) noexcept
	{
		return (static_cast<std::uint32_t>(data[0]) << 24U) |
			(static_cast<std::uint32_t>(data[1]) << 16U) |
			(static_cast<std::uint32_t>(data[2]) << 8U) | static_cast<std::uint32_t>(data[3]);
	}

	std::uint16_t BuildLe16(const std::uint8_t* data) noexcept
	{
		return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[1]) << 8U) |
										  static_cast<std::uint16_t>(data[0]));
	}

	std::uint32_t BuildLe32(const std::uint8_t* data) noexcept
	{
		return (static_cast<std::uint32_t>(data[3]) << 24U) |
			(static_cast<std::uint32_t>(data[2]) << 16U) |
			(static_cast<std::uint32_t>(data[1]) << 8U) | static_cast<std::uint32_t>(data[0]);
	}

} // namespace

namespace ket
{
	namespace byte_reader
	{
		Reader::Reader(const std::uint8_t* data, std::size_t size) noexcept
			: data_(data), size_(size)
		{
		}

		std::size_t Reader::Size() const noexcept
		{
			return size_;
		}

		std::size_t Reader::Offset() const noexcept
		{
			return offset_;
		}

		std::size_t Reader::Remaining() const noexcept
		{
			const auto can_consume_zero = CanConsume(data_, size_, offset_, 0U);
			if (!can_consume_zero)
			{
				return 0U;
			}

			return size_ - offset_;
		}

		bool Reader::Empty() const noexcept
		{
			const auto can_consume_zero = CanConsume(data_, size_, offset_, 0U);
			if (!can_consume_zero)
			{
				return false;
			}

			return offset_ == size_;
		}

		bool Reader::Skip(std::size_t size) noexcept
		{
			const auto can_consume = CanConsume(data_, size_, offset_, size);
			if (!can_consume)
			{
				return false;
			}

			offset_ += size;
			return true;
		}

		bool Reader::ReadU8(std::uint8_t& out) noexcept
		{
			const std::uint8_t* data = nullptr;
			const auto read_succeeded = ReadBytes(1U, data);
			if (!read_succeeded)
			{
				return false;
			}

			out = data[0];
			return true;
		}

		bool Reader::ReadBe16(std::uint16_t& out) noexcept
		{
			const std::uint8_t* data = nullptr;
			const auto read_succeeded = ReadBytes(2U, data);
			if (!read_succeeded)
			{
				return false;
			}

			out = BuildBe16(data);
			return true;
		}

		bool Reader::ReadBe32(std::uint32_t& out) noexcept
		{
			const std::uint8_t* data = nullptr;
			const auto read_succeeded = ReadBytes(4U, data);
			if (!read_succeeded)
			{
				return false;
			}

			out = BuildBe32(data);
			return true;
		}

		bool Reader::ReadLe16(std::uint16_t& out) noexcept
		{
			const std::uint8_t* data = nullptr;
			const auto read_succeeded = ReadBytes(2U, data);
			if (!read_succeeded)
			{
				return false;
			}

			out = BuildLe16(data);
			return true;
		}

		bool Reader::ReadLe32(std::uint32_t& out) noexcept
		{
			const std::uint8_t* data = nullptr;
			const auto read_succeeded = ReadBytes(4U, data);
			if (!read_succeeded)
			{
				return false;
			}

			out = BuildLe32(data);
			return true;
		}

		bool Reader::ReadBytes(std::size_t size, const std::uint8_t*& out_data) noexcept
		{
			const auto can_consume = CanConsume(data_, size_, offset_, size);
			if (!can_consume)
			{
				return false;
			}

			if (data_ == nullptr)
			{
				out_data = nullptr;
			}
			else
			{
				out_data = data_ + offset_;
			}

			offset_ += size;
			return true;
		}

	} // namespace byte_reader

} // namespace ket
