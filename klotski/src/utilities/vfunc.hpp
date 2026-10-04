#pragma once
#include <cstdint>
#include <cstddef>

namespace utilities
{
	template <std::uint32_t index, typename return_t, typename... args_t>
	return_t call_virtual(void* instance, args_t... args)
	{
		using function_t = return_t(__fastcall*)(void*, args_t...);
		const void** vtable = *reinterpret_cast<const void***>(instance);
		return reinterpret_cast<function_t>(vtable[index])(instance, args...);
	}

	}



#define MEM_PAD_IMPL_2(line, size) std::uint8_t pad_##line[size] = { }
#define MEM_PAD_IMPL(line, size) MEM_PAD_IMPL_2(line, size)
#define MEM_PAD(size) MEM_PAD_IMPL(__COUNTER__, size)
