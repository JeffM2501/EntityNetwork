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
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "PropertyDescriptor.h"
#include "PropertyData.h"

namespace EntityNetwork
{
	class RemoteProcedureDef
	{
	public:
		std::string Name;
		int ID = -1;

		enum class Scopes
		{
			ServerToAllClients,
			ServerToSingleClient,
			ClientToServer,
		};
		Scopes Scope = Scopes::ServerToAllClients;

		RemoteProcedureDef() {}
		RemoteProcedureDef(const std::string& name) :Name(name) {}
		RemoteProcedureDef(const std::string& name, Scopes scope) :Name(name), Scope(scope){}

		typedef std::shared_ptr<RemoteProcedureDef> Ptr;

		static inline Ptr Make()
		{
			return std::make_shared<RemoteProcedureDef>();
		}

		static inline Ptr Make(const std::string& name)
		{
			return std::make_shared<RemoteProcedureDef>(name);
		}

		static inline Ptr Make(const std::string& name, Scopes scope)
		{
			return std::make_shared<RemoteProcedureDef>(name, scope);
		}

		static RemoteProcedureDef::Ptr CreateClientSideRPC(const std::string& name, bool allClients)
		{
			return RemoteProcedureDef::Make(name, allClients ? Scopes::ServerToAllClients : Scopes::ServerToSingleClient);
		}

		static RemoteProcedureDef::Ptr CreateServerSideRPC(const std::string& name)
		{
			return RemoteProcedureDef::Make(name, Scopes::ClientToServer);
		}
		
		inline bool SendFromClient() const
		{
			return (Scope == Scopes::ClientToServer);
		}

		inline bool SentFromServer() const
		{
			return !SendFromClient();
		}

		inline RemoteProcedureDef& DefineArgument(PropertyDesc::DataTypes dataType, size_t bufferSize = 0)
		{
			PropertyDesc::Ptr desc = PropertyDesc::Make();
			desc->ID = static_cast<int>(ArgumentDefs.size());
			desc->Name = std::to_string(desc->ID);
			desc->DataType = dataType;
			desc->BufferSize = bufferSize;

			ArgumentDefs.push_back(desc);
			return *this;
		}

		std::vector<PropertyDesc::Ptr> ArgumentDefs;

		
	};
}