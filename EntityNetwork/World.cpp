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
#include "World.h"
#include "PropertyDescriptor.h"

namespace EntityNetwork
{
	int World::RegisterControllerPropertyDesc(PropertyDesc& desc)
	{
		desc.ID = (int)EntityControllerProperties.size();
		EntityControllerProperties.PushBack(desc);
		SetupControllerProperty(desc.ID);

		return desc.ID;
	}

	int World::RegisterControllerProperty(const std::string& name, PropertyDesc::DataTypes dataType, size_t bufferSize, PropertyDesc::Scopes scope)
	{
		PropertyDesc desc;
		desc.Name = name;
		desc.DataType = dataType;
		desc.BufferSize = bufferSize;
		desc.Scope = scope;
		return RegisterControllerPropertyDesc(desc);
	}

	void World::SetupControllerProperty(int index)
	{
		// cache stuff here
	}

	void World::SetupEntityController(EntityController& controller)
	{
		controller.SetPropertyInfo(EntityControllerProperties);
	}
}