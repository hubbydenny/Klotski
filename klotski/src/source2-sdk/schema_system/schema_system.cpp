#include "schema_system.hpp"

#include "../interfaces/interfaces.hpp"
#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"

#include <unordered_map>
#include <vector>

static std::unordered_map<std::string, std::unordered_map<std::string, schema_system::schema_offset_t>> schema_data;

bool schema_system::initialize()
{
	if (!interfaces::schema_system)
	{
		debug::log("[-] failed to initialize schema system (no interface)\n");
		return false;
	}

	std::vector<schema_system_type_scope*> scopes;

	scopes.push_back(interfaces::schema_system->global_type_scope());

	std::uint8_t* all_type_scope_address = utilities::pattern_scan(L"schemasystem.dll", ALL_TYPE_SCOPE);

	if (all_type_scope_address)
	{
		std::uint8_t* all_type_scope = utilities::resolve_rip(all_type_scope_address, 3, 7);

		auto scopes_array = *reinterpret_cast<schema_system_type_scope***>(all_type_scope);
		auto scopes_count = *reinterpret_cast<std::uint16_t*>(all_type_scope - 0x8);

		if (scopes_array)
		{
			for (std::uint16_t i = 0; i < scopes_count; i++)
			{
				if (scopes_array[i])
				{
					scopes.push_back(scopes_array[i]);
				}
			}
		}
	}

	for (schema_system_type_scope* scope : scopes)
	{
		if (!scope)
		{
			continue;
		}

		schema_class_list_t* class_list = scope->get_class_container();

		if (!class_list)
		{
			continue;
		}

		std::int32_t block_index = 0;

		for (schema_block_container_t& block_container : class_list->block_containers)
		{
			for (schema_block_t* block = block_container.first_block; block && block_index < class_list->get_num_schema(); block = block->next, block_index++)
			{
				schema_class_binding_t* binding = block->binding;

				if (!binding || !binding->binding_name)
				{
					continue;
				}

				if (!binding->data_array)
				{
					continue;
				}

				for (std::int32_t i = 0; i < static_cast<std::int32_t>(binding->data_array_size); i++)
				{
					schema_class_field_data_t* field = &binding->data_array[i];

					if (!field->field_name)
					{
						continue;
					}

					schema_data[binding->binding_name][field->field_name].class_name = binding->binding_name;
					schema_data[binding->binding_name][field->field_name].property_name = field->field_name;
					schema_data[binding->binding_name][field->field_name].offset = field->field_offset;
				}
			}
		}
	}

	if (schema_data.empty())
	{
		debug::log("[-] failed to initialize schema system\n");
		return false;
	}
	return true;
}

bool schema_system::has_schema(std::string class_name, std::string property_name)
{
    auto class_it = schema_data.find(class_name);

    if (class_it == schema_data.end())
    {
        return false;
    }

    auto property_it = class_it->second.find(property_name);

    if (property_it == class_it->second.end())
    {
        return false;
    }

    return property_it->second.offset != 0;
}

std::uint32_t schema_system::get_schema(std::string class_name, std::string property_name)
{
    if (!schema_system::has_schema(class_name, property_name))
    {
        return 0;
    }

    return schema_data[class_name][property_name].offset;
}
