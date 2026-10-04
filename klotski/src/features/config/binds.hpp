#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <Windows.h>

// velocity cuz im lazy :D

namespace binds
{
	enum bind_mode_t : std::int32_t
	{
		bind_hold,
		bind_toggle,
		bind_always,
		bind_mode_count
	};

	struct bind_t
	{
		std::string name;
		std::uint32_t key = 0;
		std::int32_t mode = bind_hold;
		bool* value = nullptr;
		bool previous = false;
		bool capturing = false;
	};

	inline std::vector<bind_t> list;

	inline bool key_down(std::uint32_t key)
	{
		if (!key) return false;
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	inline std::size_t get(const std::string& name, bool* value)
	{
		for (std::size_t i = 0; i < list.size(); i++)
		{
			if (list[i].value == value) return i;
		}

		list.push_back({ name, 0, bind_hold, value, false, false });

		return list.size() - 1;
	}

	inline void clear()
	{
		list.clear();
	}

	inline void run(bool menu_open)
	{
		for (bind_t& bind : list)
		{
			if (!bind.value || !bind.key) continue;

			const bool down = key_down(bind.key);

			switch (bind.mode)
			{
			case bind_hold:
				if (!menu_open) *bind.value = down;
				break;

			case bind_toggle:
				if (down && !bind.previous && !menu_open) *bind.value = !*bind.value;
				break;

			case bind_always:
				if (!menu_open) *bind.value = true;
				break;

			default:
				break;
			}

			bind.previous = down;
		}
	}
}
