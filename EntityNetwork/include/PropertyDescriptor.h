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
#include <map>

namespace EntityNetwork
{
	class PropertyDesc
	{
	public:
		std::string Name;
		int ID = 0;

		enum class Scopes
		{
			ClientNoSync = 1,
			ClientPushSync = 2,
			ServerNoSync = 128,
			ServerPushSync = 129,
			BidirectionalSync = 200,
		};
		Scopes Scope = Scopes::BidirectionalSync;

		bool Private = false;

		inline bool TransmitDef() const
		{
			return Scope != Scopes::ServerNoSync && Scope != Scopes::ClientNoSync;
		}

		inline bool UpdateFromClient() const
		{
			return TransmitDef() && (Scope == Scopes::BidirectionalSync || Scope != Scopes::ClientPushSync);
		}

		inline bool UpdateFromServer() const
		{
			return TransmitDef() && (Scope == Scopes::BidirectionalSync || Scope != Scopes::ServerPushSync);
		}

		enum class DataTypes
		{
			Integer = 0,
			Vector3I = 1,
			Vector4I = 2,
			Float = 3,
			Vector3F = 4,
			Vector4F = 5,
			Double = 6,
			Vector3D = 7,
			Vector4D = 8,
			String = 9,
			Buffer = 10,
		};
		DataTypes DataType = DataTypes::Integer;

		size_t BufferSize = 0;

		typedef std::vector<PropertyDesc> Vec;
		typedef std::map<std::string, PropertyDesc> Map;

		inline bool operator == (const PropertyDesc& rhs) const
		{
			return Name == rhs.Name;
		}

		inline bool operator != (const PropertyDesc& rhs) const
		{
			return !(*this == rhs);
		}
	};
}