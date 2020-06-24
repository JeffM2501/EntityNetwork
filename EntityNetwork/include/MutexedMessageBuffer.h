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
#include <deque>
#include <memory>
#include "ThreadTools.h"
#include "MutexedVector.h"
#include "Messages.h"

namespace EntityNetwork
{
	class MutexedMessageBufferDeque
	{
		private:
		std::mutex MessageMutex;
		std::deque<MessageBuffer::Ptr> Messages;

	public:
		inline bool Empty()
		{
			MutexGuardian guardian(MessageMutex);
			return Messages.size() == 0;
		}

		inline size_t Size()
		{
			MutexGuardian guardian(MessageMutex);
			return Messages.size();
		}

		inline MessageBuffer::Ptr Pop()
		{
			MutexGuardian guardian(MessageMutex);
			if (Messages.size() == 0)
				return nullptr;

			MessageBuffer::Ptr top = Messages.front();
			Messages.pop_front();
			return top;
		}

		inline void Push(MessageBuffer::Ptr msg)
		{
			MutexGuardian guardian(MessageMutex);
			Messages.push_back(msg);
		}

		inline void AppendRange(MutexedVector<MessageBuffer::Ptr>& newMessages)
		{
			MutexGuardian guardian(MessageMutex);
			auto raw = newMessages.GetExclusiveAccess();
			
			Messages.insert(Messages.end(), raw.begin(), raw.end());
			newMessages.ReleaseExclusiveAccess();
		}
	};
}