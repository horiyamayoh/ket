// clang-format off
#include "ket_io_stream.h"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
// clang-format on

namespace ket_io_stream_cxx11_check
{
	static_assert(
		std::is_same<decltype(ket::io_stream::TryReadExactly(std::declval<std::stringstream&>(),
															 static_cast<std::uint8_t*>(nullptr),
															 std::declval<std::size_t>())),
					 bool>::value,
		"TryReadExactly returns bool in C++11");
	static_assert(
		std::is_same<decltype(ket::io_stream::TryWriteAll(std::declval<std::stringstream&>(),
														  static_cast<const std::uint8_t*>(nullptr),
														  std::declval<std::size_t>())),
					 bool>::value,
		"TryWriteAll returns bool in C++11");
	static_assert(
		std::is_same<decltype(ket::io_stream::TryReadLineTrimmedAscii(
						 std::declval<std::stringstream&>(), std::declval<std::string&>())),
					 bool>::value,
		"TryReadLineTrimmedAscii returns bool in C++11");
	static_assert(!std::is_copy_constructible<ket::io_stream::StateSaver>::value,
				  "StateSaver copy construction is disabled");
	static_assert(!std::is_copy_assignable<ket::io_stream::StateSaver>::value,
				  "StateSaver copy assignment is disabled");
	static_assert(!std::is_move_constructible<ket::io_stream::StateSaver>::value,
				  "StateSaver move construction is disabled");
	static_assert(!std::is_move_assignable<ket::io_stream::StateSaver>::value,
				  "StateSaver move assignment is disabled");
	static_assert(noexcept(std::declval<ket::io_stream::StateSaver&>().~StateSaver()),
				  "StateSaver destructor is noexcept");

	void Run()
	{
		const std::uint8_t input[2] = {0x01U, 0x02U};
		std::uint8_t output[2] = {};
		std::stringstream stream;
		std::string line;

		const bool wrote = ket::io_stream::TryWriteAll(stream, input, 2U);
		stream.seekg(0);
		const bool read = ket::io_stream::TryReadExactly(stream, output, 2U);
		stream.clear();
		stream.str("value \t\n");
		const bool line_read = ket::io_stream::TryReadLineTrimmedAscii(stream, line);

		{
			const ket::io_stream::StateSaver saver(stream);
			stream.precision(9);
			static_cast<void>(saver);
		}

		static_cast<void>(wrote);
		static_cast<void>(read);
		static_cast<void>(line_read);
		static_cast<void>(output);
	}

} // namespace ket_io_stream_cxx11_check
