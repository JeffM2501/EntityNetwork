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

#include "World.h"
#include "server/ServerEntityController.h"
#include "MutexedMessageBuffer.h"
#include "MutexedMap.h"
#include "MutexedVector.h"
#include "EventList.h"
#include <functional>

namespace EntityNetwork
{
	namespace Server
	{
		typedef std::function<void(ServerEntityController::Ptr, const std::vector<PropertyData::Ptr>&)> ServerRPCFunction;

		class ServerWorld : public EntityNetwork::World
		{
		public:
			class ServerRPCDef
			{
			public:
				RemoteProcedureDef RPCDefintion;
				ServerRPCFunction RPCFunction;

				typedef std::shared_ptr<ServerRPCDef> Ptr;
			};

			// called by the system any time it needs to create controller or entity instance objects
			// derived servers can return derived classes of they want to hang other data off it
			ServerEntityController::CreateFunction CreateController;
			EntityInstance::CreateFunction CreateEntityInstance;

			ServerWorld() : World()
			{
				CreateController = [](int64_t id) {return std::make_shared<ServerEntityController>(id); };
				CreateEntityInstance = [](const EntityDesc& desc, int64_t id) { auto e = EntityInstance::MakeShared(desc); e->SetID(id); return e; };
			}

			// external servicing
			// process any dirty data and build up any outbound data that needs to go out
			virtual void Update();

			// called when a client connects to the server, client can assign the ID, or the library can compute it. returns a smart pointer to the controller associated with this client conenction
			// Id will be used for all other external events
			virtual ServerEntityController::Ptr AddRemoteController(int64_t id = -1);

			// called when a client disconnects from the server
			virtual void RemoveRemoteController(int64_t id);

			// called to add data packets for a specific client ID
			virtual void AddInboundData(int64_t id, MessageBuffer::Ptr message);

			// remove one outbound message that the library expects to be sent to the client with the specified ID, nullptr if no data is left
			virtual MessageBuffer::Ptr PopOutboundData(int64_t id);

			// properties

			// register a world (global) data property definition with the server. use GetWorldPropertyData to read/write data to sync to clients
			virtual int RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize = 0);

			// register a field that will be attached to all controllers (players)
			virtual int RegisterControllerPropertyDesc(PropertyDesc& desc);

			// remote procedure calls

			// register a remote procedure call defitnion to send to all clients
			virtual int RegisterRemoteProcedure(RemoteProcedureDef& desc);

			// associate a local function to a RPC name/ID that will be called when the RPC is triggered by the client.
			virtual void AssignRemoteProcedureFunction(const std::string& name, ServerRPCFunction function);
			virtual void AssignRemoteProcedureFunction(int id, ServerRPCFunction function);

			// finds an RPC definition by name/id
			ServerRPCDef::Ptr GetRPCDef(int index);
			ServerRPCDef::Ptr GetRPCDef(const std::string& name);

			// build up an empty set of arguments for an RPC using its data definition
			std::vector<PropertyData::Ptr> GetRPCArgs(int index);
			std::vector<PropertyData::Ptr> GetRPCArgs(const std::string& name);

			// Call a RPC on the server using the specified arguments
			virtual bool CallRPC(int index, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args);
			virtual bool CallRPC(const std::string& name, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args);

			// entities

			// register an entity definition
			virtual int RegisterEntityDesc(EntityDesc& desc);

			// All created entities
			MutexedMap<int64_t, EntityInstance::Ptr> EntityInstances;

			// create an entity instance by name/ID for a specific owner. Sync the entity to clients if it is not server local only.
			virtual int64_t CreateInstance(int entityTypeID, int64_t ownerID);
			virtual int64_t CreateInstance(const std::string& entityType, int64_t ownerID);

			virtual bool RemoveInstance(int64_t entityID);

			// events
			enum class ControllerEventTypes
			{
				Created,				// a controller (player) has been added
				Destroyed,				// a controller has been removed
				RemoteUpdate,			// a client has updated a client writable field on the controller
			};
			EventList<ControllerEventTypes, std::function<void(ServerEntityController::Ptr)>> ControllerEvents;

			enum class EntityEventTypes
			{
				EntityAdded,						// An entity instance has been created from the server
				EntityRemoved,						// An entity instance has been removed
				EntityAccepted,						// A locally created entity has been accepted by the server and been given a global ID
				EntityUpdated,						// Data for an entity instance has been changed
			};
			EventList<EntityEventTypes, std::function<void(EntityInstance::Ptr entity)>> EntityEvents;

		protected:
			MutexedMap<int64_t, ServerEntityController::Ptr>	RemoteEnitityControllers;	// controllers that are fully synced

			MutexedVector<MessageBuffer::Ptr> ControllerPropertyCache;
			MutexedVector<MessageBuffer::Ptr> WorldPropertyDefCache;
			MutexedVector<MessageBuffer::Ptr> RPCDefCache;
			MutexedVector<MessageBuffer::Ptr> EntityDefCache;
			MutexedVector<std::shared_ptr<ServerRPCDef>> RemoteProcedures;

			MessageBuffer::Ptr BuildControllerPropertySetupMessage(PropertyDesc& desc);
			MessageBuffer::Ptr BuildWorldPropertySetupMessage(int index);
			MessageBuffer::Ptr BuildRPCSetupMessage(int index);
			MessageBuffer::Ptr BuildEntityDefMessage(int index);
			MessageBuffer::Ptr BuildWorldPropertyDataMessage(int index);

			virtual void Send(ServerEntityController::Ptr peer, MessageBuffer::Ptr message);
			virtual void Send(ServerEntityController::Ptr peer, MutexedVector<MessageBuffer::Ptr>& messages);
			inline void Send(ServerEntityController::Ptr peer, MessageBufferBuilder& builder) { Send(peer, builder.Pack()); }
			void SendToAll(MessageBuffer::Ptr message);

			virtual void ExecuteRemoteProcedureFunction(int index, ServerEntityController::Ptr sender, std::vector<PropertyData::Ptr>& arguments);

			virtual void ProcessEntityUpdates();
			virtual void ProcessRPCall(ServerEntityController::Ptr peer, MessageBufferReader& reader);
			virtual void ProcessControllerDataUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader);

			virtual void ProcessClientEntityAdd(ServerEntityController::Ptr peer, MessageBufferReader& reader);
			virtual void ProcessClientEntityRemove(ServerEntityController::Ptr peer, MessageBufferReader& reader);
			virtual void ProcessClientEntityUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader);
		};
	}
}