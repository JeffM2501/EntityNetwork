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
using namespace EntityNetwork::Client;


const int DefaultHost = 1701;

int main()
{
	std::cout << "Client Startup\n";

	enet_initialize();

	ENetAddress address = { 0 };
	enet_address_set_host(&address, "127.0.0.1");
	address.port = DefaultHost;

	auto remoteHost = enet_host_create(nullptr, 1, 3, 0, 0);
	auto client = enet_host_connect(remoteHost, &address, 3, 0);

	if (client == nullptr)
		return -1;

	ClientWorld world;

 	world.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::SelfCreated, [](ClientEntityController::Ptr client) {std::cout << "\tSelf Created " << client->GetID() << "\n"; });
	world.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::RemoteCreated, [](ClientEntityController::Ptr client) {std::cout << "\tRemote Created " << client->GetID() << "\n"; });
	world.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::RemoteDestroyed, [](ClientEntityController::Ptr client) {std::cout << "\tRemote Destroyed " << client->GetID() << "\n"; });

	world.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::ControllerPropertyDefAdded, [](ClientEntityController::Ptr client, int index) {std::cout << "\tController Property def added " << index << "\n"; });
	world.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::RemoteControllerPropertyChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tRemote Property changed " << index << "\n"; });
 	world.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::SelfPropteryChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tSelf Property changed " << index << "\n"; });
	world.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::WorldPropertyDefAdded, [](ClientEntityController::Ptr client, int index) {std::cout << "\tWorld Property def added " << index << "\n"; });
	world.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::WorldPropertyDataChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tWorld Property data changed " << index << "\n"; });

	while (true)
	{
		ENetEvent evt;

		while (enet_host_service(remoteHost, &evt, 2) > 0)
		{
			int64_t peerID = evt.peer->incomingPeerID;
			switch (evt.type)
			{
			case ENetEventType::ENET_EVENT_TYPE_CONNECT:
			{
				std::cout << "Client Connected\n";
			}
			break;

			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			case ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Client Disconnected\n";
				return 0;

			case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
				std::cout << "Client Data Receive\n";
				if (evt.channelID == 0)
					world.AddInboundData(MessageBuffer::MakeShared(evt.packet->data, evt.packet->dataLength, false));
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
		auto msg = world.PopOutboundData();
		while (msg != nullptr)
		{
			ENetPacket* packet = enet_packet_create(msg->MessageData, msg->MessageLenght, ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(client, 0, packet);
			msg = world.PopOutboundData();
		}
	}
}

