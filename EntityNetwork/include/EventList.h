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
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace EntityNetwork
{

	template <class K, class V>
	class EventList
	{
	public:
		std::map<K, std::vector<std::shared_ptr<V>>> EventMap;
		std::mutex DataMutex;
		inline void Subscribe(K evt, V callback)
		{
			MutexGuardian guard(DataMutex);

			auto itr = EventMap.find(evt);
			if (itr == EventMap.end())
				EventMap[evt] = std::vector<std::shared_ptr<V>>();
			itr = EventMap.find(evt);

			auto& funcList = itr->second;

			std::shared_ptr<V> cbPtr = std::make_shared();
			*cbPtr = callback;

			if (std::find(funcList.begin(), funcList.end(), callback) == funcList.end())
				itr->second.push_back(cbPtr);
		}

		inline void UnsubscribeFromEvent(K evt, V callback)
		{
			MutexGuardian guard(DataMutex);

			auto itr = EventMap.find(evt);
			if (itr == EventMap.end())
				return;
			auto& funcList = itr->second;

			auto cbItr = std::find_if(funcList.begin(), funcList.end(), [callback](auto c) { return *c == callback; });
			if (cbItr != funcList.end())
				funcList.erase(cbItr);
		}

		inline void Call(K evt, std::function<void(V&)> func)
		{
			MutexGuardian guard(DataMutex);

			auto itr = EventMap.find(evt);
			if (itr == EventMap.end())
				return;

			for (auto& val : itr->second)
			{
				func(*val);
			}
		}
	};
}
