#pragma once

#include <cstdint>

class i_file_system
{
public:
	void* open(const char* file_name, const char* options = "rb", const char* path_id = "GAME")
	{
		using function_t = void*(__fastcall*)(i_file_system*, const char*, const char*, const char*);
		return (*reinterpret_cast<function_t**>(this))[13](this, file_name, options, path_id);
	}

	int read(void* destination, int size, void* handle)
	{
		using function_t = int(__fastcall*)(i_file_system*, void*, int, void*);
		return (*reinterpret_cast<function_t**>(this))[11](this, destination, size, handle);
	}

	std::uint64_t size(void* handle)
	{
		using function_t = std::uint64_t(__fastcall*)(i_file_system*, void*);
		return (*reinterpret_cast<function_t**>(this))[18](this, handle);
	}

	void close(void* handle)
	{
		using function_t = void(__fastcall*)(i_file_system*, void*);
		(*reinterpret_cast<function_t**>(this))[14](this, handle);
	}

	bool exists(const char* file_name, const char* path_id = "GAME")
	{
		using function_t = bool(__fastcall*)(i_file_system*, const char*, const char*);
		return (*reinterpret_cast<function_t**>(this))[21](this, file_name, path_id);
	}
};
