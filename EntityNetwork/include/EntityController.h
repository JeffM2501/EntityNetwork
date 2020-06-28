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
#include <functional>
#include <memory>
#include <mutex>

#include "PropertyDescriptor.h"
#include "PropertyData.h"
#include "MutexedMessageBuffer.h"
#include "MutexedVector.h"
#include "EventList.h"

namespace EntityNetwork
{
	class World;

	class EntityController
	{
	public:
		EntityController(int64_t id):ID(id) {}
		virtual ~EntityController() {}

		inline int64_t GetID() { return ID; }

		typedef std::shared_ptr<EntityController> Ptr;

		enum class EventTypes
		{
			Updated,
			Destroyed,
		};
		EventList<EventTypes, std::function<void(EntityController&)>> Events;

		enum class RemotePropertyEventTypes
		{
			Added,
			Modified,
			Deleted,
		};
		EventList<RemotePropertyEventTypes, std::function<void(EntityController&, PropertyData::Ptr)>> PropertyEvents;


		inline virtual void AddInbound(MessageBuffer::Ptr message) {}
		inline virtual MessageBuffer::Ptr GetOutbound() { return nullptr; }
		inline virtual size_t GetOutboundSize() { return 0; }

		PropertyData::Ptr FindPropertyByID(int id);

		std::vector<PropertyData::Ptr> GetDirtyProperties();

	protected:

		friend class World;

		virtual void SetPropertyInfo(MutexedVector<PropertyDesc>& propertyDecriptors);
		virtual void Update() {}

		int64_t ID = 0;
		MutexedVector<PropertyData::Ptr> Properties;
	};
}