#include "ServerTest.h"


std::string firstNames[] = { "Frost","Sunrise","Shadow","Shadow", "Applesauce", "Lightning", "Tiny", "Shining", "Star", "Fancy", "Applesauce"};
std::string lastNames[] = { "Wing","","Charm","Cube", "Star", "Haste", "Moon", "Dash", "Flower", "Tumbler" };

std::string TankAvatars[] = { "tank_blue", "tank_dark", "tank_green", "tank_red", "tank_sand" };

std::string RandomName(int id)
{
	size_t f = rand() & 11;
	size_t l = rand() & 10;
	return firstNames[f] + lastNames[l] + std::to_string(id);
}

void HandleClientCreate(ServerEntityController::Ptr sender)
{
	auto peer = ServerPeer::Cast(sender);
	auto name = peer->FindPropertyByName("Name");
	if (name != nullptr)
		name->SetValueStr(RandomName(peer->GetID()));

	auto score = peer->FindPropertyByName("Score");
	if (score != nullptr)
		score->SetValueI(0);
}

void HandleSpawnRequest(ServerEntityController::Ptr sender, const std::vector<PropertyData::Ptr>& args)
{
	auto peer = ServerPeer::Cast(sender);

	if (peer->AvatarID == -1)
	{
		peer->AvatarID = TheWorld.CreateInstance("PlayerTank", peer->GetID(), [](EntityInstance::Ptr inst) 
			{
				int tankIndex = rand() % 5;
				inst->Properties.Get(0)->SetValueStr(TankAvatars[tankIndex]);

				float state[3] = { 0,0,0 };
				state[0] = rand() % 750;
				state[1] = rand() % 750;
				state[2] = (rand() % 360) - 180;

				inst->Properties.Get(1)->SetValue3F(state);
			});
	}
}