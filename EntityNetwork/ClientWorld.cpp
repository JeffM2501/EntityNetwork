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

namespace EntityNetwork
{
	namespace Client
	{
		void ClientWorld::Update()
		{
			if (Self == nullptr || EntityControllerProperties.Size() == 0)
				return;

			ProcessLocalEntities();

			std::vector<MessageBuffer::Ptr> pendingMods;
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::SetControllerPropertyDataValues;
			for (auto prop : Self->GetDirtyProperties())
			{
				if (prop->Descriptor.UpdateFromClient())
				{
					builder.AddInt(prop->Descriptor.ID);
					prop->PackValue(builder);
				}
				pendingMods.push_back(builder.Pack());
			}

			for (auto msg : pendingMods)
				Send(msg);
		}

		void ClientWorld::AddInboundData(MessageBuffer::Ptr message)
		{
			MessageBufferReader reader(message);

			switch (reader.Command)
			{
			case MessageCodes::AddWordDataDef:
			case MessageCodes::AddControllerPropertyDef:
			case MessageCodes::AddRPCDef:
			case MessageCodes::AddEntityDef:
				HandlePropteryDescriptorMessage(reader);
				break;

			case MessageCodes::AcceptController:
			{
				auto id = reader.ReadID();
				Self = CreateController(id, true);
				Self->IsSelf = true;
			}
			break;

			case MessageCodes::AddController:
				ProcessAddController(reader);
				break;

			case MessageCodes::RemoveController:
				break;

			case MessageCodes::SetControllerPropertyDataValues:
				ProcessSetControllerPropertyData(reader);
			break;

			case MessageCodes::SetWorldDataValues:
				while (!reader.Done())
				{
					auto prop = WorldProperties.TryGet(reader.ReadByte());
					if (prop == nullptr)
						break;
					(*prop)->UnpackValue(reader, (*prop)->Descriptor.UpdateFromServer());
					PropertyEvents.Call(PropertyEventTypes::WorldPropertyDataChanged, [&prop](auto func) {func(nullptr, (*prop)->Descriptor.ID); });

					(*prop)->SetClean(); // remote properties are never dirty, only locally set ones
				}
				break;

			case MessageCodes::CallRPC:
				ProcessRPC(reader);
				break;

			case MessageCodes::AddEntity:
				ProcessAddEntity(reader);
				break;

			case MessageCodes::RemoveEntity:
				ProcessRemoveEntity(reader);
				break;

			case MessageCodes::AcceptClientEntity:
				ProcessAcceptClientAddEntity(reader);
				break;

			case MessageCodes::SetEntityDataValues:
				ProcessEntityDataChange(reader);
				break;

			case MessageCodes::NoOp:
			default:
				break;
			}
		}

		MessageBuffer::Ptr ClientWorld::PopOutboundData()
		{
			if (Self == nullptr)
				return nullptr;

			return Self->OutboundMessages.Pop();
		}

		void ClientWorld::Send(MessageBuffer::Ptr message)
		{
			if (Self == nullptr)
				return;

			Self->OutboundMessages.Push(message);
		}

		ClientEntityController::Ptr ClientWorld::PeerFromID(int64_t id)
		{
			if (Self != nullptr && Self->GetID() == id)
				return Self;
			
			auto peer = Peers.Find(id);
			if (peer == std::nullopt)
				return nullptr;
				return nullptr;

			return *peer;	
		}

		void ClientWorld::HandlePropteryDescriptorMessage(MessageBufferReader& reader)
		{
			if (EntityControllerProperties.Size() == 0 && WorldProperties.Size() == 0 && RemoteProcedures.Size() == 0) // first data of anything we get is property descriptors
				StateEvents.Call(StateEventTypes::Negotiating, [](auto func) {func(StateEventTypes::Negotiating); });

			if (reader.Command == MessageCodes::AddControllerPropertyDef)
			{
				PropertyDesc desc;
				desc.ID = reader.ReadInt();
				desc.Name = reader.ReadString();
				desc.DataType = static_cast<PropertyDesc::DataTypes>(reader.ReadByte());
				desc.Scope = static_cast<PropertyDesc::Scopes>(reader.ReadByte());
				desc.Private = reader.ReadBool();

				RegisterControllerPropertyDesc(desc);
				if (Self != nullptr)	// if we are connected, fix all the other peers
				{
					SetupEntityController(static_cast<EntityController&>(*Self));
					Peers.DoForEach([this](auto id, auto peer) {SetupEntityController(*peer); });
				}
				PropertyEvents.Call(PropertyEventTypes::ControllerPropertyDefAdded, [&desc](auto func) {func(nullptr, desc.ID); });
			}
			else if (reader.Command == MessageCodes::AddWordDataDef)
			{
				PropertyDesc desc;
				desc.ID = reader.ReadInt();
				desc.Name = reader.ReadString();
				desc.DataType = static_cast<PropertyDesc::DataTypes>(reader.ReadByte());

				RegisterWorldPropertyDesc(desc);
				
				PropertyEvents.Call(PropertyEventTypes::WorldPropertyDefAdded, [&desc](auto func) {func(nullptr, desc.ID); });
			}
			else if (reader.Command == MessageCodes::AddRPCDef)
			{
				ClientRPCDef::Ptr desc = std::make_shared<ClientRPCDef>();
				desc->RPCDefintion.ID = reader.ReadInt();
				desc->RPCDefintion.Name = reader.ReadString();
				desc->RPCDefintion.Scope = static_cast<RemoteProcedureDef::Scopes>(reader.ReadByte());

				while (!reader.Done())
					desc->RPCDefintion.DefineArgument(static_cast<PropertyDesc::DataTypes>(reader.ReadByte()));

				RemoteProcedures.PushBack(desc);

				// check to see if there is a cached function that was mapped to the name before we connected and got the definition.
				std::map<std::string, ClientRPCFunction>::iterator itr = CacheedRPCFunctions.find(desc->RPCDefintion.Name);
				if (itr != CacheedRPCFunctions.end())
				{
					desc->RPCFunction = itr->second;
					CacheedRPCFunctions.erase(itr);
				}

				PropertyEvents.Call(PropertyEventTypes::RPCRegistered, [&desc](auto func) {func(nullptr, desc->RPCDefintion.ID); });
			}
			else if (reader.Command == MessageCodes::AddEntityDef)
			{
				EntityDesc def;
				def.ID = reader.ReadInt();
				def.Name = reader.ReadString();
				def.IsAvatar = reader.ReadBool();
				def.CreateScope = static_cast<EntityDesc::CreateScopes>(reader.ReadByte());
				while (reader.Done())
				{
					PropertyDesc prop;
					prop.ID = reader.ReadInt();
					prop.Scope = static_cast<PropertyDesc::Scopes>(reader.ReadByte());
					prop.Name = reader.ReadString();
					prop.DataType = static_cast<PropertyDesc::DataTypes>(reader.ReadByte());
				}

				EntityDefs.Insert(def.ID, def);
				PropertyEvents.Call(PropertyEventTypes::EntityDefAdded, [&def](auto func) {func(nullptr, def.ID); });
			}
		}
	}
}