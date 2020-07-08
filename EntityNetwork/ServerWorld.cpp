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
#include "server/ServerWorld.h"
#include "EntityNetwork.h"

namespace EntityNetwork
{
	namespace Server
	{
		void ServerWorld::Update()
		{
			std::vector<MessageBuffer::Ptr> pendingGlobalUpdates;

			// send out any dirty world data updates
			MessageBufferBuilder worldDataUpdates;
			worldDataUpdates.Command = MessageCodes::SetWorldDataValues;
			WorldProperties.DoForEach([this,&worldDataUpdates](PropertyData::Ptr prop)
				{
					if (prop->IsDirty())
						prop->PackValue(worldDataUpdates);

					prop->SetClean();
				});

			if (!worldDataUpdates.Empty())
				pendingGlobalUpdates.push_back(worldDataUpdates.Pack());

			// find all dirty entity controller properties
			
			RemoteEnitityControllers.DoForEach([&pendingGlobalUpdates](auto& key, ServerEntityController::Ptr& peer)
				{
					auto dirtyProps = peer->GetDirtyProperties();
					if (dirtyProps.size() > 0)
					{
						MessageBufferBuilder builder;
						builder.Command = MessageCodes::SetControllerPropertyDataValues;
						builder.AddID(peer->GetID());
						for (auto prop : dirtyProps)
						{
							if (prop->Descriptor->Private) // we don't replicate private data
								continue;

							builder.AddInt(prop->Descriptor->ID);
							prop->PackValue(builder);
						}
						pendingGlobalUpdates.push_back(builder.Pack());
					}
					
				});

			for (auto msg : pendingGlobalUpdates)
				SendToAll(msg);

			ProcessEntityUpdates();
		}

		void ServerWorld::AddInboundData(int64_t id, MessageBuffer::Ptr inbound)
		{
			auto p = RemoteEnitityControllers.Find(id);
			if (p == std::nullopt)
				return;
			ServerEntityController::Ptr& peer = (*p);
			if (inbound != nullptr)
			{
				MessageBufferReader reader(inbound);
				switch (reader.Command)
				{

				case MessageCodes::SetControllerPropertyDataValues:
					ProcessControllerDataUpdate(peer, reader);
				break;

				case MessageCodes::CallRPC:
					ProcessRPCall(peer, reader);
					break;

				case MessageCodes::AddEntity:
					ProcessClientEntityAdd(peer, reader);
					break;

				case MessageCodes::RemoveEntity:
					ProcessClientEntityRemove(peer, reader);
					break;

				case MessageCodes::SetEntityDataValues:
					ProcessClientEntityUpdate(peer, reader);
					break;

				// server can't get these, it only sends them
				case MessageCodes::AddControllerPropertyDef:
				case MessageCodes::RemoveController:
				case MessageCodes::AcceptController:
				case MessageCodes::AddController:
				case MessageCodes::AddRPCDef:
				case MessageCodes::AddEntityDef:
				case MessageCodes::AddWordDataDef:
				case MessageCodes::InitalWorldDataComplete:
				case MessageCodes::NoOp:
				case MessageCodes::NoCode:
				default:
					break;
				}
			};
		}

		MessageBuffer::Ptr ServerWorld::PopOutboundData(int64_t id)
		{
			auto p = RemoteEnitityControllers.Find(id);
			if (p == std::nullopt || (*p)->OutboundMessages.Empty())
				return nullptr;

			return (*p)->OutboundMessages.Pop();
		}

		void ServerWorld::Send(ServerEntityController::Ptr peer, MutexedVector<MessageBuffer::Ptr>& messages)
		{
			if (peer != nullptr)
				peer->OutboundMessages.AppendRange(messages);
		}

		void ServerWorld::Send(ServerEntityController::Ptr peer, MessageBuffer::Ptr message)
		{
			if (peer != nullptr)
				peer->OutboundMessages.Push(message);
		}

		void ServerWorld::SendToAll(MessageBuffer::Ptr message)
		{
			RemoteEnitityControllers.DoForEach([this,&message](auto& key, ServerEntityController::Ptr& peer)
				{
					Send(peer, message);
				});
		}
	}
}