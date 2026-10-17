#pragma once

#include <cstddef>
#include <cstdint>

#include "../../utilities/utilities.hpp"

class c_econ_item_definition;
class c_paint_kit;

template <typename node_t>
class econ_utl_map_t
{
public:
	std::int32_t count;
	std::int32_t capacity_flags;
	node_t* elements;

	bool is_valid(std::int32_t max_count) const
	{
		return count > 0 && count <= max_count && utilities::is_valid_pointer(elements);
	}

	node_t& element(std::int32_t index) { return elements[index]; }
	const node_t& element(std::int32_t index) const { return elements[index]; }
};

struct econ_item_definition_node_t
{
	std::int32_t left;
	std::int32_t right;
	c_econ_item_definition* value;
	std::int32_t flag;
	std::int32_t key;
};

struct paint_kit_node_t
{
	std::int32_t left;
	std::int32_t right;
	std::int32_t pad_08;
	std::int32_t pad_0c;
	std::int32_t key;
	std::int32_t pad_14;
	c_paint_kit* value;
};

class c_paint_kit
{
public:
	std::int32_t id() const { return read<std::int32_t>(0x00); }
	const char* name() const { return read<const char*>(0x08); }
	const char* description_string() const { return read<const char*>(0x10); }
	const char* description_tag() const { return read<const char*>(0x18); }
	std::int32_t rarity() const { return read<std::int32_t>(0x44); }
	bool uses_old_model() const { return read<bool>(0xae); }

private:
	template <typename t>
	t read(std::uintptr_t offset) const
	{
		return *reinterpret_cast<const t*>(reinterpret_cast<const std::uint8_t*>(this) + offset);
	}
};

class c_econ_item_definition
{
public:
	std::uint16_t definition_index() const { return read<std::uint16_t>(0x10); }
	const char* item_base_name() const { return read<const char*>(0x70); }
	const char* item_type_name() const { return read<const char*>(0x80); }
	const char* model_name() const { return read<const char*>(0x148); }
	const char* simple_weapon_name() const { return read<const char*>(0x230); }
	const char* weapon_name() const { return read<const char*>(0x248); }
	const char* item_name() const { return read<const char*>(0x260); }

private:
	template <typename t>
	t read(std::uintptr_t offset) const
	{
		return *reinterpret_cast<const t*>(reinterpret_cast<const std::uint8_t*>(this) + offset);
	}
};

class c_econ_item_schema
{
public:
	econ_utl_map_t<econ_item_definition_node_t>& sorted_item_definitions()
	{
		return *reinterpret_cast<econ_utl_map_t<econ_item_definition_node_t>*>(reinterpret_cast<std::uint8_t*>(this) + 0xf8);
	}

	econ_utl_map_t<paint_kit_node_t>& paint_kits()
	{
		static constexpr std::uintptr_t offsets[] = { 0x2a0, 0x2a8, 0x2f0 };

		for (const std::uintptr_t offset : offsets)
		{
			auto* map = reinterpret_cast<econ_utl_map_t<paint_kit_node_t>*>(reinterpret_cast<std::uint8_t*>(this) + offset);
			if (map->is_valid(10000)) return *map;
		}

		return *reinterpret_cast<econ_utl_map_t<paint_kit_node_t>*>(reinterpret_cast<std::uint8_t*>(this) + 0x2a0);
	}
};

class c_econ_item_system
{
public:
	c_econ_item_schema* schema() const
	{
		return *reinterpret_cast<c_econ_item_schema* const*>(reinterpret_cast<const std::uint8_t*>(this) + 0x8);
	}
};
