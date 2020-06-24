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

			ServerEntityController::CreateFunction CreateController;

			ServerWorld() : World()
			{
				CreateController = [](int64_t id) {return std::make_shared<ServerEntityController>(id); };
			}

			// external servicing
			virtual void Update();
			virtual void AddInboundData(int64_t id, MessageBuffer::Ptr message);
			virtual MessageBuffer::Ptr PopOutboundData(int64_t id);

			// world properties

			virtual int RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize = 0);

			// controllers
			virtual ServerEntityController::Ptr AddRemoteController(int64_t id = -1);
			virtual void RemoveRemoteController(int64_t id);

			virtual int RegisterControllerPropertyDesc(PropertyDesc& desc);

			// remote procedure calls
			virtual int RegisterRemoteProcedure(RemoteProcedureDef& desc);

			virtual void AssignRemoteProcedureFunction(const std::string& name, ServerRPCFunction function);
			virtual void AssignRemoteProcedureFunction(int id, ServerRPCFunction function);

			ServerRPCDef::Ptr GetRPCDef(int index);
			ServerRPCDef::Ptr GetRPCDef(const std::string& name);

			std::vector<PropertyData::Ptr> GetRPCArgs(int index);
			std::vector<PropertyData::Ptr> GetRPCArgs(const std::string& name);

			virtual bool CallRPC(int index, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args);
			virtual bool CallRPC(const std::string& name, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args);

			// entity definitions
			virtual int RegisterEntityDesc(EntityDesc& desc);

			// entities
			MutexedMap<int64_t, EntityInstance::Ptr> EntityInstances;

			virtual int64_t CreateInstance(int entityTypeID, int64_t ownerID);
			virtual int64_t CreateInstance(const std::string& entityType, int64_t ownerID);

			// events
			enum class ControllerEventTypes
			{
				Created,
				Destroyed,
				RemoteUpdate,
			};
			EventList<ControllerEventTypes, std::function<void(ServerEntityController::Ptr)>> ControllerEvents;

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
			virtual void ProcessClientEntityUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader);
		};
	}
}