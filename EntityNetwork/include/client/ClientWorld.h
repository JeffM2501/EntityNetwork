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

namespace EntityNetwork
{
	namespace Client
	{
		class ClientWorld : public EntityNetwork::World
		{
		public:

			ClientEntityController::CreateFunction CreateController;

			ClientWorld() : World()
			{
				CreateController = [](int64_t id, bool self) {return std::make_shared<ClientEntityController>(id); };
			}

			virtual void Update();

			virtual void AddInboundData(MessageBuffer::Ptr message);

			virtual  MessageBuffer::Ptr PopOutboundData();
		
			// events
			enum class ControllerEventTypes
			{
				SelfCreated,
				RemoteCreated,
				RemoteDestroyed,
			};
			EventList<ControllerEventTypes, std::function<void(ClientEntityController::Ptr)>> ControllerEvents;

			enum class PropertyEventTypes
			{
				SelfPropteryChanged,
				RemoteControllerPropertyChanged,
				ControllerPropertyDefAdded,
				ControllerPropertyDefRemoved,
				WorldPropertyDefAdded,
				WorldPropertyDataChanged,
			};
			EventList<PropertyEventTypes, std::function<void(ClientEntityController::Ptr, int)>> PropertyEvents;

			enum class StateEventTypes
			{
				Negotiating,
				ActiveSyncing,
			};
			EventList<StateEventTypes, std::function<void(StateEventTypes state)>> StateEvents;

		protected:
			MutexedMap<int64_t, ClientEntityController::Ptr>	Peers;	// controllers that are fully synced
			ClientEntityController::Ptr Self; // me

			void Send(MessageBuffer::Ptr message);

			ClientEntityController::Ptr PeerFromID(int64_t id);
		private:
			void HandlePropteryDescriptorMessage(MessageBufferReader& reader);
		};
	}
}