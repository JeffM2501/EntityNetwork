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
#include "Entity.h"

namespace EntityNetwork
{
	EntityInstance::EntityInstance(EntityDesc::Ptr desc) : Descriptor(desc)
	{
		for (auto& prop : Descriptor->Properties)
		{
			Properties.PushBack(PropertyData::MakeShared(prop));
		}
	}

	bool EntityInstance::Dirty()
	{
		bool dirty = false;
		Properties.DoForEach([&dirty](PropertyData::Ptr ptr) {if (ptr->IsDirty()) dirty = false; });
		return dirty;
	}

	void EntityInstance::CleanAll()
	{
		Properties.DoForEach([](PropertyData::Ptr ptr) {ptr->SetClean(); });
	}

	std::vector<PropertyData::Ptr> EntityInstance::GetDirtyProperties(KnownEnityDataset& knownSet)
	{
		int index = 0;
		std::vector<PropertyData::Ptr> dirtyList;

		Properties.DoForEach([&index, &dirtyList, &knownSet](PropertyData::Ptr ptr) 
			{
				if (index >= knownSet.DataRevisions.size() || knownSet.DataRevisions[index] != ptr->GetRevision())
					dirtyList.push_back(ptr);
				index++;
			});

		return dirtyList;
	}
}
