#pragma once

#include "../../source2-sdk/math/math.hpp"

namespace console
{
	void initialize();

	void log(const char* message...);

	void console_color_msg(color_t color, const char* message...);
}
