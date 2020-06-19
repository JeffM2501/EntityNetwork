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
const int DefaultHost = 2501;

int main()
{
    std::cout << "Startup\n";

	ENetAddress address = { 0 };

	// any host on default port
	address.host = ENET_HOST_ANY;
	address.port = DefaultHost;

	auto ServerHost = enet_host_create(&address, MaxClients, 3, 0, 0);

	ServerWorld world;
	world.RegisterControllerProperty("name", PropertyDesc::DataTypes::String, 32);
	world.FinalizePropertyData();


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
				auto peer = world.AddRemoteController(peerID);
				connectedPeers.push_back(evt.peer);
			}
			break;

			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
				world.RemoveRemoteController(peerID);
				connectedPeers.erase(std::find(connectedPeers.begin(), connectedPeers.end(), evt.peer));
			break;

			case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
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
				ENetPacket* packet = enet_packet_create(msg->MessageData, msg->MessageLenght, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				msg = world.PopOutboundData(peer->incomingPeerID);
			}
		}
	}
}

