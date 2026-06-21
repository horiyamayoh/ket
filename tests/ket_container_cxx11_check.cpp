#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ket_container.h"

namespace
{
	struct Factory
	{
		int operator()() const
		{
			return 42;
		}
	};

} // namespace

void KetContainerCxx11Check()
{
	std::vector<int> values;
	values.push_back(2);
	values.push_back(1);
	values.push_back(2);

	const bool has_two = ket::container::Contains(values, 2);
	const std::size_t removed = ket::container::EraseIf(values,
														[](int value)
														{
															return value == 1;
														});
	ket::container::SortUnique(values);

	std::map<std::string, int> map_values;
	map_values.insert(std::make_pair(std::string("one"), 1));
	const bool has_key = ket::container::ContainsKey(map_values, std::string("one"));
	int* const pointer = ket::container::AtOrNull(map_values, std::string("one"));
	const std::map<std::string, int>& const_map_values = map_values;
	const int* const const_pointer = ket::container::AtOrNull(const_map_values, std::string("one"));
	const int value = ket::container::AtOr(map_values, std::string("missing"), 0);
	int& created = ket::container::AtOrCreate(map_values, std::string("created"), Factory{});
	created = 43;

	static_cast<void>(has_two);
	static_cast<void>(removed);
	static_cast<void>(has_key);
	static_cast<void>(pointer);
	static_cast<void>(const_pointer);
	static_cast<void>(value);
	static_cast<void>(created);
}
