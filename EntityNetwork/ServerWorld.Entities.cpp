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
			builder.AddBool(def->Child);
			builder.AddBool(def->AllowClientCreate);
			builder.AddInt(def->ParrentTypeID);
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
			if (entDef == std::nullopt)
				return EntityInstance::InvalidID;

			EntityInstance::Ptr ent = EntityInstance::MakeShared(*entDef);
			ent->ID = EntityInstances.Size();
			ent->OwnerID = ownerID;
			EntityInstances.Insert(ent->ID, ent);
			return ent->ID;
		}

		int64_t ServerWorld::CreateInstance(const std::string& entityType, int64_t ownerID)
		{
			auto ent = EntityDefs.FindIF([&entityType](const int& id, EntityDesc& desc) {return desc.Name == entityType; });
			if (ent == std::nullopt)
				return EntityInstance::InvalidID;

			return CreateInstance(ent->ID, ownerID);
		}
	}
}
