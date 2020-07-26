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
#pragma once

#include "EntityNetwork.h"
#include "enet/enet.h"

using namespace EntityNetwork;
using namespace EntityNetwork::Server;

class ServerPeer : public ServerEntityController
{
public: 
	ENetPeer* NetworkPeer = nullptr;

	int64_t AvatarID = -1;

	typedef std::shared_ptr< ServerPeer> Ptr;

	ServerPeer(int64_t id) : ServerEntityController(id) {}
	virtual ~ServerPeer() {}

	static inline Ptr Cast(ServerEntityController::Ptr p)
	{
		return std::dynamic_pointer_cast<ServerPeer>(p);
	}
};

void HandleSpawnRequest(ServerEntityController::Ptr sender, const std::vector<PropertyData::Ptr>& args);

void HandleClientCreate(ServerEntityController::Ptr sender);

extern ServerWorld TheWorld;