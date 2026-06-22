#include <array>
#include <cstdint>

#include "ket_wire.h"

namespace
{
	struct ExactIntegerFrame
	{
		std::uint8_t u8 = 0U;
		std::uint16_t u16_be = 0U;
		std::uint16_t u16_le = 0U;
		std::uint32_t u32_be = 0U;
		std::uint32_t u32_le = 0U;
		std::uint64_t u64_be = 0U;
		std::uint64_t u64_le = 0U;
		std::int8_t i8 = 0;
		std::int16_t i16_be = 0;
		std::int16_t i16_le = 0;
		std::int32_t i32_be = 0;
		std::int32_t i32_le = 0;
		std::int64_t i64_be = 0;
		std::int64_t i64_le = 0;
	};

	[[maybe_unused]] void BuildExactIntegerSchema()
	{
		const auto schema = ket::wire::MakeSchema<ExactIntegerFrame>(
			"ExactIntegerFrame",
			std::array<ket::wire::Field<ExactIntegerFrame>, 14U>{
				ket::wire::U8<ExactIntegerFrame, &ExactIntegerFrame::u8>("u8"),
				ket::wire::U16Be<ExactIntegerFrame, &ExactIntegerFrame::u16_be>("u16_be"),
				ket::wire::U16Le<ExactIntegerFrame, &ExactIntegerFrame::u16_le>("u16_le"),
				ket::wire::U32Be<ExactIntegerFrame, &ExactIntegerFrame::u32_be>("u32_be"),
				ket::wire::U32Le<ExactIntegerFrame, &ExactIntegerFrame::u32_le>("u32_le"),
				ket::wire::U64Be<ExactIntegerFrame, &ExactIntegerFrame::u64_be>("u64_be"),
				ket::wire::U64Le<ExactIntegerFrame, &ExactIntegerFrame::u64_le>("u64_le"),
				ket::wire::I8<ExactIntegerFrame, &ExactIntegerFrame::i8>("i8"),
				ket::wire::I16Be<ExactIntegerFrame, &ExactIntegerFrame::i16_be>("i16_be"),
				ket::wire::I16Le<ExactIntegerFrame, &ExactIntegerFrame::i16_le>("i16_le"),
				ket::wire::I32Be<ExactIntegerFrame, &ExactIntegerFrame::i32_be>("i32_be"),
				ket::wire::I32Le<ExactIntegerFrame, &ExactIntegerFrame::i32_le>("i32_le"),
				ket::wire::I64Be<ExactIntegerFrame, &ExactIntegerFrame::i64_be>("i64_be"),
				ket::wire::I64Le<ExactIntegerFrame, &ExactIntegerFrame::i64_le>("i64_le")});
		(void)schema;
	}

} // namespace
