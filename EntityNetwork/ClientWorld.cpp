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

namespace EntityNetwork
{
	namespace Client
	{
		void ClientWorld::Update()
		{
			if (Self == nullptr || EntityControllerProperties.Size() == 0)
				return;

			std::vector<MessageBuffer::Ptr> pendingMods;
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::SetControllerPropertyValues;
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
			case MessageCodes::AddControllerProperty:
			case MessageCodes::RemoveControllerProperty:
			case MessageCodes::AddRPCDef:
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
			{
				auto id = reader.ReadID();
				ClientEntityController::Ptr subject = nullptr;

				if (Self != nullptr && id == Self->GetID())
					subject = Self;
				else
				{
					subject = CreateController(id, false);
					Peers.Insert(id, subject);
				}
				SetupEntityController(static_cast<EntityController&>(*subject));

				while (!reader.Done())
				{
					auto prop = subject->FindPropertyByID(reader.ReadByte());
					prop->UnpackValue(reader, true); // always save the initial data 
				}
				
				ControllerEvents.Call(subject->IsSelf ? ControllerEventTypes::SelfCreated : ControllerEventTypes::RemoteCreated, [&subject](auto func) {func(subject); });
				subject->GetDirtyProperties(); // just clear out anything that god dirty due to an unpack, we always start clean

				if (subject->IsSelf)
					StateEvents.Call(StateEventTypes::ActiveSyncing, [](auto func) {func(StateEventTypes::ActiveSyncing); });
			}
			break;

			case MessageCodes::RemoveController:
				break;

			case MessageCodes::SetControllerPropertyValues:
			{
				auto subject = PeerFromID(reader.ReadID());
				if (subject != nullptr)
				{
					while (!reader.Done())
					{
						auto prop = subject->FindPropertyByID(reader.ReadByte());
						prop->UnpackValue(reader, prop->Descriptor.UpdateFromServer());
						
						if (prop->Descriptor.UpdateFromServer())
							PropertyEvents.Call(subject->IsSelf ? PropertyEventTypes::SelfPropteryChanged : PropertyEventTypes::RemoteControllerPropertyChanged, [&subject,&prop](auto func) {func(subject, prop->Descriptor.ID); });

						prop->SetClean(); // remote properties are never dirty, only locally set ones
					}
				}
			}
			break;

			case MessageCodes::SetWorldDataValues:
				while (!reader.Done())
				{
					auto prop = WorldProperties.Get(reader.ReadByte());
					prop->UnpackValue(reader, prop->Descriptor.UpdateFromServer());

					if (prop->Descriptor.UpdateFromServer())
						PropertyEvents.Call(PropertyEventTypes::WorldPropertyDataChanged, [&prop](auto func) {func(nullptr, prop->Descriptor.ID); });

					prop->SetClean(); // remote properties are never dirty, only locally set ones
				}
				break;

			case MessageCodes::CallRPC:
			{
				int id = reader.ReadInt();
				auto rpc = GetRPCDef(id);

				if (rpc == nullptr || rpc->RPCFunction == nullptr || rpc->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ClientToServer)
					break;

				std::vector<PropertyData::Ptr> args = GetRPCArgs(id);
				int argIndex = 0;
				while (!reader.Done())
				{
					if (argIndex > args.size())
						break;

					args[argIndex]->UnpackValue(reader,true);

					argIndex++;
				}

				ExecuteRemoteProcedureFunction(id, args);
			}
			break;

			case MessageCodes::NoOp:
			default:
				break;
			}
		}

		void ClientWorld::AssignRemoteProcedureFunction(const std::string& name, ClientRPCFunction function)
		{
			auto rpc = GetRPCDef(name);
			if (rpc == nullptr) // keep the function pointer around in case we get the function definition later
				CacheedRPCFunctions[name] = function;
			else
				rpc->RPCFunction = function;
		}

		void  ClientWorld::AssignRemoteProcedureFunction(int id, ClientRPCFunction function)
		{
			auto rpc = GetRPCDef(id);
			if (rpc != nullptr)
				rpc->RPCFunction = function;
		}


		ClientWorld::ClientRPCDef::Ptr ClientWorld::GetRPCDef(int index)
		{
			auto procDef = RemoteProcedures.TryGet(index);
			if (procDef == nullptr)
				return nullptr;

			return *procDef;
		}

		ClientWorld::ClientRPCDef::Ptr ClientWorld::GetRPCDef(const std::string& name)
		{
			auto procDef = RemoteProcedures.FindFirstMatch([name](ClientRPCDef::Ptr p) {return p->RPCDefintion.Name == name; });
			if (procDef == std::nullopt)
				return nullptr;

			return *procDef;
		}

		std::vector<PropertyData::Ptr> ClientWorld::GetRPCArgs(int index)
		{
			std::vector<PropertyData::Ptr> args;

			auto procDef = GetRPCDef(index);
			if (procDef == nullptr)
				return args;

			for (auto& argDef : procDef->RPCDefintion.ArgumentDefs)
				args.push_back(PropertyData::MakeShared(argDef));

			return args;
		}

		std::vector<PropertyData::Ptr> ClientWorld::GetRPCArgs(const std::string& name)
		{
			auto procDef = GetRPCDef(name);
			if (procDef == nullptr)
				return std::vector<PropertyData::Ptr>();

			return GetRPCArgs(procDef->RPCDefintion.ID);
		}

		bool ClientWorld::CallRPC(int index, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(index);

			if (procPtr == nullptr || procPtr->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return false;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::CallRPC;
			for (PropertyData::Ptr arg : args)
				arg->PackValue(builder);
			
			Send(builder.Pack());

			return true;
		}

		bool ClientWorld::CallRPC(const std::string& name, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(name);
			if (procPtr == nullptr)
				return false;

			return CallRPC(procPtr->RPCDefintion.ID, args);
		}

		void ClientWorld::ExecuteRemoteProcedureFunction(int index, std::vector<PropertyData::Ptr>& arguments)
		{
			auto procDef = GetRPCDef(index);
			if (procDef == nullptr || procDef->RPCFunction == nullptr || procDef->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return;

			procDef->RPCFunction(arguments);
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

			if (reader.Command == MessageCodes::AddControllerProperty)
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
			else if (reader.Command == MessageCodes::RemoveControllerProperty)
			{
				auto id = reader.ReadInt();

				PropertyEvents.Call(PropertyEventTypes::ControllerPropertyDefRemoved, [id](auto func) {func(nullptr,id); });
				EntityControllerProperties.EraseIf([id](PropertyDesc& desc) {return desc.ID == id; });

				if (Self != nullptr)	// if we are connected, fix all the other peers
				{
					SetupEntityController(static_cast<EntityController&>(*Self));
					Peers.DoForEach([this](auto id, auto peer) {SetupEntityController(*peer); });
				}
			}
		}
	}
}