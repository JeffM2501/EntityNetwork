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

		void ServerWorld::ProcessRPCall(ServerEntityController::Ptr peer, MessageBufferReader& reader)
		{
			int id = reader.ReadInt();
			auto rpc = GetRPCDef(id);

			if (rpc == nullptr || rpc->RPCFunction == nullptr || rpc->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return;

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

		void ServerWorld::ExecuteRemoteProcedureFunction(int index, ServerEntityController::Ptr sender, std::vector<PropertyData::Ptr>& arguments)
		{
			auto procDef = GetRPCDef(index);
			if (procDef == nullptr || procDef->RPCFunction == nullptr || procDef->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return;

			procDef->RPCFunction(sender, arguments);
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

		bool ServerWorld::CallRPC(int index, ServerEntityController::Ptr target, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(index);

			if (procPtr == nullptr || procPtr->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ClientToServer)
				return false;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::CallRPC;
			builder.AddInt(index);
			for (PropertyData::Ptr arg : args)
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
	}
}