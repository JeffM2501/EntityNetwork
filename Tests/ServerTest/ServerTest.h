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

	static inline Ptr Cast(ServerEntityController::Ptr p)
	{
		return std::dynamic_pointer_cast<ServerPeer>(p);
	}
};

void HandleSpawnRequest(ServerEntityController::Ptr sender, const std::vector<PropertyData::Ptr>& args);

void HandleClientCreate(ServerEntityController::Ptr sender);

extern ServerWorld TheWorld;