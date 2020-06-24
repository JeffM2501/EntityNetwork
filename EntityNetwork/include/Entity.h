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

#include <vector>

#include "MutexedVector.h"
#include "PropertyData.h"
#include "EntityDescriptor.h"

namespace EntityNetwork
{
	class KnownEnityDataset
	{
	public:
		std::vector<revision_t> DataRevisions;
	};


	class EntityInstance
	{
	public:
		static const int64_t InvalidID = -1;

		int64_t ID = InvalidID;
		int64_t OwnerID = InvalidID;
		const EntityDesc& Descriptor;

		MutexedVector<PropertyData::Ptr> Properties;

		EntityInstance(const EntityDesc& desc) : Descriptor(desc) {}

		bool Dirty();

		std::vector<PropertyData::Ptr> GetDirtyProperties(KnownEnityDataset& knownSet);

		typedef std::shared_ptr<EntityInstance> Ptr;

		static Ptr MakeShared(const EntityDesc& desc)
		{
			return std::make_shared<EntityInstance>(desc);
		}

	protected:
	};
}