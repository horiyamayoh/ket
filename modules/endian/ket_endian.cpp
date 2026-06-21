#include "ket_endian.h"

#include <cstddef>
#include <cstdint>

namespace
{
	constexpr std::size_t kUint16ByteSize = 2U;
	constexpr std::size_t kUint32ByteSize = 4U;
	constexpr std::size_t kUint64ByteSize = 8U;

	bool HasBytes(const std::uint8_t* data, std::size_t size, std::size_t required_size) noexcept
	{
		return data != nullptr && size >= required_size;
	}

	std::uint32_t ByteAt(const std::uint8_t* data, std::size_t index) noexcept
	{
		return static_cast<std::uint32_t>(data[index]);
	}

	std::uint64_t WideByteAt(const std::uint8_t* data, std::size_t index) noexcept
	{
		return static_cast<std::uint64_t>(data[index]);
	}

	std::uint8_t ByteFrom(std::uint16_t value, unsigned int shift) noexcept
	{
		const auto wide_value = static_cast<std::uint32_t>(value);
		return static_cast<std::uint8_t>((wide_value >> shift) & 0x000000FFU);
	}

	std::uint8_t ByteFrom(std::uint32_t value, unsigned int shift) noexcept
	{
		return static_cast<std::uint8_t>((value >> shift) & 0x000000FFU);
	}

	std::uint8_t ByteFrom(std::uint64_t value, unsigned int shift) noexcept
	{
		return static_cast<std::uint8_t>((value >> shift) & 0x00000000000000FFULL);
	}

} // namespace

namespace ket
{
	namespace endian
	{
		std::uint16_t LoadBe16(const std::uint8_t* data) noexcept
		{
			return static_cast<std::uint16_t>((ByteAt(data, 0U) << 8U) | ByteAt(data, 1U));
		}

		std::uint32_t LoadBe32(const std::uint8_t* data) noexcept
		{
			return (ByteAt(data, 0U) << 24U) | (ByteAt(data, 1U) << 16U) |
				(ByteAt(data, 2U) << 8U) | ByteAt(data, 3U);
		}

		std::uint64_t LoadBe64(const std::uint8_t* data) noexcept
		{
			return (WideByteAt(data, 0U) << 56U) | (WideByteAt(data, 1U) << 48U) |
				(WideByteAt(data, 2U) << 40U) | (WideByteAt(data, 3U) << 32U) |
				(WideByteAt(data, 4U) << 24U) | (WideByteAt(data, 5U) << 16U) |
				(WideByteAt(data, 6U) << 8U) | WideByteAt(data, 7U);
		}

		std::uint16_t LoadLe16(const std::uint8_t* data) noexcept
		{
			return static_cast<std::uint16_t>((ByteAt(data, 1U) << 8U) | ByteAt(data, 0U));
		}

		std::uint32_t LoadLe32(const std::uint8_t* data) noexcept
		{
			return (ByteAt(data, 3U) << 24U) | (ByteAt(data, 2U) << 16U) |
				(ByteAt(data, 1U) << 8U) | ByteAt(data, 0U);
		}

		std::uint64_t LoadLe64(const std::uint8_t* data) noexcept
		{
			return (WideByteAt(data, 7U) << 56U) | (WideByteAt(data, 6U) << 48U) |
				(WideByteAt(data, 5U) << 40U) | (WideByteAt(data, 4U) << 32U) |
				(WideByteAt(data, 3U) << 24U) | (WideByteAt(data, 2U) << 16U) |
				(WideByteAt(data, 1U) << 8U) | WideByteAt(data, 0U);
		}

		void StoreBe16(std::uint8_t* data, std::uint16_t value) noexcept
		{
			data[0] = ByteFrom(value, 8U);
			data[1] = ByteFrom(value, 0U);
		}

		void StoreBe32(std::uint8_t* data, std::uint32_t value) noexcept
		{
			data[0] = ByteFrom(value, 24U);
			data[1] = ByteFrom(value, 16U);
			data[2] = ByteFrom(value, 8U);
			data[3] = ByteFrom(value, 0U);
		}

		void StoreBe64(std::uint8_t* data, std::uint64_t value) noexcept
		{
			data[0] = ByteFrom(value, 56U);
			data[1] = ByteFrom(value, 48U);
			data[2] = ByteFrom(value, 40U);
			data[3] = ByteFrom(value, 32U);
			data[4] = ByteFrom(value, 24U);
			data[5] = ByteFrom(value, 16U);
			data[6] = ByteFrom(value, 8U);
			data[7] = ByteFrom(value, 0U);
		}

		void StoreLe16(std::uint8_t* data, std::uint16_t value) noexcept
		{
			data[0] = ByteFrom(value, 0U);
			data[1] = ByteFrom(value, 8U);
		}

		void StoreLe32(std::uint8_t* data, std::uint32_t value) noexcept
		{
			data[0] = ByteFrom(value, 0U);
			data[1] = ByteFrom(value, 8U);
			data[2] = ByteFrom(value, 16U);
			data[3] = ByteFrom(value, 24U);
		}

		void StoreLe64(std::uint8_t* data, std::uint64_t value) noexcept
		{
			data[0] = ByteFrom(value, 0U);
			data[1] = ByteFrom(value, 8U);
			data[2] = ByteFrom(value, 16U);
			data[3] = ByteFrom(value, 24U);
			data[4] = ByteFrom(value, 32U);
			data[5] = ByteFrom(value, 40U);
			data[6] = ByteFrom(value, 48U);
			data[7] = ByteFrom(value, 56U);
		}

		bool TryLoadBe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint16ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadBe16(data);
			return true;
		}

		bool TryLoadBe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint32ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadBe32(data);
			return true;
		}

		bool TryLoadBe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint64ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadBe64(data);
			return true;
		}

		bool TryLoadLe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint16ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadLe16(data);
			return true;
		}

		bool TryLoadLe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint32ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadLe32(data);
			return true;
		}

		bool TryLoadLe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint64ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			out = LoadLe64(data);
			return true;
		}

		bool TryStoreBe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint16ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreBe16(data, value);
			return true;
		}

		bool TryStoreBe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint32ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreBe32(data, value);
			return true;
		}

		bool TryStoreBe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint64ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreBe64(data, value);
			return true;
		}

		bool TryStoreLe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint16ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreLe16(data, value);
			return true;
		}

		bool TryStoreLe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint32ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreLe32(data, value);
			return true;
		}

		bool TryStoreLe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept
		{
			const auto has_bytes = HasBytes(data, size, kUint64ByteSize);
			if (!has_bytes)
			{
				return false;
			}

			StoreLe64(data, value);
			return true;
		}

	} // namespace endian

} // namespace ket
