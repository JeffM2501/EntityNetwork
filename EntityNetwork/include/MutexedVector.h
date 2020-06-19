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
#include <vector>
#include <functional>
#include <optional>
#include "ThreadTools.h"

namespace EntityNetwork
{

	template <class V>
	class MutexedVector
	{
	protected:
		std::vector<V> Data;
		std::mutex DataMutex;

	public:
		inline size_t size() { MutexGuardian guardian(DataMutex); return Data.size(); }

		inline void PushBack( const V& val)
		{
			MutexGuardian guardian(DataMutex);
			Data.push_back(val);
		}

		inline void PushFront(const V& val)
		{
			MutexGuardian guardian(DataMutex);
			Data.push_back(val);
		}

		inline V PopBack()
		{
			MutexGuardian guardian(DataMutex);
			return Data.pop_back();
		}

		inline V PopFront()
		{
			MutexGuardian guardian(DataMutex);
			return Data.pop_front();
		}

		inline V& Get(int index)
		{
			MutexGuardian guardian(DataMutex);
			return Data[index];
		}

		inline V& operator [](int index)
		{
			MutexGuardian guardian(DataMutex);
			return Data[index];
		}

		inline void Remove(int index)
		{
			MutexGuardian guardian(DataMutex);
			auto itr = Data.begin() + index;
			if (itr != Data.end())
				Data.erase(itr);
		}

		inline std::vector<V>& GetExclusiveAccess()
		{
			DataMutex.lock();
			return Data;
		}

		inline void ReleaseExclusiveAccess()
		{
			DataMutex.unlock();
		}

		typedef std::function<void(V&)> ValueFunction;
		inline void DoForEach(ValueFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end(); itr++)
				function(*itr);
		}

		typedef std::function<bool(V&)> ValueBoolFunction;
		inline void EraseIf(ValueBoolFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end();)
			{
				if (function(*itr))
					itr = Data.erase();
				else
					itr++;
			}
		}

		inline std::vector<V> FindIf(ValueBoolFunction function)
		{
			std::vector<V> found;
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end();)
			{
				if (function(*itr))
					found.push_back(*itr);
			
				itr++;
			}
			return found;
		}

		inline std::optional<V> FindFirstMatch(ValueBoolFunction function)
		{
			MutexGuardian guardian(DataMutex);
			for (auto itr = Data.begin(); itr != Data.end();)
			{
				if (function(*itr))
					return *itr;
			
				itr++;
			}
			return std::nullopt;
		}

		inline void Replace(std::vector<V>& other)
		{
			MutexGuardian guardian(DataMutex);
			Data = other;
		}

	};
}