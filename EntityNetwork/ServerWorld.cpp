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

			MessageBufferBuilder hail;
			hail.Command = MessageCodes::HailCheck;
			hail.AddString(PROTOCOL_HEADER);
			Send(ctl, hail);

			// RPC defs
			Send(ctl,RPCDefCache);

			// send world data
			Send(ctl,WorldPropertyDefCache);
			MessageBufferBuilder worldDataUpdates;
			worldDataUpdates.Command = MessageCodes::SetWorldDataValues;
			WorldProperties.DoForEach([&worldDataUpdates](PropertyData::Ptr prop){prop->PackValue(worldDataUpdates);});
			if (!worldDataUpdates.Empty())
				Send(ctl, worldDataUpdates);

			// send controller properties
			ctl->OutboundMessages.AppendRange(ControllerPropertyCache);
			SetupEntityController(static_cast<EntityController&>(*ctl));

			// tell the owner they were accepted and what there ID is.
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AcceptController;
			builder.AddID(id);
			Send(ctl, builder);

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

			RemoteEnitityControllers.DoForEach([this,&builder,&ctl,&createMessage](auto& key, ServerEntityController::Ptr& peer)
				{
					Send(peer, createMessage);

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

				case MessageCodes::CallRPC:
				{
					int id = reader.ReadInt();
					auto rpc = GetRPCDef(id);

					if (rpc == nullptr || rpc->RPCFunction == nullptr || rpc->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
						break;

					std::vector<PropertyData::Ptr> args = GetRPCArgs(id);
					int argIndex = 0;
					while (!reader.Done())
					{
						if (argIndex > args.size())
							break;

						reader.ReadByte();// just skip the ID, we know it's always in order
						args[argIndex]->UnpackValue(reader, true);

						argIndex++;
					}

					ExecuteRemoteProcedureFunction(id, peer, args);
				}
					break;

				// server can't get these, it only sends them
				case MessageCodes::AddControllerProperty:
				case MessageCodes::RemoveControllerProperty:
				case MessageCodes::RemoveController:
				case MessageCodes::AcceptController:
				case MessageCodes::AddController:
				case MessageCodes::AddRPCDef:
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

		int ServerWorld::RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize)
		{
			int index = World::RegisterWorldPropertyData(name,dataType,dataSize);

			SendToAll(BuildWorldPropertySetupMessage(index));
			SendToAll(BuildWorldPropertyDataMessage(index));

			return index;
		}

		int ServerWorld::RegisterRemoteProcedure(RemoteProcedureDef& desc)
		{
			desc.ID = static_cast<int>(RemoteProcedures.Size());
			auto ptr = std::make_shared<ServerRPCDef>();
			ptr->RPCDefintion = desc;
			RemoteProcedures.PushBack(ptr);
			SendToAll(BuildRPCSetupMessage(desc.ID));
			return desc.ID;
		}

		void ServerWorld::AssignRemoteProcedureFunction(const std::string& name, ServerRPCFunction function)
		{
			auto procDef = GetRPCDef(name);
			if (procDef == nullptr)
				return;

			procDef->RPCFunction = function;
		}

		void ServerWorld::AssignRemoteProcedureFunction(int id, ServerRPCFunction function)
		{
			auto procDef = GetRPCDef(id);
			if (procDef == nullptr)
				return;

			procDef->RPCFunction = function;
		}

		ServerWorld::ServerRPCDef::Ptr ServerWorld::GetRPCDef(int index)
		{
			auto procDef = RemoteProcedures.TryGet(index);
			if (procDef == nullptr)
				return nullptr;

			return *procDef;
		}

		ServerWorld::ServerRPCDef::Ptr ServerWorld::GetRPCDef(const std::string& name)
		{
			auto procDef = RemoteProcedures.FindFirstMatch([name](ServerRPCDef::Ptr p) {return p->RPCDefintion.Name == name; });
			if (procDef == std::nullopt)
				return nullptr;

			return *procDef;
		}

		std::vector<PropertyData::Ptr> ServerWorld::GetRPCArgs(int index)
		{
			std::vector<PropertyData::Ptr> args;

			auto procDef = GetRPCDef(index);
			if (procDef == nullptr)
				return args;

			for (auto& argDef : procDef->RPCDefintion.ArgumentDefs)
				args.push_back(PropertyData::MakeShared(argDef));

			return args;
		}

		std::vector<PropertyData::Ptr> ServerWorld::GetRPCArgs(const std::string& name)
		{
			auto procDef = GetRPCDef(name);
			if (procDef == nullptr)
				return std::vector<PropertyData::Ptr>();

			return GetRPCArgs(procDef->RPCDefintion.ID);
		}

		void ServerWorld::ExecuteRemoteProcedureFunction(int index, ServerEntityController::Ptr sender, std::vector<PropertyData::Ptr>& arguments)
		{
			auto procDef = GetRPCDef(index);
			if (procDef == nullptr || procDef->RPCFunction == nullptr || procDef->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return;

			procDef->RPCFunction(sender, arguments);
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

		MessageBuffer::Ptr ServerWorld::BuildRPCSetupMessage(int index)
		{
			auto data = GetRPCDef(index);
			if (data == nullptr)
				return nullptr;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::AddRPCDef;
			builder.AddInt(index);
			builder.AddString(data->RPCDefintion.Name);
			builder.AddByte(static_cast<int>(data->RPCDefintion.Scope));
			for (auto arg : data->RPCDefintion.ArgumentDefs)
				builder.AddByte(static_cast<int>(arg.DataType));
			
			auto msg = builder.Pack();
			RPCDefCache.PushBack(msg);

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

		bool ServerWorld::CallRPC(int index, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(index);

			if (procPtr == nullptr || procPtr->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ClientToServer)
				return false;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::CallRPC;
			builder.AddInt(index);
			for(PropertyData::Ptr arg : args)
				arg->PackValue(builder);

			if (procPtr->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ServerToSingleClient)
				Send(target, builder.Pack());
			else
				SendToAll(builder.Pack());

			return true;
		}

		bool ServerWorld::CallRPC(const std::string& name, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(name);
			if (procPtr == nullptr)
				return false;

			return CallRPC(procPtr->RPCDefintion.ID, target, args);
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