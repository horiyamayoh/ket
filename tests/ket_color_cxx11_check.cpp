#include <string>

#include "../modules/color/ket_color.h"

void KetColorCxx11Check()
{
	ket::color::Rgb color;
	const auto parsed = ket::color::TryParse(std::string("#0aFF10"), color);
	if (!parsed)
	{
		return;
	}

	const auto formatted = ket::color::Format(color);
	static_cast<void>(formatted);
}
