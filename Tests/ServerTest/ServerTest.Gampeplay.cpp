//  Copyright (c) 2020 Jeffery Myers
//
//	EntityNetwork and its associated sub proejcts are free software;
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//	
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
#include "ServerTest.h"


std::string firstNames[] = { "Frost","Sunrise","Shadow","Shadow", "Applesauce", "Lightning", "Tiny", "Shining", "Star", "Fancy", "Applesauce"};
std::string lastNames[] = { "Wing","","Charm","Cube", "Star", "Haste", "Moon", "Dash", "Flower", "Tumbler" };

std::string TankAvatars[] = { "tank_blue", "tank_dark", "tank_green", "tank_red", "tank_sand" };

std::string RandomName(int64_t id)
{
	size_t f = rand() & 11;
	size_t l = rand() & 10;
	return firstNames[f] + lastNames[l] + std::to_string(id);
}

void HandleClientCreate(ServerEntityController::Ptr sender)
{
	auto peer = ServerPeer::Cast(sender);
	auto name = sender->FindPropertyByName("Name");
	if (name != nullptr)
		name->SetValueStr(RandomName(sender->GetID()));

	auto score = sender->FindPropertyByName("Score");
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
				state[0] = (float)(rand() % 750);
				state[1] = (float)(rand() % 750);
				state[2] = (float)((rand() % 360) - 180);

				inst->Properties.Get(1)->SetValue3F(state);
			});
	}
}