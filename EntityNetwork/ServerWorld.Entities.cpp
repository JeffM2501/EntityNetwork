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

		EntityInstance::Ptr ServerWorld::NewEntityInstance(EntityDesc::Ptr desc, int64_t id)
		{
			auto facItr = EntityFactories.find(desc->ID);
			if (facItr != EntityFactories.end())
				return facItr->second(desc, id);
			return CreateEntityInstance(desc, id);
		}

		void ServerWorld::RegisterEntityFactory(int64_t id, EntityInstance::CreateFunction function)
		{
			EntityFactories[id] = function;
		}

		void ServerWorld::RegisterEntityFactory(const std::string& name, EntityInstance::CreateFunction function)
		{
			auto ent = GetEntityDef(name);

			if (ent != nullptr)
				RegisterEntityFactory(ent->ID, function);
			else
				PendingEntityFactories[name] = function;
		}

		int ServerWorld::RegisterEntityDesc(EntityDesc::Ptr desc)
		{
			if (desc == nullptr)
				return -1;
			desc->ID = static_cast<int>(EntityDefs.Size());
			EntityDefs.Insert(desc->ID, desc);

			SendToAll(BuildEntityDefMessage(desc->ID));

			auto itr = PendingEntityFactories.find(desc->Name);
			if (itr != PendingEntityFactories.end())
				EntityFactories[desc->ID] = itr->second;

			return desc->ID;
		}

		std::vector<EntityInstance::Ptr> ServerWorld::GetEntitiesOfType(int64_t typeID)
		{
			std::vector<EntityInstance::Ptr> ents;

			EntityInstances.DoForEach([&ents, typeID](auto id, EntityInstance::Ptr& ptr)
				{
					if (ptr->Descriptor->ID == typeID)
						ents.push_back(ptr);
				});

			return ents;
		}

		std::vector<EntityInstance::Ptr> ServerWorld::GetEntitiesOfType(const std::string& typeName)
		{
			auto ent = GetEntityDef(typeName);
			if (ent == nullptr)
				return std::vector<EntityInstance::Ptr>();

			return GetEntitiesOfType(ent->ID);
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
				builder.AddInt(prop->ID);
				builder.AddByte(static_cast<int>(prop->Scope));
				builder.AddString(prop->Name);
				builder.AddByte(static_cast<int>(prop->DataType));
			}

			auto msg = builder.Pack();
			EntityDefCache.PushBack(msg);

			return msg;
		}

		int64_t ServerWorld::CreateInstance(int entityTypeID, int64_t ownerID, EntityFunciton setupCallback)
		{
			auto entDef = GetEntityDef(entityTypeID);
			if (entDef == nullptr || !entDef->AllowServerCreate()) // invalid or client only create
				return EntityInstance::InvalidID;

			EntityInstance::Ptr ent = NewEntityInstance(entDef, EntityInstances.Size());
			ent->OwnerID = ownerID;
			EntityInstances.Insert(ent->ID, ent);

			if (setupCallback != nullptr)
				setupCallback(ent);

			ent->Created();
			EntityEvents.Call(EntityEventTypes::EntityAdded, [&ent](auto func) {func(ent); });
			return ent->ID;
		}

		int64_t ServerWorld::CreateInstance(const std::string& entityType, int64_t ownerID, EntityFunciton setupCallback)
		{
			auto def = GetEntityDef(entityType);
			if (def == nullptr)
				return EntityInstance::InvalidID;

			return CreateInstance(def->ID, ownerID, setupCallback);
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

			auto entDef = GetEntityDef(entityTypeID);
			if (entDef == nullptr || !entDef->AllowClientCreate() || !entDef->SyncCreate()) // invalid or server only create
			{
				MessageBufferBuilder denyMessage;
				denyMessage.Command = MessageCodes::AcceptClientEntity;
				denyMessage.AddID(-1);
				denyMessage.AddID(localID);
				Send(peer, denyMessage);
				return;
			}

			EntityInstance::Ptr ent = NewEntityInstance(entDef, EntityInstances.Size());
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
			ent->Created();
			EntityEvents.Call(EntityEventTypes::EntityAdded, [&ent](auto func) {func(ent); });
			
			// send back the acceptance
			MessageBufferBuilder ackMsg;
			ackMsg.Command = MessageCodes::AcceptClientEntity;
			ackMsg.AddID(ent->ID);
			ackMsg.AddID(localID);
			Send(peer, ackMsg);
			EntityEvents.Call(EntityEventTypes::EntityAccepted, [&ent](auto func) {func(ent); });

			// they aways know about this revision, they added the thing. This prevents us from sending the item back to them as an add. The accept message handles that for this client.
			KnownEnityDataset& dataset = peer->KnownEnitities.Insert(ent->ID, KnownEnityDataset());
			ent->Properties.DoForEach([&dataset](PropertyData::Ptr prop)
				{
					dataset.DataRevisions.push_back(prop->GetRevision());
				});
		}

		void ServerWorld::ProcessClientEntityRemove(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			auto entityID = reader.ReadID();
			auto ent = EntityInstances.Find(entityID);
			if (ent == std::nullopt || (*ent)->OwnerID != peer->ID || (*ent)->Descriptor->AllowServerCreate() || !(*ent)->Descriptor->SyncCreate())
				return;

			RemoveInstance(entityID);
		}

		void ServerWorld::ProcessClientEntityUpdate(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			auto entityID = reader.ReadID();
			auto ent = EntityInstances.Find(entityID);
			if (ent == std::nullopt || (*ent)->OwnerID != peer->ID)
				return;

			if (!(*ent)->Descriptor->AllowClientCreate() || !(*ent)->Descriptor->SyncCreate())
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
					(*prop)->UnpackValue(reader, (*prop)->Descriptor->UpdateFromClient());
					(*ent)->PropertyChanged(*prop);
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
								addMsg.AddInt(entity->Descriptor->ID);
								addMsg.AddID(entity->OwnerID);
								
								KnownEnityDataset& dataset = peer->KnownEnitities.Insert(id, KnownEnityDataset());
								entity->Properties.DoForEach([&addMsg, &dataset](PropertyData::Ptr prop) 
									{
										// always pack all values when the server sends an entity
										prop->PackValue(addMsg);
										dataset.DataRevisions.push_back(prop->GetRevision());
									});
								Send(peer, addMsg);
							}
							else
							{
								std::vector<PropertyData::Ptr> dirtyProps;
								int index = 0;
								// check all the properties find ones that have not been sent to the controller
								entity->Properties.DoForEach([&knownEnt, &dirtyProps, &index,&peer,&entity](PropertyData::Ptr prop)
									{
										auto rev = prop->GetRevision();

										bool transmit = prop->Descriptor->TransmitDef();
										if (transmit && prop->Descriptor->Scope == PropertyDesc::Scopes::ClientPushSync)
											transmit = entity->OwnerID == peer->GetID(); // dont send them back updates for a value they pushed to us
											
										if (rev != knownEnt->DataRevisions[index] && transmit) // different and we sync it
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
