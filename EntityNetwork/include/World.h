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

#include "EntityController.h"
#include "PropertyDescriptor.h"
#include "MutexedVector.h"
#include <MutexedMap.h>
#include "RemoteProcedureDescriptor.h"
#include "EntityDescriptor.h"

#include <vector>

namespace EntityNetwork
{
	// common fucntions for both client and server worlds
	class World
	{
	public:

		// register a controller property descriptor (may be overridden)
		virtual int RegisterControllerPropertyDesc(PropertyDesc& desc);
		virtual int RegisterControllerProperty(const std::string& name, PropertyDesc::DataTypes dataType, size_t bufferSize = 0, PropertyDesc::Scopes scope = PropertyDesc::Scopes::BidirectionalSync, bool isPrivate = false);

		// register a world property descriptor (may be overridden)
		virtual int RegisterWorldPropertyDesc(PropertyDesc& desc);
		virtual int RegisterWorldPropertyData(const std::string& name, PropertyDesc::DataTypes dataType, size_t dataSize = 0);


		// get the data for a world property, data set will be synced to all clients
		PropertyData::Ptr GetWorldPropertyData(int id);
		PropertyData::Ptr GetWorldPropertyData(const std::string& name);

	protected:
		// entity controllers
		MutexedVector<PropertyDesc> EntityControllerProperties;
		MutexedVector<PropertyDesc> WorldPropertyDefs;
		MutexedMap<int, EntityDesc>	EntityDefs;

		EntityDesc* GetEntityDef(int index);

		virtual void SetupControllerProperty(int index);
		virtual void SetupEntityController(EntityController& controller);

		// world properties
		MutexedVector<PropertyData::Ptr> WorldProperties;

	};
}