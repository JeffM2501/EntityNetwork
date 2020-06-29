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
#include <string>
#include <regex>

#include "ClientTest.h"

#define ENET_IMPLEMENTATION
#include "enet/enet.h"

ENetPeer* NetClient = nullptr;
ENetHost* RemoteHost = nullptr;
const int DefaultHost = 1701;

ClientWorld WorldData;

bool Quit = false;

FPSTimer RenderTimer(60);
FPSTimer EnityUpdateTimer(15);

void LogSDLError(const std::string& msg) 
{
	std::cout << msg << " error: " << SDL_GetError() << std::endl;
}

std::string getResourcePath(const std::string& subDir) 
{
	//We need to choose the path separator properly based on which
	//platform we're running on, since Windows uses a different
	//separator than most systems
#ifdef _WIN32
	const std::string PATH_SEP = "\\";
#else
	const td::string  PATH_SEP = "/";
#endif
	//This will hold the base resource path: Lessons/res/
	//We give it static lifetime so that we'll only need to call
	//SDL_GetBasePath once to get the executable path
	static std::string baseRes;

	if (baseRes.empty())
	{
		//SDL_GetBasePath will return NULL if something went wrong in retrieving the path
		char* basePath = SDL_GetBasePath();
		if (basePath)
		{
			baseRes = basePath;
			SDL_free(basePath);
		}
		else 
		{
			std::cerr << "Error getting resource path: " << SDL_GetError() << std::endl;
			return "";
		}
		//We replace the last bin/ with res/ to get the the resource path
		size_t pos = baseRes.rfind("x64");
		if (pos != std::string::npos)
			baseRes = baseRes.substr(0, pos) + "TestAssets" + PATH_SEP;
		else
			baseRes = baseRes + "TestAssets" + PATH_SEP;
	}

	auto osPath = std::regex_replace(subDir, std::regex("\\/"), PATH_SEP);

	//If we want a specific subdirectory path in the resource directory
	//append it to the base path. This would be something like Lessons/res/Lesson0
	return subDir.empty() ? baseRes : baseRes + osPath;
}

bool InitNetworking()
{
	enet_initialize();

	ENetAddress address = { 0 };
	enet_address_set_host(&address, "127.0.0.1");
	address.port = DefaultHost;

	RemoteHost = enet_host_create(nullptr, 1, 3, 0, 0);
	if (RemoteHost != nullptr)
		NetClient = enet_host_connect(RemoteHost, &address, 3, 0);

	return NetClient != nullptr;
}

int serverRPCID = -1;

void InitEntNet()
{
	WorldData.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::SelfCreated, [](ClientEntityController::Ptr client)
		{
			std::cout << " Client Self Created " << client->GetID() << "\n";
			if (serverRPCID != -1)
			{
				auto args = WorldData.GetRPCArgs(serverRPCID);
				args[0]->SetValueI(1234);
				WorldData.CallRPC(serverRPCID, args);
			}

		});
	WorldData.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::RemoteCreated, [](ClientEntityController::Ptr client) {std::cout << "\tRemote Created " << client->GetID() << "\n"; });
	WorldData.ControllerEvents.Subscribe(ClientWorld::ControllerEventTypes::RemoteDestroyed, [](ClientEntityController::Ptr client) {std::cout << "\tRemote Destroyed " << client->GetID() << "\n"; });

	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::ControllerPropertyDefAdded, [](ClientEntityController::Ptr client, int index) {std::cout << "\tController Property def added " << index << "\n"; });
	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::RemoteControllerPropertyChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tRemote Property changed " << index << "\n"; });
	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::SelfPropteryChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tSelf Property changed " << index << "\n"; });
	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::WorldPropertyDefAdded, [](ClientEntityController::Ptr client, int index) {std::cout << "\tWorld Property def added " << index << "\n"; });
	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::WorldPropertyDataChanged, [](ClientEntityController::Ptr client, int index) {std::cout << "\tWorld Property data changed " << index << "\n"; });


	WorldData.PropertyEvents.Subscribe(ClientWorld::PropertyEventTypes::RPCRegistered, [](ClientEntityController::Ptr, int index)
		{
			auto proc = WorldData.GetRPCDef(index);
			if (proc->RPCDefintion.Name == "serverSideRPC1")
				serverRPCID = index;
			std::cout << " Server Registered RPC " << index << " " << proc->RPCDefintion.Name << "\n";
		});

	WorldData.AssignRemoteProcedureFunction("clientRPC1", [](const std::vector<PropertyData::Ptr>& args)
		{
			std::cout << " Server " << " triggered client rpc1 with data " << args[0]->GetValueStr() << "\n";
		});

}

void Cleanup()
{
	if (Renderer != nullptr)
	{
		SDL_DestroyRenderer(Renderer);
		Renderer = nullptr;
	}

	if (Window != nullptr)
	{
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}

	if (NetClient != nullptr)
	{
		enet_peer_disconnect_now(NetClient, -1);
		NetClient = nullptr;
	}

	if (RemoteHost != nullptr)
	{
		enet_host_destroy(RemoteHost);
		RemoteHost = nullptr;
	}

	SDL_Quit();
}

void UpdateEntNet()
{
	ENetEvent evt;

	while (enet_host_service(RemoteHost, &evt, 2) > 0)
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
			Quit = true;
			return;

		case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
			std::cout << "Client Data Receive\n";
			if (evt.channelID == 0)
				WorldData.AddInboundData(MessageBuffer::MakeShared(evt.packet->data, evt.packet->dataLength, false));
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

	WorldData.Update();

	// send any pending outbound sync messages
	auto msg = WorldData.PopOutboundData();
	while (msg != nullptr)
	{
		ENetPacket* packet = enet_packet_create(msg->MessageData, msg->MessageLenght, ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(NetClient, 0, packet);
		msg = WorldData.PopOutboundData();
	}
}


void UpdateSDL()
{
	SDL_Event sdlEvent;
	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		//User requests quit
		switch (sdlEvent.type)
		{
		case SDL_QUIT:
			Quit = true;
			return;

		case SDL_WINDOWEVENT:
			switch (sdlEvent.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				WindowRect.w = sdlEvent.window.data1;
				WindowRect.h = sdlEvent.window.data2;
				break;
			default:
				break;
			}

		default:
			break;
		} 
	}
	if (RenderTimer.Update())
		RenderGraph();

	RenderTimer.Delay();
}

int main(int argc, char**argv)
{
	std::cout << "Client Startup\n";

	if (!InitGraph() || !InitNetworking())
	{
		Cleanup();
		return -1;
	}
	InitEntNet();

	while (!Quit)
	{
		UpdateEntNet();
		UpdateSDL();
	}
	return 0;
}

