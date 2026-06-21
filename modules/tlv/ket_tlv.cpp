#include "ket_tlv.h"

#include <cstddef>
#include <cstdint>
#include <functional>
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

	std::size_t CheckedRecordSize(std::size_t value_size)
	{
		const auto max_wire_value_size =
			static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
		const auto value_exceeds_wire_length = value_size > max_wire_value_size;
		if (value_exceeds_wire_length)
		{
			throw std::length_error("ket TLV value exceeds uint32 wire length.");
		}

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

		const auto record_size = kHeaderSize + value_size;
		return record_size;
	}

	void ValidateAppendSize(const std::vector<std::uint8_t>& dst, std::size_t record_size)
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

	void AppendValueBytes(std::vector<std::uint8_t>& dst,
						  const std::uint8_t* value,
						  std::size_t value_size)
	{
		for (std::size_t index = 0U; index < value_size; ++index)
		{
			dst.push_back(value[index]);
		}
	}

	std::vector<std::uint8_t>
	BuildRecord(std::uint16_t type, const std::uint8_t* value, std::size_t value_size)
	{
		const auto record_size = CheckedRecordSize(value_size);
		std::vector<std::uint8_t> record;
		record.reserve(record_size);
		AppendBe16(record, type);
		AppendBe32(record, static_cast<std::uint32_t>(value_size));
		AppendValueBytes(record, value, value_size);

		return record;
	}

	bool PointerIsInsideDestination(const std::vector<std::uint8_t>& dst,
									const std::uint8_t* value,
									std::size_t value_size) noexcept
	{
		const auto destination_is_empty = dst.empty();
		const auto value_is_null = value == nullptr;
		const auto value_is_empty = value_size == 0U;
		if (destination_is_empty || value_is_null || value_is_empty)
		{
			return false;
		}

		const auto* const begin = dst.data();
		const auto* const end = begin + dst.size();
		const std::less<const std::uint8_t*> less; // NOLINT(modernize-use-transparent-functors)
		const auto before_begin = less(value, begin);
		const auto before_end = less(value, end);
		return !before_begin && before_end;
	}

	void AppendRecord(std::vector<std::uint8_t>& dst,
					  std::uint16_t type,
					  const std::uint8_t* value,
					  std::size_t value_size)
	{
		const auto record_size = CheckedRecordSize(value_size);
		ValidateAppendSize(dst, record_size);
		dst.reserve(dst.size() + record_size);
		AppendBe16(dst, type);
		AppendBe32(dst, static_cast<std::uint32_t>(value_size));
		AppendValueBytes(dst, value, value_size);
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
		const auto value_count = static_cast<std::size_t>(value_size);
		return value_count > max_value_size;
	}

} // namespace

namespace ket
{
	namespace tlv
	{
		std::vector<std::uint8_t>
		Encode(std::uint16_t type, const std::uint8_t* value, std::size_t value_size)
		{
			return BuildRecord(type, value, value_size);
		}

		void Append(std::vector<std::uint8_t>& dst,
					std::uint16_t type,
					const std::uint8_t* value,
					std::size_t value_size)
		{
			const auto value_is_inside_destination =
				PointerIsInsideDestination(dst, value, value_size);
			if (value_is_inside_destination)
			{
				const auto record = BuildRecord(type, value, value_size);
				AppendRecord(dst, type, record.data() + kHeaderSize, value_size);
				return;
			}

			AppendRecord(dst, type, value, value_size);
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

			DecodeResult result;
			result.view.type = type;
			result.view.value = data + kHeaderSize;
			result.view.value_size = value_size;
			result.consumed = kHeaderSize + value_size_as_size;
			out = result;

			return true;
		}

	} // namespace tlv

} // namespace ket
