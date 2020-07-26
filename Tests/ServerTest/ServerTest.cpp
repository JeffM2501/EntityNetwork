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
#include <iostream>
#include <thread>
#include <time.h>

#define ENET_IMPLEMENTATION
#include "enet/enet.h"

#include "ServerTest.h"

using namespace EntityNetwork;
using namespace EntityNetwork::Server;


const int MaxClients = 64;
const int DefaultHost = 1701;

ServerWorld TheWorld;

int ClientSpawnRequestRPC;

void SetupWorldData()
{
	// players
	TheWorld.CreateController = [](int64_t id) {return std::dynamic_pointer_cast<ServerEntityController>(std::make_shared<ServerPeer>(id)); };

	TheWorld.RegisterControllerProperty("Name", PropertyDesc::DataTypes::String, 32);
	TheWorld.RegisterControllerProperty("Score", PropertyDesc::DataTypes::Integer);
	TheWorld.ControllerEvents.Subscribe(ServerWorld::ControllerEventTypes::Created, HandleClientCreate);

	// tanks
	EntityDesc::Ptr	tank = EntityDesc::Make();
	tank->Name = "PlayerTank";
	tank->IsAvatar = true;
	tank->CreateScope = EntityDesc::CreateScopes::ServerSync;

	PropertyDesc::Ptr image = PropertyDesc::Make();
	image->Name = "Image";
	image->DataType = PropertyDesc::DataTypes::String;
	image->Scope = PropertyDesc::Scopes::ServerPushSync;
	tank->AddPropertyDesc(image);

	PropertyDesc::Ptr state = PropertyDesc::Make();
	state->Name = "State";
	state->DataType = PropertyDesc::DataTypes::Vector3F;
	state->Scope = PropertyDesc::Scopes::ClientPushSync;

	tank->AddPropertyDesc(state);

	TheWorld.RegisterEntityDesc(tank);

	// map data
	TheWorld.GetWorldPropertyData(TheWorld.RegisterWorldPropertyData("Width", EntityNetwork::PropertyDesc::DataTypes::Integer))->SetValueI(800);
	TheWorld.GetWorldPropertyData(TheWorld.RegisterWorldPropertyData("Height", EntityNetwork::PropertyDesc::DataTypes::Integer))->SetValueI(800);

	MessageBufferBuilder mapObjects(false);

	std::vector<std::string> worldTileIDs;
	worldTileIDs.push_back("crateWood.png");
	worldTileIDs.push_back("crateMetal.png");
	worldTileIDs.push_back("barricadeMetal.png");
	worldTileIDs.push_back("barricadeWood.png");
	worldTileIDs.push_back("treeBrown_large.png");
	worldTileIDs.push_back("treeBrown_small.png");
	worldTileIDs.push_back("treeGreen_large.png");
	worldTileIDs.push_back("treeGreen_small.png");
	mapObjects.AddInt(worldTileIDs.size());
	for (auto& tile : worldTileIDs)
		mapObjects.AddString(tile);

	mapObjects.AddString("tileGrass2.png");

	mapObjects.AddInt(10);
	for (int i = 0; i < 10; i++)
	{
		int id = rand() % worldTileIDs.size();
		int x = rand() % 750 + 25;
		int y = rand() % 750 + 25;
		mapObjects.AddInt(id);
		mapObjects.AddInt(x);
		mapObjects.AddInt(y);
	}
	TheWorld.GetWorldPropertyData(TheWorld.RegisterWorldPropertyData("Map", EntityNetwork::PropertyDesc::DataTypes::Buffer))->SetValueWriter(mapObjects);

	// RPCS
	auto requestSpawn = RemoteProcedureDef::CreateServerSideRPC("RequestSpawn");
	ClientSpawnRequestRPC = TheWorld.RegisterRemoteProcedure(requestSpawn);

	TheWorld.AssignRemoteProcedureFunction(ClientSpawnRequestRPC, HandleSpawnRequest);
}

int main()
{
    std::cout << "Server Startup\n";
	srand(time(nullptr));

	enet_initialize();

	ENetAddress address = { 0 };

	// any host on default port
	address.host = ENET_HOST_ANY;
	address.port = DefaultHost;

	auto ServerHost = enet_host_create(&address, MaxClients, 3, 0, 0);
	if (ServerHost == nullptr)
		return -1;

	SetupWorldData();

	std::vector<ENetPeer*> connectedPeers;

	while (true)
	{
		ENetEvent evt;

		while (enet_host_service(ServerHost, &evt, 2) > 0)
		{
			int64_t peerID = evt.peer->incomingPeerID;
			switch (evt.type)
			{
			case ENetEventType::ENET_EVENT_TYPE_CONNECT:
			{
				std::cout << "Server Peer Connected\n";
				ServerPeer::Ptr peer = ServerPeer::Cast(TheWorld.AddRemoteController(peerID));
				peer->NetworkPeer = evt.peer;
			}
			break;

			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Server Peer Disconnected\n";
				TheWorld.RemoveRemoteController(peerID);
			break;

			case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
				std::cout << "Server Peer Receive Data\n";
				if (evt.channelID == 0)
					TheWorld.AddInboundData(peerID, MessageBuffer::MakeShared(evt.packet->data, evt.packet->dataLength, false));
				else
				{
					// non entity data, must be other game data, like chat or something
					// handle as needed
				}

				enet_packet_destroy(evt.packet);
				break;
				
			case ENetEventType::ENET_EVENT_TYPE_NONE:
				break;
			}
		}

		TheWorld.Update();

		// send any pending outbound sync messages
		TheWorld.RemoteEnitityControllers.DoForEach([](auto& id, ServerEntityController::Ptr& p)
			{
				ServerPeer::Ptr peer = ServerPeer::Cast(p);

				auto msg = p->GetOutbound();
				while (msg != nullptr)
				{
					std::cout << "Server Peer Send Data\n";
					ENetPacket* packet = enet_packet_create(msg->MessageData, msg->MessageLenght, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer->NetworkPeer, 0, packet);
					msg = p->GetOutbound();
				}
			});

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

}

