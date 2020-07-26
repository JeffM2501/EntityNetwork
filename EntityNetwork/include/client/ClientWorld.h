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
#include "client/ClientEntityController.h"
#include "MutexedMessageBuffer.h"
#include "MutexedMap.h"
#include "MutexedVector.h"
#include "EventList.h"
#include "Entity.h"

namespace EntityNetwork
{
	namespace Client
	{
		typedef std::function<void(const std::vector<PropertyData::Ptr>&)> ClientRPCFunction;

		class ClientWorld : public EntityNetwork::World
		{
		public:
			class ClientRPCDef
			{
			public:
				RemoteProcedureDef RPCDefintion;
				ClientRPCFunction RPCFunction;

				typedef std::shared_ptr<ClientRPCDef> Ptr;
			};

			// called by the system any time it needs to create controller or entity instance objects
			// derived clients can return derived classes of they want to hang other data off it
			ClientEntityController::CreateFunction CreateController;
			EntityInstance::CreateFunction CreateEntityInstance;

			ClientWorld() : World()
			{
				CreateController = [](int64_t id, bool self) {return std::make_shared<ClientEntityController>(id); };
				CreateEntityInstance = [](EntityDesc::Ptr desc, int64_t id) { auto e = EntityInstance::Make(desc); e->SetID(id); return e; };
			}

			// process any dirty data and build up any outbound data that needs to go out
			virtual void Update();

			// called to add data packets from the server
			virtual void AddInboundData(MessageBuffer::Ptr message);

			// remove one outbound message that the library expects to be sent form the server, nullptr if no data is left
			virtual  MessageBuffer::Ptr PopOutboundData();
		
			// events
			enum class ControllerEventTypes
			{
				SelfCreated,						// client has connected and been given a controller ID
				RemoteCreated,						// a remote player has been created
				RemoteDestroyed,					// a remote player has been destroyed
			};
			EventList<ControllerEventTypes, std::function<void(ClientEntityController::Ptr)>> ControllerEvents;

			enum class PropertyEventTypes
			{
				SelfPropteryChanged,				// a property of our local controller has changed
				RemoteControllerPropertyChanged,	// a property of a remote controller has changed
				ControllerPropertyDefAdded,			// a new property type for controllers has been registered
				WorldPropertyDefAdded,				// a new property type for the world has been registered
				WorldPropertyDataChanged,			// data for a world property has changed
				InitialWorldPropertyDataComplete,	// the initial world data is complete
				RPCRegistered,						// a new Remote Procedure Call has been registered
				EntityDefAdded,						// a new entity type definition has been registered
			};
			EventList<PropertyEventTypes, std::function<void(ClientEntityController::Ptr, int)>> PropertyEvents;

			enum class StateEventTypes
			{
				Negotiating,						// The server has connected but is still sending out all definitions and global data
				ActiveSyncing,						// The connection is working in normal mode, syncing entities as needed
				Disconnected,						// The connection is not active
			};
			EventList<StateEventTypes, std::function<void(StateEventTypes state)>> StateEvents;

			StateEventTypes CurrentState = StateEventTypes::Disconnected;

			enum class EntityEventTypes
			{
				EntityAdded,						// An entity instance has been created from the server
				EntityRemoved,						// An entity instance has been removed
				EntityAccepted,						// A locally created entity has been accepted by the server and been given a global ID
				EntityUpdated,						// Data for an entity instance has been changed
			};
			EventList<EntityEventTypes, std::function<void(EntityInstance::Ptr entity)>> EntityEvents;

			// remote procedure calls

			// Assigns a local function to be called when the server triggers a named RPC
			virtual void AssignRemoteProcedureFunction(const std::string& name, ClientRPCFunction function);
			virtual void AssignRemoteProcedureFunction(int id, ClientRPCFunction function);

			// finds an RPC definition by name/id
			ClientRPCDef::Ptr GetRPCDef(int index);
			ClientRPCDef::Ptr GetRPCDef(const std::string& name);

			// build up an empty set of arguments for an RPC using its data definition
			std::vector<PropertyData::Ptr> GetRPCArgs(int index);
			std::vector<PropertyData::Ptr> GetRPCArgs(const std::string& name);

			// Call a RPC on the server using the specified arguments
			virtual bool CallRPC(int index, const std::vector<PropertyData::Ptr>& args);
			virtual bool CallRPC(const std::string& name, const std::vector<PropertyData::Ptr>& args);

			// Create an entity instance and sync it with the server (if the definition allows it), returns local instance ID (<0), for synced entities the server will change the ID to a global ID with the accept event (ID >=0)
			virtual int64_t CreateInstance(int entityTypeID);
			virtual int64_t CreateInstance(const std::string& entityType);

			// removes an entity that is locally controlled
			virtual bool RemoveInstance(int64_t entityID);

			// entities
			MutexedMap<int64_t, EntityInstance::Ptr> EntityInstances;

			// known peer controllers
			MutexedMap<int64_t, ClientEntityController::Ptr>	Peers;	// controllers that are fully synced
			ClientEntityController::Ptr Self; // me

			void RegisterEntityFactory(int64_t id, EntityInstance::CreateFunction function);
			void RegisterEntityFactory(const std::string& name, EntityInstance::CreateFunction function);

			std::vector< EntityInstance::Ptr> GetEntitiesOfType(int64_t typeID);
			std::vector< EntityInstance::Ptr> GetEntitiesOfType(const std::string& typeID);

		protected:
			void Send(MessageBuffer::Ptr message);

			ClientEntityController::Ptr PeerFromID(int64_t id);

			virtual void ExecuteRemoteProcedureFunction(int index, std::vector<PropertyData::Ptr>& arguments);

			void ProcessAddController(MessageBufferReader& reader);
			void ProcessSetControllerPropertyData(MessageBufferReader& reader);
			void ProcessRPC(MessageBufferReader& reader);
			void ProcessAddEntity(MessageBufferReader& reader);
			void ProcessRemoveEntity(MessageBufferReader& reader);
			void ProcessAcceptClientAddEntity(MessageBufferReader& reader);
			void ProcessEntityDataChange(MessageBufferReader& reader);

			MutexedVector<std::shared_ptr<ClientRPCDef>> RemoteProcedures;
			std::map<std::string, ClientRPCFunction> CacheedRPCFunctions;

		private:
			void HandlePropteryDescriptorMessage(MessageBufferReader& reader);

			bool SavePropertyUpdate(EntityInstance::Ptr inst, int propertyID);

			void ProcessLocalEntities();

			int64_t GetNewEntityLocalID();
			int64_t LastLocalID = 0;
			std::vector<EntityInstance::Ptr>	NewLocalEntities;
			std::vector<int64_t>				DeadLocalEntities;

			std::map<int64_t, EntityInstance::CreateFunction> EntityFactories;
			std::map<std::string, EntityInstance::CreateFunction> PendingEntityFactories;

			EntityInstance::Ptr NewEntityInstance(EntityDesc::Ptr desc, int64_t id);
	};
	}
}