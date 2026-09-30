#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

class cprediction_t
{
public:
	char pad_0000[48];
	std::int32_t m_call_reason;
	bool in_prediction;
	char pad_0035[3];
	void* m_current_cmd;
	bool engine_pause;
	char pad_0041[3];
	std::int32_t m_snapshot_tick;
	std::int32_t m_last_ack_tick;
	char pad_004C[4];
	std::int32_t m_slot_count;
	char pad_0054[4];
	void* m_slots;
	char pad_0060[40];
	std::uint8_t m_frame_snapshot[96];
	bool in_current_tick;
	char pad_00E9[1];
	bool supress_prediction;
	char pad_00EB[5];
	bool first_prediction;
	char pad_00F1[3];
	bool b_buttons_changed;
	char pad_00F5[3];
	std::int32_t m_desired_tick;
	float m_desired_when;
	char pad_0100[12];
	bool b_cmd_changed;
	char pad_010D[11];
	std::int32_t m_transmit_count;
	char pad_011C[20];
	std::int32_t m_error_count;
	char pad_0134[84];
	bool needs_reinit;
};

static_assert(offsetof(cprediction_t, m_call_reason) == 0x30, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, in_prediction) == 0x34, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_current_cmd) == 0x38, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, engine_pause) == 0x40, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_snapshot_tick) == 0x44, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_last_ack_tick) == 0x48, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_slot_count) == 0x50, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_slots) == 0x58, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_frame_snapshot) == 0x88, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, in_current_tick) == 0xE8, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, supress_prediction) == 0xEA, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, first_prediction) == 0xF0, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, b_buttons_changed) == 0xF4, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_desired_tick) == 0xF8, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_desired_when) == 0xFC, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, b_cmd_changed) == 0x10C, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_transmit_count) == 0x118, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, m_error_count) == 0x130, "cprediction_t wrong offset");
static_assert(offsetof(cprediction_t, needs_reinit) == 0x188, "cprediction_t wrong offset");

