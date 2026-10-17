#pragma once
#include "../../source2-sdk/classes/convars/convars.hpp"

class user_cmd_t;
class CConVar;
namespace misc {
	void run();
	void grenade_trail();
	void update_velocity();
	void practice_move(user_cmd_t* cmd);
	void practice_request(bool load);
	void grenade_pred(user_cmd_t* cmd);
}
