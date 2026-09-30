#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "../math/math.hpp"
#include "types.hpp"

class cmd_qangle_t
{
public:
	char pad1[0x18];
	vec3_t angles;
};

static_assert(sizeof(cmd_qangle_t) == 0x24, "cmd_qangle_t has wrong size");

class cin_button_state_pb_t
{
public:
	char pad1[0x10];
	std::uint32_t has_bits;
	std::int32_t cached_size;
	std::uint64_t buttonstate1;
	std::uint64_t buttonstate2;
	std::uint64_t buttonstate3;
};

static_assert(offsetof(cin_button_state_pb_t, has_bits) == 0x10, "cin_button_state_pb_t has wrong size");
static_assert(offsetof(cin_button_state_pb_t, cached_size) == 0x14, "cin_button_state_pb_t has wrong size");
static_assert(offsetof(cin_button_state_pb_t, buttonstate1) == 0x18, "cin_button_state_pb_t has wrong size");
static_assert(offsetof(cin_button_state_pb_t, buttonstate2) == 0x20, "cin_button_state_pb_t has wrong size");
static_assert(offsetof(cin_button_state_pb_t, buttonstate3) == 0x28, "cin_button_state_pb_t has wrong size");
static_assert(sizeof(cin_button_state_pb_t) == 0x30, "cin_button_state_pb_t has wrong size");

class protobuf_repeated_ptr_field_t
{
public:
	struct rep_t
	{
		std::int32_t allocated_size;
		void** elements;
	};

	void* arena;
	std::int32_t current_size;
	std::int32_t capacity;
	rep_t* rep;
};

static_assert(sizeof(protobuf_repeated_ptr_field_t) == 0x18, "protobuf_repeated_ptr_field_t has wrong size");

class utl_string_t
{
public:
	char* memory;
	std::int32_t allocation_count;
	std::int32_t grow_size;
	std::int32_t length;
	char pad[0x4];
};

static_assert(sizeof(utl_string_t) == 0x18, "utl_string_t has wrong size");

class subtick_move_step_t
{
public:
	char pad1[0x10];
	std::uint32_t has_bits;
	std::int32_t cached_size;
	std::uint64_t button;
	bool pressed;
	char pad2[0x3];
	float when;
	float analog_forward_delta;
	float analog_left_delta;
	float pitch_delta;
	float yaw_delta;

	void set_button(std::uint64_t value)
	{
		has_bits |= 0x1u;
		button = value;
	}

	void set_pressed(bool value)
	{
		has_bits |= 0x2u;
		pressed = value;
	}

	void set_when(float value)
	{
		has_bits |= 0x4u;
		when = value;
	}
};

static_assert(offsetof(subtick_move_step_t, has_bits) == 0x10, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, cached_size) == 0x14, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, button) == 0x18, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, pressed) == 0x20, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, when) == 0x24, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, analog_forward_delta) == 0x28, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, analog_left_delta) == 0x2C, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, pitch_delta) == 0x30, "subtick_move_step_t has wrong size");
static_assert(offsetof(subtick_move_step_t, yaw_delta) == 0x34, "subtick_move_step_t has wrong size");
static_assert(sizeof(subtick_move_step_t) == 0x38, "subtick_move_step_t has wrong size");

class csgo_interpolation_info_pb_t
{
public:
	char pad1[0x10];
	std::uint32_t has_bits;
	std::int32_t cached_size;
	float frac;
	std::int32_t src_tick;
	std::int32_t dst_tick;
	char pad2[0x4];
};

static_assert(offsetof(csgo_interpolation_info_pb_t, has_bits) == 0x10, "csgo_interpolation_info_pb_t has wrong size");
static_assert(offsetof(csgo_interpolation_info_pb_t, cached_size) == 0x14, "csgo_interpolation_info_pb_t has wrong size");
static_assert(offsetof(csgo_interpolation_info_pb_t, frac) == 0x18, "csgo_interpolation_info_pb_t has wrong size");
static_assert(offsetof(csgo_interpolation_info_pb_t, src_tick) == 0x1C, "csgo_interpolation_info_pb_t has wrong size");
static_assert(offsetof(csgo_interpolation_info_pb_t, dst_tick) == 0x20, "csgo_interpolation_info_pb_t has wrong size");
static_assert(sizeof(csgo_interpolation_info_pb_t) == 0x28, "csgo_interpolation_info_pb_t has wrong size");

class cbase_user_cmd_pb_t
{
public:
	enum has_bits_t : std::uint32_t
	{
		has_move_crc = 0x1,
		has_buttons = 0x2,
		has_viewangles = 0x4,
		has_legacy_command_number = 0x10,
		has_client_tick = 0x20,
		has_forwardmove = 0x40,
		has_leftmove = 0x80,
		has_upmove = 0x100,
		has_impulse = 0x200,
		has_weaponselect = 0x400,
		has_random_seed = 0x800,
		has_mousedx = 0x1000,
		has_mousedy = 0x2000,
		has_prediction_offset_ticks_x256 = 0x4000,
		has_consumed_server_angle_changes = 0x8000,
		has_cmd_flags = 0x10000,
		has_pawn_entity_handle = 0x20000
	};

	char pad1[0x10];
	std::uint32_t has_bits;
	std::int32_t cached_size;
	protobuf_repeated_ptr_field_t subtick_moves;
	void* move_crc;
	cin_button_state_pb_t* buttons;
	cmd_qangle_t* viewangles;
	void* execution_notes;
	std::int32_t legacy_command_number;
	std::int32_t client_tick;
	float forwardmove;
	float leftmove;
	float upmove;
	std::int32_t impulse;
	std::int32_t weaponselect;
	std::int32_t random_seed;
	std::int32_t mousedx;
	std::int32_t mousedy;
	std::uint32_t prediction_offset_ticks_x256;
	std::uint32_t consumed_server_angle_changes;
	std::int32_t cmd_flags;
	std::uint32_t pawn_entity_handle;
};

static_assert(offsetof(cbase_user_cmd_pb_t, has_bits) == 0x10, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, cached_size) == 0x14, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, subtick_moves) == 0x18, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, move_crc) == 0x30, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, buttons) == 0x38, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, viewangles) == 0x40, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, execution_notes) == 0x48, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, legacy_command_number) == 0x50, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, client_tick) == 0x54, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, forwardmove) == 0x58, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, leftmove) == 0x5C, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, upmove) == 0x60, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, impulse) == 0x64, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, weaponselect) == 0x68, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, random_seed) == 0x6C, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, mousedx) == 0x70, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, mousedy) == 0x74, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, prediction_offset_ticks_x256) == 0x78, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, consumed_server_angle_changes) == 0x7C, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, cmd_flags) == 0x80, "cbase_user_cmd_pb_t has wrong size");
static_assert(offsetof(cbase_user_cmd_pb_t, pawn_entity_handle) == 0x84, "cbase_user_cmd_pb_t has wrong size");
static_assert(sizeof(cbase_user_cmd_pb_t) == 0x88, "cbase_user_cmd_pb_t has wrong size");

class csgo_user_cmd_pb_t
{
public:
	char pad1[0x10];
	std::uint32_t has_bits;
	std::int32_t cached_size;
	protobuf_repeated_ptr_field_t input_history;
	cbase_user_cmd_pb_t* base;
	bool left_hand_desired;
	bool is_predicting_body_shot_fx;
	bool is_predicting_head_shot_fx;
	bool is_predicting_kill_ragdolls;
	std::int32_t attack1_start_history_index;
	std::int32_t attack2_start_history_index;
};

static_assert(offsetof(csgo_user_cmd_pb_t, has_bits) == 0x10, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, cached_size) == 0x14, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, input_history) == 0x18, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, base) == 0x30, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, left_hand_desired) == 0x38, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, is_predicting_body_shot_fx) == 0x39, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, is_predicting_head_shot_fx) == 0x3A, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, is_predicting_kill_ragdolls) == 0x3B, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, attack1_start_history_index) == 0x3C, "csgo_user_cmd_pb_t has wrong size");
static_assert(offsetof(csgo_user_cmd_pb_t, attack2_start_history_index) == 0x40, "csgo_user_cmd_pb_t has wrong size");
static_assert(sizeof(csgo_user_cmd_pb_t) == 0x48, "csgo_user_cmd_pb_t has wrong size");

class user_cmd_t
{
public:
	void* vtable;
	std::int64_t command_number_value;
	csgo_user_cmd_pb_t cmd;
	void* button_state_vtable;
	std::uint64_t buttonstate1;
	std::uint64_t buttonstate2;
	std::uint64_t buttonstate3;
	char pad1[0x1C];
	std::int32_t subtick_state;

	cbase_user_cmd_pb_t* get_base()
	{
		cbase_user_cmd_pb_t* base = cmd.base;

		return utilities::is_valid_pointer(base) ? base : nullptr;
	}

	cin_button_state_pb_t* get_proto_buttons()
	{
		cbase_user_cmd_pb_t* base = get_base();
		if (!base) return nullptr;
		cin_button_state_pb_t* buttons = base->buttons;

		return utilities::is_valid_pointer(buttons) ? buttons : nullptr;
	}

	cmd_qangle_t* get_view_angles()
	{
		cbase_user_cmd_pb_t* base = get_base();

		if (!base) return nullptr;
		return utilities::is_valid_pointer(base->viewangles) ? base->viewangles : nullptr;
	}

	bool is_button_down(std::uint64_t buttons)
	{
		return (buttonstate1 & buttons) != 0;
	}

	std::int32_t command_number()
	{
		return static_cast<std::int32_t>(command_number_value);
	}

	bool has_been_predicted()
	{
		return *reinterpret_cast<const bool*>(reinterpret_cast<const std::uint8_t*>(this) + 0x88);
	}

	void set_has_been_predicted(bool value)
	{
		*reinterpret_cast<bool*>(reinterpret_cast<std::uint8_t*>(this) + 0x88) = value;
	}

	bool update_move_crc()
	{
		cbase_user_cmd_pb_t* base = get_base();

		if (!base) return false;

		using function_t = void(__fastcall*)(void*, void*, void*);
		static function_t serialize_move_crc = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", SERIALIZE_MOVE_CRC));

		if (!serialize_move_crc) return false;

		const cin_button_state_pb_t* buttons = utilities::is_valid_pointer(base->buttons) ? base->buttons : nullptr;
		const cmd_qangle_t* view = utilities::is_valid_pointer(base->viewangles) ? base->viewangles : nullptr;

		const std::uint64_t button_states[3] =
		{
			buttons ? buttons->buttonstate1 : 0ull,
			buttons ? buttons->buttonstate2 : 0ull,
			buttons ? buttons->buttonstate3 : 0ull
		};
		const float angles[3] = { view ? view->angles.x : 0.f, view ? view->angles.y : 0.f, view ? view->angles.z : 0.f };

		std::uint8_t buffer[64] = { };
		std::uint8_t* cursor = buffer;

		std::uint8_t button_size = 0;

		for (const std::uint64_t state : button_states)
		{
			if (state != 0) button_size += 9;
		}

		if (button_size != 0)
		{
			*cursor++ = 0x1a;
			*cursor++ = button_size;

			for (std::size_t i = 0; i < 3; ++i)
			{
				if (button_states[i] == 0) continue;

				*cursor++ = static_cast<std::uint8_t>(0x09 + i * 0x08);
				std::memcpy(cursor, &button_states[i], sizeof(std::uint64_t));
				cursor += sizeof(std::uint64_t);
			}
		}

		std::uint8_t angle_size = 0;

		for (const float angle : angles)
		{
			if (angle != 0.f) angle_size += 5;
		}

		if (angle_size != 0)
		{
			*cursor++ = 0x22;
			*cursor++ = angle_size;

			for (std::size_t i = 0; i < 3; ++i)
			{
				if (angles[i] == 0.f) continue;

				*cursor++ = static_cast<std::uint8_t>(0x0d + i * 0x08);
				std::memcpy(cursor, &angles[i], sizeof(float));
				cursor += sizeof(float);
			}
		}

		const std::uintptr_t arena_address = reinterpret_cast<std::uintptr_t>(base) + 0x8;

		if (!utilities::is_readable(reinterpret_cast<void*>(arena_address), sizeof(std::uintptr_t))) return false;

		utl_string_t payload = { reinterpret_cast<char*>(buffer), 0, 0, static_cast<std::int32_t>(cursor - buffer) };

		const std::uintptr_t arena_bits = *reinterpret_cast<std::uintptr_t*>(arena_address);
		std::uintptr_t arena = arena_bits & ~static_cast<std::uintptr_t>(0x3);

		if ((arena_bits & 1) != 0)
		{
			if (!utilities::is_readable(reinterpret_cast<void*>(arena), sizeof(std::uintptr_t))) return false;

			arena = *reinterpret_cast<std::uintptr_t*>(arena);
		}

		serialize_move_crc(&base->move_crc, &payload, reinterpret_cast<void*>(arena));

		base->has_bits |= cbase_user_cmd_pb_t::has_move_crc;

		return true;
	}

	void sync_buttons()
	{
		cin_button_state_pb_t* buttons = get_proto_buttons();

		if (!buttons)
		{
			return;
		}

		cbase_user_cmd_pb_t* base = get_base();

		base->has_bits |= cbase_user_cmd_pb_t::has_buttons;
		buttons->has_bits |= 0x7u;
		buttons->buttonstate1 = buttonstate1;
		buttons->buttonstate2 = buttonstate2;
		buttons->buttonstate3 = buttonstate3;
	}

	void set_buttons(std::uint64_t buttons, bool state)
	{
		if (state)
		{
			buttonstate1 |= buttons;
			buttonstate2 |= buttons;
		}
		else
		{
			buttonstate1 &= ~buttons;
			buttonstate2 &= ~buttons;
			buttonstate3 &= ~buttons;
		}

		sync_buttons();
	}

	void set_button(buttons_t button, bool state)
	{
		set_buttons(static_cast<std::uint64_t>(button), state);
	}

	void set_move(float forward, float left)
	{
		cbase_user_cmd_pb_t* base = get_base();

		if (!base) return;
		base->has_bits |= cbase_user_cmd_pb_t::has_forwardmove | cbase_user_cmd_pb_t::has_leftmove;
		base->forwardmove = forward;
		base->leftmove = left;
	}

	void set_view_angles(vec3_t angles)
	{
		cmd_qangle_t* view = get_view_angles();

		if (!view) return;
		view->angles.x = angles.x;
		view->angles.y = angles.y;
	}

	void clear_subticks()
	{
		cbase_user_cmd_pb_t* base = get_base();

		if (!base) return;
		if (!utilities::is_readable(&base->subtick_moves, sizeof(protobuf_repeated_ptr_field_t))) return;

		base->subtick_moves.current_size = 0;
	}

	void add_subtick(buttons_t button, bool pressed, float when)
	{
		cbase_user_cmd_pb_t* base = get_base();

		if (!base) return;
		protobuf_repeated_ptr_field_t* field = &base->subtick_moves;
		if (!utilities::is_readable(field, sizeof(protobuf_repeated_ptr_field_t))) return;
		

		subtick_move_step_t* step = nullptr;
		protobuf_repeated_ptr_field_t::rep_t* rep = field->rep;

		if (rep && utilities::is_readable(rep, sizeof(protobuf_repeated_ptr_field_t::rep_t)))
		{
			const std::int32_t index = field->current_size;
			const std::int32_t allocated = rep->allocated_size;

			if (index >= 0 && allocated > 0 && index < allocated && allocated <= 512 && utilities::is_readable(rep->elements, sizeof(void*) * (index + 1)))
			{
				subtick_move_step_t* reused = reinterpret_cast<subtick_move_step_t*>(rep->elements[index]);

				if (utilities::is_readable(reused, sizeof(subtick_move_step_t)))
				{
					field->current_size++;
					reused->has_bits = 0;
					reused->cached_size = 0;
					reused->button = 0;
					reused->pressed = false;
					reused->when = 0.f;
					reused->analog_forward_delta = 0.f;
					reused->analog_left_delta = 0.f;
					reused->pitch_delta = 0.f;
					reused->yaw_delta = 0.f;
					step = reused;
				}
			}
		}

		if (!step)
		{
			using create_fn_t = subtick_move_step_t*(__fastcall*)(void*);
			using add_fn_t = void*(__fastcall*)(void*, void*);

			static create_fn_t create_subtick = reinterpret_cast<create_fn_t>(utilities::scan_call(L"client.dll", CREATE_SUBTICK_MOVE_STEP));
			static add_fn_t add_to_repeated = reinterpret_cast<add_fn_t>(utilities::scan_function(L"client.dll", PROTOBUF_ADD_TO_REPEATED_PTR_ELEMENT));

			if (!create_subtick || !add_to_repeated || !field->arena) return;

			step = create_subtick(field->arena);

			if (!utilities::is_readable(step, sizeof(subtick_move_step_t))) return;

			add_to_repeated(field, step);
		}

		step->set_button(static_cast<std::uint64_t>(button));
		step->set_pressed(pressed);
		step->set_when(when);
	}
};

static_assert(offsetof(user_cmd_t, vtable) == 0x0, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, command_number_value) == 0x8, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, cmd) == 0x10, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, button_state_vtable) == 0x58, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, buttonstate1) == 0x60, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, buttonstate2) == 0x68, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, buttonstate3) == 0x70, "user_cmd_t has wrong size");
static_assert(offsetof(user_cmd_t, subtick_state) == 0x94, "user_cmd_t has wrong size");
static_assert(sizeof(user_cmd_t) == 0x98, "user_cmd_t has wrong size");
