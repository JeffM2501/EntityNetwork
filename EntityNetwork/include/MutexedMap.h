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
#include <mutex>
#include <map>
#include <functional>
#include <optional>

#include "ThreadTools.h"

namespace EntityNetwork
{
	template <class K, class V>
	class MutexedMap
	{
	protected:
		std::map<K, V> Data;
		std::mutex DataMutex;

	public:
		inline size_t Size() { MutexGuardian guardian(DataMutex); return Data.size(); }

		inline bool Empty() { return Size() == 0; }
		
		inline V& Insert(K key, V val)
		{
			MutexGuardian guardian(DataMutex);
			Data[key] = val;
			return Data[key];
		}

		inline V& Get (K key)
		{
			MutexGuardian guardian(DataMutex);
			return Data[key];
		}

		inline V& operator [] (K key)
		{
			MutexGuardian guardian(DataMutex);
			return Data[key];
		}
		
		inline void Remove(K key)
		{
			MutexGuardian guardian(DataMutex);
			auto itr = Data.find(key);
			if (itr != Data.end())
				Data.erase(itr);
		}

		inline std::optional<V> Find(K key)
		{
			MutexGuardian guardian(DataMutex);
			auto itr = Data.find(key);
			if (itr != Data.end())
				return itr->second;

			return std::nullopt;
		}

		typedef std::function<void(K&, V&)> KeyValueFunction;
		inline void DoForEach(KeyValueFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end(); itr++)
			{
				K k = itr->first;
				V v = itr->second;
				function(k, v);
			}
		}

		typedef std::function<bool(K&, V&)> KeyValueBoolFunction;

		inline void DoForEachUntil(KeyValueBoolFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end(); itr++)
			{
				if (function(itr->first, itr->second))
					break;
			}
		}

		inline void EraseIf(KeyValueBoolFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end();)
			{
				if (function(itr->first, itr->second))
					itr = Data.erase();
				else
					itr++;
			}	
		}
	};
}