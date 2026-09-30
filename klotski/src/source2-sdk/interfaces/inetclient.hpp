#pragma once
#include "../../utilities/utilities.hpp"
#include "../../utilities/vfunc.hpp"
#include "../classes/trace.hpp"
#include "../math/math.hpp"
#include "../../signatures.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"

class CNetChannelInfo
{
public:
	[[nodiscard]] const char* GetName()
	{
		return 	utilities::call_virtual<0U, const char*>(this);
	}

	[[nodiscard]] const char* GetAddress()
	{
		return 	utilities::call_virtual<1U, const char*>(this);
	}

	[[nodiscard]] float GetTime()
	{
		return 	utilities::call_virtual<2U, float>(this);
	}

	[[nodiscard]] float GetTimeConnected()
	{
		return 	utilities::call_virtual<3U, float>(this);
	}

	[[nodiscard]] float GetBufferSize()
	{
		return 	utilities::call_virtual<4U, float>(this);
	}

	[[nodiscard]] float GetDataRate()
	{
		return 	utilities::call_virtual<5U, float>(this);
	}

	[[nodiscard]] float GetLatency(int nFlow)
	{
		return 	utilities::call_virtual<9U, float>(this, nFlow);
	}

	[[nodiscard]] int GetAverageLatency(int nFlow)
	{
		return	utilities::call_virtual<10U, int>(this, nFlow);
	}

	[[nodiscard]] float GetNetLatency()
	{
		return 	utilities::call_virtual<10, float>(this);
	}
	[[nodiscard]] float GetEngineLatency()
	{
		return 	utilities::call_virtual<11, float>(this);
	}

};

class INetworkClient {
public:
	MEM_PAD(0x90); // 0x0000
	float m_flRealTime; //0x0090
	int m_iFrameCount; //0x0094
	float m_flFrameTime; //0x0098
	float m_flFrameTime2; //0x009C
	int m_iMaxClients; //0x00A0
	MEM_PAD(0x8); // 0x00A4
	float m_flPlayerInterp; //0x00AC
	double m_dbSomeTimer; //0x00B0
	MEM_PAD(0x8); // 0x00B8
	float m_flIntervalPerSubTick; //0x00C0
	float m_flCurrentTime; //0x00C4
	float m_flCurrentTIme2; //0x00C8
	MEM_PAD(0x8); //0x00CC
	bool m_bInPrediction; //0x00D4
	MEM_PAD(0x3); //0x00D5
	int m_iTickCount; //0x00D8
	int m_iTickCount2; //0x00DC
	float m_flSomeTimer; //0x00E0
	MEM_PAD(0x4); //0x00E4
	CNetChannelInfo* m_pNetChannelInfo; // 0x00E8
	MEM_PAD(0x8); // 0x00F0
	bool m_bShouldPredict; // 0x00F8
	MEM_PAD(0x14B); // 0x00F9
	int m_iDeltaTick; // 0x0244
	MEM_PAD(0x124); // 0x0248
	int m_iServerTick; // 0x036C
	MEM_PAD(2642556);
	int m_iClientTick;
	int get_client_tick() {
		return 	utilities::call_virtual<5U, int>(this);
	}

	int get_server_tick() {
		return 	utilities::call_virtual<6U, int>(this);
	}

	void set_prediction(bool value) {
		*(bool*)(std::uintptr_t(this) + 0xF0) = value;
	}

	void set_delta_tick(int tick) {
		*(int*)(std::uintptr_t(this) + 0x23C) = tick;
	}

	SCHEMA_ADD_OFFSET(bool, m_predicted, 0xF0);
	SCHEMA_ADD_OFFSET(int, m_delta_tick, 0x23C);
};

class I_NetworkClientService
{
public:
	INetworkClient* get_network_client() {
		return 	utilities::call_virtual<23U, INetworkClient*>(this);
	}
};