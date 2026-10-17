#pragma once

#include <cstdint>
#include "../math/math.hpp"
#include "../../utilities/utilities.hpp"
#include "../classes/econ_item_schema.hpp"

class i_client
{
public:
	c_econ_item_system* get_econ_item_system()
	{
		using function_t = c_econ_item_system*(__fastcall*)(i_client*);
		return (*reinterpret_cast<function_t**>(this))[129](this);
	}

	void set_view_angles(vec3_t angles)
	{}
};