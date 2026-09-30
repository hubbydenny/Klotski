#pragma once

#include <cstdint>
#include <array>
#include <vector>

typedef struct _schema_class_field_data_t
{
	const char* field_name;
	void* field_type;
	std::int32_t field_offset;
	char pad1[0xC];
} schema_class_field_data_t;

static_assert(sizeof(schema_class_field_data_t) == 0x20, "schema_class_field_data_t has wrong size");

class schema_class_binding_t
{
public:
	char pad1[0x8];
	const char* binding_name;
	const char* dll_name;
	char pad2[0x8];
	std::int32_t size_of;
	std::uint16_t data_array_size;
	char pad3[0x2];
	char pad4[0x8];
	schema_class_field_data_t* data_array;
	void* base_class;
};

class schema_block_t
{
public:
	void* unkn0;
	schema_block_t* next;
	schema_class_binding_t* binding;
};

class schema_block_container_t
{
public:
	void* unkn[2];
	schema_block_t* first_block;
};

class schema_class_list_t
{
public:
	std::array<schema_block_container_t, 256> block_containers;

	std::int32_t get_num_schema()
	{
		return *reinterpret_cast<std::int32_t*>(reinterpret_cast<std::uintptr_t>(this) - 0x74);
	}
};

class schema_system_type_scope
{
public:
	schema_class_list_t* get_class_container()
	{
		return reinterpret_cast<schema_class_list_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x5C0);
	}
};

class i_schema_system
{
public:
	schema_system_type_scope* global_type_scope()
	{
		using function_t = schema_system_type_scope*(__thiscall*)(void*);
		return (*reinterpret_cast<function_t**>(this))[11](this);
	}
};
