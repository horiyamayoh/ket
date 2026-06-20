#include <cstddef>
#include <type_traits>
#include <vector>

#include "ket_ranges.h"

namespace ket_ranges_cxx11_check
{
	struct AdlRange
	{
		int* first;
		int* last;
	};

	int* begin(AdlRange& range)
	{
		return range.first;
	}

	int* end(AdlRange& range)
	{
		return range.last;
	}

	struct MoveOnlyForEach
	{
		MoveOnlyForEach() = default;

		MoveOnlyForEach(const MoveOnlyForEach&) = delete;
		MoveOnlyForEach& operator=(const MoveOnlyForEach&) = delete;

		MoveOnlyForEach(MoveOnlyForEach&&) noexcept = default;
		MoveOnlyForEach& operator=(MoveOnlyForEach&&) noexcept = default;

		void operator()(std::size_t index, int& value)
		{
			++call_count_;
			value += static_cast<int>(index);
		}

	  private:
		int call_count_ = 0;
	};

	struct MoveOnlyPredicate
	{
		MoveOnlyPredicate() = default;

		MoveOnlyPredicate(const MoveOnlyPredicate&) = delete;
		MoveOnlyPredicate& operator=(const MoveOnlyPredicate&) = delete;

		MoveOnlyPredicate(MoveOnlyPredicate&&) noexcept = default;
		MoveOnlyPredicate& operator=(MoveOnlyPredicate&&) noexcept = default;

		bool operator()(const int& value)
		{
			++call_count_;
			return value == 2;
		}

	  private:
		int call_count_ = 0;
	};

	struct IndexTypeProbe
	{
		template <typename Index, typename Element>
		void operator()(Index index, Element& element) const
		{
			static_assert(std::is_same<Index, std::size_t>::value,
						  "ForEachWithIndex passes std::size_t index in C++11");
			static_cast<void>(index);
			static_cast<void>(element);
		}
	};

	struct BoolProxy
	{
		explicit BoolProxy(bool value) : value_(value) {}

		operator bool() const
		{
			return value_;
		}

	  private:
		bool value_;
	};

	struct ProxyPredicate
	{
		BoolProxy operator()(const int& value) const
		{
			return BoolProxy(value == 4);
		}
	};

} // namespace ket_ranges_cxx11_check

void KetRangesCxx11Check()
{
	int values[] = {1, 2};
	ket_ranges_cxx11_check::AdlRange range = {values, values + 2};

	ket_ranges_cxx11_check::MoveOnlyForEach visitor;
	ket::ranges::ForEachWithIndex(range, visitor);

	const ket_ranges_cxx11_check::IndexTypeProbe index_type_probe;
	ket::ranges::ForEachWithIndex(range, index_type_probe);

	std::size_t found_index = 0U;
	ket_ranges_cxx11_check::MoveOnlyPredicate predicate;
	const bool found = ket::ranges::FindIndexIf(range, predicate, found_index);

	std::vector<int> proxy_values = {4, 5};
	const bool proxy_found = ket::ranges::FindIndexIf(
		proxy_values, ket_ranges_cxx11_check::ProxyPredicate(), found_index);

	static_cast<void>(found);
	static_cast<void>(proxy_found);
	static_cast<void>(found_index);
}
