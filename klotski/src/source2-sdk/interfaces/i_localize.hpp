#pragma once

class i_localize
{
public:
	const char* find_safe(const char* token)
	{
		if (!token || !*token) return "";
		using function_t = const char*(__fastcall*)(i_localize*, const char*);
		return (*reinterpret_cast<function_t**>(this))[17](this, token);
	}
};
