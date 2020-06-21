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

namespace EntityNetwork
{
	namespace Server
	{
		ServerEntityController::Ptr ServerWorld::AddRemoteController(int64_t id)
		{
			static std::mutex addMutex;	// simple lock to ensure that only one ID gets added at a time, should be a property of the class

			MutexGuardian guard(addMutex);

			// find an new ID and insert it
			if (id < 0)
			{
				auto findID = [&id](auto& key, auto& ctl) {if (key > id) id = key; };
				RemoteEnitityControllers.DoForEach(findID);
				id++;
			}
			ServerEntityController::Ptr ctl = CreateController(id);
			ctl = RemoteEnitityControllers.Insert(id, ctl);

			// setup any data and properties

			// send world data
			ctl->OutboundMessages.AppendRange(WorldPropertyDefCache);
			MessageBufferBuilder worldDataUpdates;
			worldDataUpdates.Command = MessageCodes::SetWorldDataValues;
			WorldProperties.DoForEach([&worldDataUpdates](PropertyData::Ptr prop){prop->PackValue(worldDataUpdates);});
			if (!worldDataUpdates.Empty())
				ctl->OutboundMessages.Push(worldDataUpdates.Pack());

			// send controller properties
			ctl->OutboundMessages.AppendRange(ControllerPropertyCache);
			SetupEntityController(static_cast<EntityController&>(*ctl));

			// tell the owner they were accepted and what there ID is.
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AcceptController;
			builder.AddID(id);
			ctl->OutboundMessages.Push(builder.Pack());


			// let someone fill out the default data
			ControllerEvents.Call(ControllerEventTypes::Created, [ctl](auto func) {func(ctl);});

			// add the new controller and send that message to everyone
			builder.Clear();
			builder.Command = MessageCodes::AddController;
			builder.AddID(id);

			// everyone wants to know new peer's property data
			ctl->Properties.DoForEach([&builder](auto& prop)
				{
					if (prop->Descriptor.TransmitDef())
						prop->PackValue(builder);
				});

			MessageBuffer::Ptr createMessage = builder.Pack();

			RemoteEnitityControllers.DoForEach([&builder,&ctl,&createMessage](auto& key, ServerEntityController::Ptr& peer)
				{
					peer->OutboundMessages.Push(createMessage); // tell everyone about new peer

					if (peer != ctl) // tell new peer about everyone else's property data while we are iterating
					{
						builder.Clear();
						builder.Command = MessageCodes::SetControllerPropertyValues;
						builder.AddID(peer->ID);

						peer->Properties.DoForEach([&builder](auto& prop)
							{
								if (prop->Descriptor.TransmitDef())
									prop->PackValue(builder);
							});
						ctl->OutboundMessages.Push(builder.Pack());
					}
				});
			return ctl;
		}

		void ServerWorld::RemoveRemoteController(int64_t id)
		{
			auto p = RemoteEnitityControllers.Find(id);
			if (p == nullptr)
				return;

			RemoteEnitityControllers.Remove(id);

			auto entPtr = *p;

			// let anyone cleanup special data
			ControllerEvents.Call(ControllerEventTypes::Destroyed, [entPtr](auto func) {func(entPtr); });

			// tell everyone about the removal
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::RemoveController;
			builder.AddID(entPtr->GetID());
			SendToAll(builder.Pack());
		}

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
						builder.Command = MessageCodes::SetControllerPropertyValues;
						builder.AddID(peer->GetID());
						for (auto prop : dirtyProps)
						{
							if (prop->Descriptor.Private) // we don't replicate private data
								continue;

							builder.AddInt(prop->Descriptor.ID);
							prop->PackValue(builder);
						}
						pendingGlobalUpdates.push_back(builder.Pack());
					}
					
				});

			for (auto msg : pendingGlobalUpdates)
				SendToAll(msg);

			// find any new entities around each avatar and send those

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

				case MessageCodes::SetControllerPropertyValues:
				{
					while (!reader.Done())
					{
						int propertyID = reader.ReadInt();
						auto prop = peer->FindPropertyByID(propertyID);
						if (prop == nullptr)
							reader.End();
						else
							prop->UnpackValue(reader, prop->Descriptor.UpdateFromClient());
					}
				}
				break;

				// server can't get these, it only sends them
				case MessageCodes::AddControllerProperty:
				case MessageCodes::RemoveControllerProperty:
				case MessageCodes::RemoveController:
				case MessageCodes::AcceptController:
				case MessageCodes::AddController:
				case MessageCodes::NoOp:
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

		int ServerWorld::RegisterControllerPropertyDesc(PropertyDesc& desc)
		{
			World::RegisterControllerPropertyDesc(desc);

			SendToAll(BuildControllerPropertySetupMessage(desc));

			return desc.ID;
		}

		void ServerWorld::FinalizePropertyData()
		{
			//SetupPropertyCache();
		}

		MessageBuffer::Ptr ServerWorld::BuildControllerPropertySetupMessage(PropertyDesc& desc)
		{
			if (!desc.TransmitDef())
				return nullptr;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AddControllerProperty;
			builder.AddInt(desc.ID);
			builder.AddString(desc.Name);
			builder.AddByte(static_cast<int>(desc.DataType));
			builder.AddByte(static_cast<int>(desc.Scope));
			builder.AddBool(desc.Private);
			auto msg = builder.Pack();
			ControllerPropertyCache.PushBack(msg);

			return msg;
		}

		void ServerWorld::SetupPropertyCache()
		{
			EntityControllerProperties.DoForEach([this](auto& prop) {BuildControllerPropertySetupMessage(prop); });
		}

		int ServerWorld::RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize)
		{
			int index = World::RegisterWorldPropertyData(name,dataType,dataSize);

			SendToAll(BuildWorldPropertySetupMessage(index));
			SendToAll(BuildWorldPropertyDataMessage(index));

			return index;
		}

		MessageBuffer::Ptr ServerWorld::BuildWorldPropertySetupMessage(int index)
		{
			PropertyData::Ptr data = WorldProperties.Get(index);

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AddWordDataDef;
			builder.AddInt(index);
			builder.AddString(data->Descriptor.Name);
			builder.AddByte(static_cast<int>(data->Descriptor.DataType));
			auto msg = builder.Pack();
			WorldPropertyDefCache.PushBack(msg);

			return msg;
		}

		MessageBuffer::Ptr ServerWorld::BuildWorldPropertyDataMessage(int index)
		{
			auto data = WorldProperties[index];

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::SetWorldDataValues;
			data->PackValue(builder);
			auto msg = builder.Pack();
			return msg;
		}

		void ServerWorld::SendToAll(MessageBuffer::Ptr message)
		{
			RemoteEnitityControllers.DoForEach([&message](auto& key, ServerEntityController::Ptr& peer)
				{
					peer->OutboundMessages.Push(message);
				});
		}
	}
}