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
		int ServerWorld::RegisterEntityDesc(EntityDesc& desc)
		{
			desc.ID = static_cast<int>(EntityDefs.Size());
			SendToAll(BuildEntityDefMessage(desc.ID));
			EntityDefs.Insert(desc.ID, desc);
			return desc.ID;
		}

		MessageBuffer::Ptr ServerWorld::BuildEntityDefMessage(int index)
		{
			auto def = GetEntityDef(index);
			if (def == nullptr)
				return nullptr;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AddEntityDef;
			builder.AddInt(index);
			builder.AddString(def->Name);
			builder.AddBool(def->IsAvatar);
			builder.AddByte(static_cast<int>(def->CreateScope));
			for (auto& prop : def->Properties)
			{
				builder.AddInt(prop.ID);
				builder.AddByte(static_cast<int>(prop.Scope));
				builder.AddString(prop.Name);
				builder.AddByte(static_cast<int>(prop.DataType));
			}

			auto msg = builder.Pack();
			EntityDefCache.PushBack(msg);

			return msg;
		}

		int64_t ServerWorld::CreateInstance(int entityTypeID, int64_t ownerID)
		{
			auto entDef = EntityDefs.Find(entityTypeID);
			if (entDef == std::nullopt || !entDef->AllowServerCreate()) // invalid or client only create
				return EntityInstance::InvalidID;

			EntityInstance::Ptr ent = CreateEntityInstance(*entDef, EntityInstances.Size());
			ent->OwnerID = ownerID;
			EntityInstances.Insert(ent->ID, ent);

			EntityEvents.Call(EntityEventTypes::EntityAdded, [&ent](auto func) {func(ent); });
			return ent->ID;
		}

		int64_t ServerWorld::CreateInstance(const std::string& entityType, int64_t ownerID)
		{
			auto ent = EntityDefs.FindIF([&entityType](const int& id, EntityDesc& desc) {return desc.Name == entityType; });
			if (ent == std::nullopt)
				return EntityInstance::InvalidID;

			return CreateInstance(ent->ID, ownerID);
		}

		bool ServerWorld::RemoveInstance(int64_t entityID)
		{
			auto ent = EntityInstances.Find(entityID);
			if (ent == std::nullopt)
				return false;

			EntityInstances.Remove(entityID);
			EntityEvents.Call(EntityEventTypes::EntityRemoved, [&ent](auto func) {func(*ent); });

			MessageBufferBuilder removeMsg;
			removeMsg.Command = MessageCodes::RemoveEntity;
			removeMsg.AddID(entityID);
			SendToAll(removeMsg.Pack());

			// purge the known entity from the list so we don't try to keep sending it data
			RemoteEnitityControllers.DoForEach([entityID](auto& key, ServerEntityController::Ptr& peer)
				{
					peer->KnownEnitities.Remove(entityID);
				});
			return true;
		}

		void ServerWorld::ProcessClientEntityAdd(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			auto entityTypeID = reader.ReadInt();
			auto localID = reader.ReadID();

			auto entDef = EntityDefs.Find(entityTypeID);
			if (entDef == std::nullopt || !entDef->AllowClientCreate() || !entDef->SyncCreate()) // invalid or server only create
			{
				MessageBufferBuilder denyMessage;
				denyMessage.Command = MessageCodes::AcceptClientEntity;
				denyMessage.AddID(-1);
				denyMessage.AddID(localID);
				Send(peer, denyMessage);
				return;
			}

			EntityInstance::Ptr ent = CreateEntityInstance(*entDef, EntityInstances.Size());
			ent->OwnerID = peer->ID;

			int index = 0;
			while (!reader.Done())
			{
				auto ptr = ent->Properties.TryGet(index);
				if (ptr != nullptr)
				{
					(*ptr)->UnpackValue(reader, true);
				}
				index++;
			}
			EntityInstances.Insert(ent->ID, ent);
			EntityEvents.Call(EntityEventTypes::EntityAdded, [&ent](auto func) {func(ent); });
			
			// send back the acceptance
			MessageBufferBuilder ackMsg;
			ackMsg.Command = MessageCodes::AcceptClientEntity;
			ackMsg.AddID(ent->ID);
			ackMsg.AddID(localID);
			Send(peer, ackMsg);
			EntityEvents.Call(EntityEventTypes::EntityAccepted, [&ent](auto func) {func(ent); });
		}

		void ServerWorld::ProcessClientEntityRemove(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			auto entityID = reader.ReadID();
			auto ent = EntityInstances.Find(entityID);
			if (ent == std::nullopt || (*ent)->OwnerID != peer->ID || (*ent)->Descriptor.AllowServerCreate() || !(*ent)->Descriptor.SyncCreate())
				return;

			RemoveInstance(entityID);
		}

		void ServerWorld::ProcessClientEntityUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			auto entityID = reader.ReadID();
			auto ent = EntityInstances.Find(entityID);
			if (ent == std::nullopt || (*ent)->OwnerID != peer->ID)
				return;

			if (!(*ent)->Descriptor.AllowClientCreate() || !(*ent)->Descriptor.SyncCreate())
			{
				auto known = peer->KnownEnitities.Find(entityID);
				if (known != std::nullopt)
					peer->KnownEnitities[entityID].DataRevisions.clear();
				return;
			}

			while (!reader.Done())
			{
				auto propID = reader.ReadInt();
				auto prop = (*ent)->Properties.TryGet(propID);
				if (prop != nullptr)
				{
					(*prop)->UnpackValue(reader, (*prop)->Descriptor.UpdateFromClient());
				}
			}
			EntityEvents.Call(EntityEventTypes::EntityUpdated, [&ent](auto func) {func(*ent); });
		}

		/*
		Future logic, check avatar entities to see what is near and add them as known entities, remove items outside of range.
		Then just update known entities for each controller
		*/

		void ServerWorld::ProcessEntityUpdates()
		{
			RemoteEnitityControllers.DoForEach([this](auto& key, ServerEntityController::Ptr& peer)
				{
					EntityInstances.DoForEachIf(EntityInstance::CanSyncFunc, [this, &peer](int64_t& id, EntityInstance::Ptr entity)
						{
							auto knownEnt = peer->KnownEnitities.Find(id);
							if (knownEnt == std::nullopt)	// if the client has never seen this entity, send it to them (TODO, check if it's in range once we have spatial)
							{
								MessageBufferBuilder addMsg;
								addMsg.Command = MessageCodes::AddEntity;
								addMsg.AddID(id);
								addMsg.AddInt(entity->Descriptor.ID);
								addMsg.AddID(entity->OwnerID);
								
								KnownEnityDataset& dataset = peer->KnownEnitities.Insert(id, KnownEnityDataset());
								entity->Properties.DoForEach([&addMsg, &dataset](PropertyData::Ptr prop) 
									{
										// always pack all values when the server sends an entity
										prop->PackValue(addMsg); dataset.DataRevisions.push_back(prop->GetRevision());
									});
								Send(peer, addMsg);
							}
							else
							{
								std::vector<PropertyData::Ptr> dirtyProps;
								int index = 0;
								// check all the properties find ones that have not been sent to the controller
								entity->Properties.DoForEach([&knownEnt, &dirtyProps, &index](PropertyData::Ptr prop)
									{
										auto rev = prop->GetRevision();
										if (rev != knownEnt->DataRevisions[index] && prop->Descriptor.TransmitDef()) // different and we sync it
											dirtyProps.push_back(prop);
										knownEnt->DataRevisions[index] = rev;
									});

								MessageBufferBuilder updateMsg;
								updateMsg.Command = MessageCodes::SetEntityDataValues;
								updateMsg.AddID(id);

								for (auto p : dirtyProps)
								{
									p->PackValue(updateMsg);
								}

								Send(peer, updateMsg);
							}
						});
				});
		}
	}
}