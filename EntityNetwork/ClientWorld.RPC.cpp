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
		void ClientWorld::ExecuteRemoteProcedureFunction(int index, std::vector<PropertyData::Ptr>& arguments)
		{
			auto procDef = GetRPCDef(index);
			if (procDef == nullptr || procDef->RPCFunction == nullptr || procDef->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ClientToServer)
				return;

			procDef->RPCFunction(arguments);
		}

		bool ClientWorld::CallRPC(const std::string& name, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(name);
			if (procPtr == nullptr)
				return false;

			return CallRPC(procPtr->RPCDefintion.ID, args);
		}

		bool ClientWorld::CallRPC(int index, std::vector<PropertyData::Ptr>& args)
		{
			auto procPtr = GetRPCDef(index);

			if (procPtr == nullptr || procPtr->RPCDefintion.Scope != RemoteProcedureDef::Scopes::ClientToServer)
				return false;

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::CallRPC;
			builder.AddInt(index);
			for (PropertyData::Ptr arg : args)
				arg->PackValue(builder);

			Send(builder.Pack());

			return true;
		}

		void ClientWorld::ProcessRPC(MessageBufferReader& reader)
		{
			int id = reader.ReadInt();
			auto rpc = GetRPCDef(id);

			if (rpc == nullptr || rpc->RPCFunction == nullptr || rpc->RPCDefintion.Scope == RemoteProcedureDef::Scopes::ClientToServer)
				return;

			std::vector<PropertyData::Ptr> args = GetRPCArgs(id);
			int argIndex = 0;
			while (!reader.Done())
			{
				if (argIndex > args.size())
					return;

				reader.ReadByte(); // skip the id we know the order from the function definition
				args[argIndex]->UnpackValue(reader, true);

				argIndex++;
			}

			ExecuteRemoteProcedureFunction(id, args);
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
	}
}