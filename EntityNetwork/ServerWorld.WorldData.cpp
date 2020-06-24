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
		int ServerWorld::RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize)
		{
			int index = World::RegisterWorldPropertyData(name, dataType, dataSize);

			SendToAll(BuildWorldPropertySetupMessage(index));
			SendToAll(BuildWorldPropertyDataMessage(index));

			return index;
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

		MessageBuffer::Ptr ServerWorld::BuildWorldPropertyDataMessage(int index)
		{
			auto data = WorldProperties[index];

			MessageBufferBuilder builder;
			builder.Command = MessageCodes::SetWorldDataValues;
			data->PackValue(builder);
			auto msg = builder.Pack();
			return msg;
		}

	}
}
