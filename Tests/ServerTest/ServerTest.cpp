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

#define ENET_IMPLEMENTATION
#include "enet/enet.h"

#include "EntityNetwork.h"

using namespace EntityNetwork;
using namespace EntityNetwork::Server;


const int MaxClients = 64;
const int DefaultHost = 1701;

int main()
{
    std::cout << "Server Startup\n";

	enet_initialize();

	ENetAddress address = { 0 };

	// any host on default port
	address.host = ENET_HOST_ANY;
	address.port = DefaultHost;

	auto ServerHost = enet_host_create(&address, MaxClients, 3, 0, 0);
	if (ServerHost == nullptr)
		return -1;

	ServerWorld world;
	world.RegisterControllerProperty("name", PropertyDesc::DataTypes::String, 32);

	EntityDesc eDesc;
	eDesc.Name = "sample_ent";
	eDesc.CreateScope = EntityDesc::CreateScopes::ServerSync;
	eDesc.AddPropertyDesc("value", PropertyDesc::DataTypes::Integer);
	world.RegisterEntityDesc(eDesc);
	
	int propID = world.RegisterWorldPropertyData("WorldProp1", EntityNetwork::PropertyDesc::DataTypes::Integer);
	world.GetWorldPropertyData(propID)->SetValueI(1);

	// in 5 seconds change the world data property to make sure it gets sent out
	auto testThread = std::thread([&world, propID]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			std::cout << "Server Trigger Property Change\n";
			auto prop = world.GetWorldPropertyData(propID);
			prop->SetValueI(5);
		});

	int clientProcID = world.RegisterRemoteProcedure(RemoteProcedureDef::CreateClientSideRPC("clientRPC1", true).DefineArgument(PropertyDesc::DataTypes::String));
	int serverProcID = world.RegisterRemoteProcedure(RemoteProcedureDef::CreateServerSideRPC("serverSideRPC1").DefineArgument(PropertyDesc::DataTypes::Integer));
	
	world.AssignRemoteProcedureFunction(serverProcID, [&world, propID, clientProcID](ServerEntityController::Ptr sender, const std::vector<PropertyData::Ptr>& args)
	{
		std::cout << " Client " << sender->GetID() << " triggered server rpc1 with value " << args[0]->GetValueI() << "\n";
		world.GetWorldPropertyData(propID)->SetValueI(world.GetWorldPropertyData(propID)->GetValueI() + 2);

		auto args2 = world.GetRPCArgs(clientProcID);
		args2[0]->SetValueStr("Test Value " + std::to_string(sender->GetID()));
		world.CallRPC(clientProcID, sender, args2);
	});

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
				auto peer = world.AddRemoteController(peerID);
				connectedPeers.push_back(evt.peer);
			}
			break;

			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Server Peer Disconnected\n";
				world.RemoveRemoteController(peerID);
				connectedPeers.erase(std::find(connectedPeers.begin(), connectedPeers.end(), evt.peer));
			break;

			case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
				std::cout << "Server Peer Receive Data\n";
				if (evt.channelID == 0)
					world.AddInboundData(peerID, MessageBuffer::MakeShared(evt.packet->data, evt.packet->dataLength, false));
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

		world.Update();

		// send any pending outbound sync messages
		for (auto peer : connectedPeers)
		{
			auto msg = world.PopOutboundData(peer->incomingPeerID);
			while (msg != nullptr)
			{
				std::cout << "Server Peer Send Data\n";
				ENetPacket* packet = enet_packet_create(msg->MessageData, msg->MessageLenght, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				msg = world.PopOutboundData(peer->incomingPeerID);
			}
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	testThread.join();
}

