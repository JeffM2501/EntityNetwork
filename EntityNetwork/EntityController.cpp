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
#include "EntityController.h"
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>

#include "ThreadTools.h"

namespace EntityNetwork
{
	PropertyData::Ptr EntityController::FindPropertyByID(int id)
	{
		auto prop = Properties.FindFirstMatch([id](PropertyData::Ptr p) {return p->Descriptor->ID == id; });
		if (prop != std::nullopt)
			return *prop;
		
		return nullptr;
	}

	PropertyData::Ptr EntityController::FindPropertyByName(const std::string& name)
	{
		auto prop = Properties.FindFirstMatch([&name](PropertyData::Ptr p) {return p->Descriptor->Name == name; });
		if (prop != std::nullopt)
			return *prop;

		return nullptr;
	}

	void EntityController::SetPropertyInfo(MutexedVector<PropertyDesc::Ptr>& propertyDecriptors)
	{
		std::vector<PropertyData::Ptr> newProps;

		propertyDecriptors.DoForEach([this, &newProps](PropertyDesc::Ptr& desc)
			{
				auto existing = Properties.FindFirstMatch([desc](PropertyData::Ptr otherPtr) {return desc == otherPtr->Descriptor; });

				PropertyData::Ptr newProp;
				if (existing != std::nullopt)
					newProp = *existing;
				else
					newProp = PropertyData::MakeShared(desc);

				newProps.push_back(newProp);

				PropertyEvents.Call(RemotePropertyEventTypes::Added, [this, newProp](auto func) {func(*this, newProp); });
			});

		Properties.Replace(newProps);
	}

	std::vector<PropertyData::Ptr> EntityController::GetDirtyProperties()
	{
		std::vector<PropertyData::Ptr> dirtyProps;

		Properties.DoForEach([&dirtyProps](PropertyData::Ptr prop) 
			{
				if (prop->IsDirty())
					dirtyProps.push_back(prop);

				prop->SetClean();
			});
		return dirtyProps;
	}
}