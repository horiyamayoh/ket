#include "ket_tlv.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
	constexpr std::size_t kHeaderSize = 6U;

	std::uint16_t LoadBe16(const std::uint8_t* data) noexcept
	{
		const auto high = static_cast<std::uint16_t>(data[0]);
		const auto low = static_cast<std::uint16_t>(data[1]);

		return static_cast<std::uint16_t>((high << 8U) | low);
	}

	std::uint32_t LoadBe32(const std::uint8_t* data) noexcept
	{
		const auto byte0 = static_cast<std::uint32_t>(data[0]);
		const auto byte1 = static_cast<std::uint32_t>(data[1]);
		const auto byte2 = static_cast<std::uint32_t>(data[2]);
		const auto byte3 = static_cast<std::uint32_t>(data[3]);

		return (byte0 << 24U) | (byte1 << 16U) | (byte2 << 8U) | byte3;
	}

	void AppendBe16(std::vector<std::uint8_t>& dst, std::uint16_t value)
	{
		dst.push_back(static_cast<std::uint8_t>(value >> 8U));
		dst.push_back(static_cast<std::uint8_t>(value & 0x00FFU));
	}

	void AppendBe32(std::vector<std::uint8_t>& dst, std::uint32_t value)
	{
		dst.push_back(static_cast<std::uint8_t>((value >> 24U) & 0x000000FFU));
		dst.push_back(static_cast<std::uint8_t>((value >> 16U) & 0x000000FFU));
		dst.push_back(static_cast<std::uint8_t>((value >> 8U) & 0x000000FFU));
		dst.push_back(static_cast<std::uint8_t>(value & 0x000000FFU));
	}

	std::size_t CheckedRecordSize(std::uint32_t value_size)
	{
		const auto max_size = std::vector<std::uint8_t>().max_size();
		const auto header_does_not_fit = max_size < kHeaderSize;
		if (header_does_not_fit)
		{
			throw std::length_error("ket TLV record exceeds std::vector::max_size().");
		}

		const auto max_value_size = max_size - kHeaderSize;
		const auto value_too_large = value_size > max_value_size;
		if (value_too_large)
		{
			throw std::length_error("ket TLV record exceeds std::vector::max_size().");
		}

		const auto value_count = static_cast<std::size_t>(value_size);
		const auto record_size = kHeaderSize + value_count;
		return record_size;
	}

	void ReserveForAppend(const std::vector<std::uint8_t>& dst, std::size_t record_size)
	{
		const auto max_size = dst.max_size();
		const auto record_too_large = record_size > max_size;
		if (record_too_large)
		{
			throw std::length_error("ket TLV record exceeds std::vector::max_size().");
		}

		const auto record_does_not_fit = dst.size() > max_size - record_size;
		if (record_does_not_fit)
		{
			throw std::length_error("ket TLV output exceeds std::vector::max_size().");
		}
	}

	std::vector<std::uint8_t>
	BuildRecord(std::uint16_t type, const std::uint8_t* value, std::uint32_t value_size)
	{
		const auto record_size = CheckedRecordSize(value_size);
		std::vector<std::uint8_t> record;
		record.reserve(record_size);
		AppendBe16(record, type);
		AppendBe32(record, value_size);

		for (std::uint32_t index = 0; index < value_size; ++index)
		{
			record.push_back(value[index]);
		}

		return record;
	}

	bool ConsumedSizeWouldOverflow(std::uint32_t value_size) noexcept
	{
		const auto max_size = std::numeric_limits<std::size_t>::max();
		const auto header_size = kHeaderSize;
		const auto header_does_not_fit = max_size < header_size;
		if (header_does_not_fit)
		{
			return true;
		}

		const auto max_value_size = max_size - header_size;
		return value_size > max_value_size;
	}

} // namespace

namespace ket
{
	namespace tlv
	{
		std::vector<std::uint8_t>
		Encode(std::uint16_t type, const std::uint8_t* value, std::uint32_t value_size)
		{
			return BuildRecord(type, value, value_size);
		}

		void Append(std::vector<std::uint8_t>& dst,
					std::uint16_t type,
					const std::uint8_t* value,
					std::uint32_t value_size)
		{
			const auto record = BuildRecord(type, value, value_size);
			ReserveForAppend(dst, record.size());
			dst.reserve(dst.size() + record.size());
			dst.insert(dst.end(), record.begin(), record.end());
		}

		bool TryDecode(const std::uint8_t* data, std::size_t size, DecodeResult& out) noexcept
		{
			if (data == nullptr)
			{
				return false;
			}

			const auto header_is_short = size < kHeaderSize;
			if (header_is_short)
			{
				return false;
			}

			const auto type = LoadBe16(data);
			const auto value_size = LoadBe32(data + 2U);
			const auto consumed_size_would_overflow = ConsumedSizeWouldOverflow(value_size);
			if (consumed_size_would_overflow)
			{
				return false;
			}

			const auto remaining_size = size - kHeaderSize;
			const auto value_size_as_size = static_cast<std::size_t>(value_size);
			const auto value_exceeds_input = value_size_as_size > remaining_size;
			if (value_exceeds_input)
			{
				return false;
			}

			const DecodeResult result{{type, data + kHeaderSize, value_size},
									  kHeaderSize + value_size_as_size};
			out = result;

			return true;
		}

	} // namespace tlv

} // namespace ket
