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

namespace EntityNetwork
{
	namespace Server
	{
		class ServerWorld : public EntityNetwork::World
		{
		public:

			ServerEntityController::CreateFunction CreateController;

			ServerWorld() : World()
			{
				CreateController = [](int64_t id) {return std::make_shared<ServerEntityController>(id); };
			}

			virtual ServerEntityController::Ptr AddRemoteController(int64_t id = -1);
			virtual void RemoveRemoteController(int64_t id);

			virtual void Update();

			virtual void AddInboundData(int64_t id, MessageBuffer::Ptr message);

			virtual  MessageBuffer::Ptr PopOutboundData(int64_t id);

			virtual int RegisterControllerPropertyDesc(PropertyDesc& desc);

			virtual void FinalizePropertyData();

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
			void SetupPropertyCache();

			MessageBuffer::Ptr BuildControllerPropertySetupMessage(PropertyDesc& desc);

			void SendToAll(MessageBuffer::Ptr message);
		};
	}
}