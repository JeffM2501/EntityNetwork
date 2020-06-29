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

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>

#include "PropertyDescriptor.h"

namespace EntityNetwork
{
	class EntityDesc
	{
	public:
		int ID = -1;
		std::string Name;

		bool IsAvatar = false;

		enum class CreateScopes
		{
			ClientLocal,
			ClientSync,
			ServerLocal,
			ServerSync,
		};
		CreateScopes CreateScope = CreateScopes::ServerSync;

		std::vector<PropertyDesc> Properties;

		inline bool AllowServerCreate() const
		{
			return CreateScope == CreateScopes::ServerLocal || CreateScope == CreateScopes::ServerSync;
		}

		inline bool AllowClientCreate() const
		{
			return CreateScope == CreateScopes::ClientLocal || CreateScope == CreateScopes::ClientSync;
		}

		inline bool SyncCreate() const
		{
			return CreateScope == CreateScopes::ServerSync || CreateScope == CreateScopes::ClientSync;
		}

		inline int AddPropertyDesc(PropertyDesc& desc)
		{
			desc.ID = static_cast<int>(Properties.size());
			Properties.push_back(desc);
			return desc.ID;
		}

		inline int AddPropertyDesc(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize = 0)
		{
			PropertyDesc desc;
			desc.Name = name;
			desc.DataType = dataType;
			desc.Scope = PropertyDesc::Scopes::BidirectionalSync;
			desc.BufferSize = dataSize;
			return AddPropertyDesc(desc);
		}

	protected:
	};
}