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
#include "client/ClientWorld.h"
#include "MutexedMessageBuffer.h"
#include "EntityDescriptor.h"
#include "Entity.h"

namespace EntityNetwork
{
	namespace Client
	{
		EntityInstance::Ptr ClientWorld::NewEntityInstance(const EntityDesc& desc, int64_t id)
		{
			auto facItr = EntityFactories.find(desc.ID);
			if (facItr != EntityFactories.end())
				return facItr->second(desc, id);
			return CreateEntityInstance(desc, id);
		}

		void ClientWorld::RegisterEntityFactory(int64_t id, EntityInstance::CreateFunction function)
		{
			EntityFactories[id] = function;
		}

		void ClientWorld::RegisterEntityFactory(const std::string& name, EntityInstance::CreateFunction function)
		{
			auto ent = EntityDefs.FindIF([&name](const int& id, EntityDesc& desc) {return desc.Name == name; });

			if (ent != std::nullopt)
				RegisterEntityFactory(ent->ID, function);
			else
				PendingEntityFactories[name] = function;
		}

		void ClientWorld::ProcessAddEntity(MessageBufferReader& reader)
		{
			auto id = reader.ReadID();
			auto type = reader.ReadInt();
			auto owner = reader.ReadInt();

			auto desc = GetEntityDef(type);
			if (desc == nullptr || desc->AllowClientCreate() || !desc->SyncCreate())	// we are not supposed to get this from the remote
				return;
			
			EntityInstance::Ptr inst = NewEntityInstance(*desc, id);;
			inst->OwnerID = owner;
			
			int index = 0;
			while (!reader.Done())
			{
				if (index < inst->Properties.Size())
				{
					inst->Properties[index]->UnpackValue(reader, true);
				}

				index++;
			}

			EntityInstances.Insert(id, inst);
			EntityEvents.Call(EntityEventTypes::EntityAdded, [&inst](auto func) {func(inst);});
		}

		void ClientWorld::ProcessRemoveEntity(MessageBufferReader& reader)
		{
			auto id = reader.ReadID();
			auto inst = EntityInstances.Find(id);
			if (inst == std::nullopt || !(*inst)->Descriptor.SyncCreate()) // don't know the ent, or it's a local only ent (negative number)
				return;

			EntityInstances.Remove(id);
			EntityEvents.Call(EntityEventTypes::EntityRemoved, [&inst](auto func) {func(*inst); });
		}

		void ClientWorld::ProcessAcceptClientAddEntity(MessageBufferReader& reader)
		{
			auto remoteID = reader.ReadID();
			auto localID = reader.ReadID();

			// check to see if it's a local entity that we removed before
			auto itr = std::find(DeadLocalEntities.begin(), DeadLocalEntities.end(), localID);
			if (itr != DeadLocalEntities.end())
			{
				
				DeadLocalEntities.erase(itr);
				if (remoteID >= 0)// the server accepted it so tell them to remove it
				{
					MessageBufferBuilder deleteMesage;
					deleteMesage.Command = MessageCodes::RemoveEntity;
					deleteMesage.AddID(remoteID);
					Send(deleteMesage.Pack());
				}
				return; // we already removed this one
			}

			auto inst = EntityInstances.Find(localID);
			if ( localID > 0|| inst == std::nullopt || (*inst)->OwnerID != Self->GetID() ||!(*inst)->Descriptor.SyncCreate() || !(*inst)->Descriptor.AllowServerCreate())
				return; // not a local ent, unknown ent, or not a client synced ent

			if (remoteID < 0) // server rejected our local ent, so we must destroy it
			{
				EntityInstances.Remove(localID);
				EntityEvents.Call(EntityEventTypes::EntityRemoved, [&inst](auto func) {func(*inst); });
				return;
			}

			// remap the entity to the global remote ID and notify
			EntityInstances.Remove(localID);
			EntityInstances.Insert(remoteID, *inst);
			(*inst)->SetID(remoteID);
			EntityEvents.Call(EntityEventTypes::EntityAccepted, [&inst](auto func) {func(*inst); });
		}

		void ClientWorld::ProcessEntityDataChange(MessageBufferReader& reader)
		{
			auto entityID = reader.ReadID();
			auto inst = EntityInstances.Find(entityID);
			if (entityID < 0 || inst == nullptr || !(*inst)->Descriptor.SyncCreate())
				return;

			while (!reader.Done())
			{
				int prop = reader.ReadInt();
				if (prop < 0 || prop >= (*inst)->Properties.Size())
					continue;;
				(*inst)->Properties[prop]->UnpackValue(reader, (*inst)->Properties[prop]->Descriptor->UpdateFromServer());
			}
			EntityEvents.Call(EntityEventTypes::EntityUpdated, [&inst](auto func) {func(*inst); });
		}

		int64_t ClientWorld::CreateInstance(int entityTypeID)
		{
			int64_t id = GetNewEntityLocalID();

			auto entDef = EntityDefs.Find(entityTypeID);
			if (entDef == std::nullopt || entDef->AllowServerCreate()) // invalid or client only create
				return EntityInstance::InvalidID;

			EntityInstance::Ptr ent = NewEntityInstance(*entDef, id);
			ent->OwnerID = Self->GetID();
			EntityInstances.Insert(ent->ID, ent);

			NewLocalEntities.push_back(ent); // cache so the next update will send the message with any data set

			EntityEvents.Call(EntityEventTypes::EntityAdded, [&ent](auto func) {func(ent); });

			return ent->ID;
		}

		int64_t ClientWorld::CreateInstance(const std::string& entityType)
		{
			auto ent = EntityDefs.FindIF([&entityType](const int& id, EntityDesc& desc) {return desc.Name == entityType; });
			if (ent == std::nullopt)
				return EntityInstance::InvalidID;

			return CreateInstance(ent->ID);
		}

		bool ClientWorld::RemoveInstance(int64_t entityID)
		{
			auto inst = EntityInstances.Find(entityID);
			if (entityID < 0 || inst == nullptr || !(*inst)->Descriptor.AllowClientCreate())
				return false;

			if (entityID < 0) // it's local of some kind, 
			{
				if ((*inst)->Descriptor.SyncCreate()) // we told the server we added this
				{
					auto itr = std::find(NewLocalEntities.begin(), NewLocalEntities.end(), *inst);
					if (itr != NewLocalEntities.end()) // it's so new we haven't sent it yet
						NewLocalEntities.erase(itr);
					else
						DeadLocalEntities.push_back(entityID); // we sent it, so flag it for immediate removal once the server accepts it
				}
			}
			else // tell the server that we removed it and sync all other clients
			{
				MessageBufferBuilder removeMessage;
				removeMessage.Command = MessageCodes::RemoveEntity;
				removeMessage.AddID(entityID);
				Send(removeMessage.Pack());
			}

			EntityInstances.Remove(entityID);
			EntityEvents.Call(EntityEventTypes::EntityRemoved, [&inst](auto func) {func(*inst); });
			return true;
		}

		int64_t ClientWorld::GetNewEntityLocalID()
		{
			LastLocalID--;
			while (EntityInstances.ContainsKey(LastLocalID))
			{
				if (LastLocalID == (EntityInstance::InvalidID + 1))
				{
					if (EntityInstances.Size() >= static_cast<size_t>(abs(EntityInstance::InvalidID)))
					{
						throw("Too many entities to create local ID");
					}
					LastLocalID = -1;
				}
				else
					LastLocalID--;
			}

			return LastLocalID;
		}

		void ClientWorld::ProcessLocalEntities()
		{
			for (auto ptr : NewLocalEntities)	// walk any new entities and send the data for them
			{
				MessageBufferBuilder addMsg;
				addMsg.Command = MessageCodes::AddEntity;
				addMsg.AddID(ptr->Descriptor.ID);
				addMsg.AddID(ptr->ID);
				ptr->Properties.DoForEach([&addMsg](PropertyData::Ptr prop) {prop->PackValue(addMsg); });
				Send(addMsg.Pack());
			}
			NewLocalEntities.clear();
		}
	}
}