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
		int ServerWorld::RegisterControllerPropertyDesc(PropertyDesc::Ptr desc)
		{
			World::RegisterControllerPropertyDesc(desc);

			SendToAll(BuildControllerPropertySetupMessage(desc));

			return desc->ID;
		}

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
			RemoteEnitityControllers.Insert(id, ctl);
			SetupEntityController(ctl);

			// let someone fill out the default data
			ControllerEvents.Call(ControllerEventTypes::Created, [ctl](auto func) {func(ctl); });

			// setup any data and properties

			MessageBufferBuilder hail;
			hail.Command = MessageCodes::HailCheck;
			hail.AddString(PROTOCOL_HEADER);
			Send(ctl, hail);

			// RPC defs
			Send(ctl, RPCDefCache);

			// send world data
			Send(ctl, WorldPropertyDefCache);
			MessageBufferBuilder worldDataUpdates;
			worldDataUpdates.Command = MessageCodes::SetWorldDataValues;
			WorldProperties.DoForEach([&worldDataUpdates](PropertyData::Ptr prop) {prop->PackValue(worldDataUpdates); });
			if (!worldDataUpdates.Empty())
				Send(ctl, worldDataUpdates);

			Send(ctl, MessageBufferBuilder(MessageCodes::InitalWorldDataComplete).Pack());

			// entity data
			Send(ctl, EntityDefCache);

			// send controller properties
			Send(ctl, ControllerPropertyCache);
			

			// tell the owner they were accepted and what there ID is.
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AcceptController;
			builder.AddID(id);
			Send(ctl, builder);

		
			// add the new controller and send that message to everyone
			builder.Clear();
			builder.Command = MessageCodes::AddController;
			builder.AddID(id);

			// everyone wants to know new peer's property data
			ctl->Properties.DoForEach([&builder](auto& prop)
				{
					if (prop->Descriptor->TransmitDef())
						prop->PackValue(builder);
				});

			MessageBuffer::Ptr createMessage = builder.Pack();

			RemoteEnitityControllers.DoForEach([this, &builder, &ctl, &createMessage](auto& key, ServerEntityController::Ptr& peer)
				{
					Send(peer, createMessage);

					if (peer != ctl) // tell new peer about everyone else's property data while we are iterating
					{
						builder.Clear();
						builder.Command = MessageCodes::SetControllerPropertyDataValues;
						builder.AddID(peer->ID);

						peer->Properties.DoForEach([&builder](auto& prop)
							{
								if (prop->Descriptor->TransmitDef())
									prop->PackValue(builder);
							});
						Send(ctl, builder);
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

		MessageBuffer::Ptr ServerWorld::BuildControllerPropertySetupMessage(PropertyDesc::Ptr desc)
		{
			if (desc == nullptr || !desc->TransmitDef())
				return nullptr;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AddControllerPropertyDef;
			builder.AddInt(desc->ID);
			builder.AddString(desc->Name);
			builder.AddByte(static_cast<int>(desc->DataType));
			builder.AddByte(static_cast<int>(desc->Scope));
			builder.AddBool(desc->Private);
			auto msg = builder.Pack();
			ControllerPropertyCache.PushBack(msg);

			return msg;
		}

		void ServerWorld::ProcessControllerDataUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			while (!reader.Done())
			{
				int propertyID = reader.ReadInt();
				auto prop = peer->FindPropertyByID(propertyID);
				if (prop == nullptr)
					reader.End();
				else
					prop->UnpackValue(reader, prop->Descriptor->UpdateFromClient());
			}
		}
	}
}