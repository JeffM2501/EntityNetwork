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

#include "EntityController.h"
#include "MutexedMessageBuffer.h"
#include "MutexedMap.h"
#include "Entity.h"
#include "server/ServerWorld.h"
#include <mutex>

namespace EntityNetwork
{
	namespace Server
	{
		class ServerEntityController : public EntityController
		{
		public:
			typedef std::shared_ptr<ServerEntityController> Ptr;
			typedef std::function<ServerEntityController::Ptr(int64_t)> CreateFunction;

			MutexedMap<int64_t, KnownEnityDataset> KnownEnitities;

			ServerEntityController(int64_t id) : EntityController(id) {}

			inline virtual void AddInbound(MessageBuffer::Ptr message)
			{
				InboundMessages.Push(message);
			}

			inline virtual MessageBuffer::Ptr GetOutbound()
			{
				return OutboundMessages.Pop();
			}

			inline virtual size_t GetOutboundSize()
			{
				return OutboundMessages.Size();
			}
		protected:
			friend class ServerWorld;

			MutexedMessageBufferDeque InboundMessages;
			MutexedMessageBufferDeque OutboundMessages;
		};
	}
}