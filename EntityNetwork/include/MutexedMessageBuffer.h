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

namespace EntityNetwork
{
	class MessageBuffer
	{
	public:
		void* MessageData = nullptr;
		size_t MessageLenght = 0;

		MessageBuffer(void* data, size_t lenght, bool canOwn = false)
		{
			MessageLenght = lenght;
			if (canOwn)
				MessageData = data;
			else
			{
				MessageData = (void*) new char[lenght];
				memcpy(MessageData, data, lenght);
			}
		}

		MessageBuffer(size_t lenght)
		{
			MessageLenght = lenght;
			MessageData = (void*) new char[lenght];
		}

		virtual ~MessageBuffer()
		{
			if (MessageData != nullptr)
				delete[] MessageData;
		}

		typedef std::shared_ptr<MessageBuffer> Ptr;
		static inline Ptr MakeShared(void* data, size_t len, bool canOwn = false) { return std::make_shared<MessageBuffer>(data, len, canOwn); }
		static inline Ptr MakeShared(size_t len) { return std::make_shared<MessageBuffer>(len); }
	};

	enum class MessageCodes
	{
		NoOp = 0,
		AddController,
		AcceptController,
		RemoveController,
		AddControllerProperty,
		RemoveControllerProperty,
		SetControllerPropertyValues,
		AddWordDataDef,
		SetWorldDataValues,
	};

	class MessageBufferBuilder
	{
	public:
		std::vector<char> Data;

		MessageCodes Command = MessageCodes::NoOp;

	private:
		void Insert(const void* ptr, size_t size)
		{
			const char* p = (const char*)ptr;
			Data.insert(Data.end(), p, p + size);
		}

	public:

		inline MessageBufferBuilder()
		{
			Data.reserve(128);
			Data.push_back(0);
		}

		inline bool Empty()
		{
			return Data.size() > 1;
		}

		inline void Clear()
		{
			Data = std::vector<char>(128);
			Data.push_back(0);
		}

		inline void AddInt(int value)
		{
			Insert(&value, 4);
		}

		inline void AddByte(int value)
		{
			unsigned char c = static_cast<unsigned char>(value);
			Insert(&c, 1);
		}
		
		inline void AddBool(bool value)
		{
			unsigned char c = value ? 1 : 0;
			Insert(&c, 1);
		}

		inline void AddID(int64_t value)
		{
			Insert(&value, 8);
		}

		inline void AddString(const std::string& str)
		{
			uint16_t strLen = (uint16_t)str.length();
			Insert(&strLen, 2);
			Insert(str.c_str(), strLen);
		}

		inline void AddBuffer(void* value, size_t size)
		{
			uint16_t len = (uint16_t)size;
			Insert(&len, 2);
			Insert(value, size);
		}

		inline MessageBuffer::Ptr Pack()
		{
			Data[0] = (char)Command;

			return MessageBuffer::MakeShared(&(Data[0]), Data.size(), false);
		}
	};

	class MessageBufferReader
	{
	public:
		MessageBuffer::Ptr Message;
		size_t ReadOffset =  0;

		MessageCodes Command = MessageCodes::NoOp;

		inline void Reset(MessageBuffer::Ptr data)
		{
			Message = data;
			ReadOffset = 0;
			if (data != nullptr && data->MessageLenght > 0)
			{
				ReadOffset = 1;
				unsigned char p = static_cast<unsigned char*>(data->MessageData)[0];
				Command = static_cast<MessageCodes>(p);
			}
		}

		inline bool Done()
		{
			return Message != nullptr && ReadOffset < Message->MessageLenght;
		}

		inline void End()
		{
			if (Message != nullptr)
				ReadOffset = Message->MessageLenght;
			else
				ReadOffset = 0;
		}

		inline void Advance(size_t size)
		{
			ReadOffset += size;
		}

	private:
		void* Read(size_t size)
		{
			if (ReadOffset + size > Message->MessageLenght)
				return nullptr;

			void* p = (char*)Message->MessageData + ReadOffset;
			ReadOffset += size;

			return p;
		}

	public:

		inline MessageBufferReader(MessageBuffer::Ptr data)
		{
			Reset(data);
		}

		inline int ReadInt()
		{
			void* p = Read(4);
			if (p == nullptr)
				return 0;
			return *static_cast<int*>(p);
		}

		inline int ReadByte()
		{
			void* p = Read(1);
			if (p == nullptr)
				return 0;
			return *static_cast<unsigned char*>(p);
		}

		inline bool ReadBool()
		{
			void* p = Read(1);
			if (p == nullptr)
				return false;
			return static_cast<unsigned char*>(p) != 0;
		}

		inline int64_t ReadID()
		{
			void* p = Read(8);
			if (p == nullptr)
				return 0;
			return *static_cast<int64_t*>(p);
		}

		inline std::string ReadString()
		{
			void* p = Read(2);
			if (p == nullptr)
				return std::string();
			size_t strLen = *(uint16_t*)p;

			p = Read(strLen);
			if (p == nullptr)
				return std::string();

			return std::string(static_cast<const char*>(p), strLen);
		}

		inline size_t PeakBufferSize()
		{
			if (ReadOffset + 2 > Message->MessageLenght)
				return 0;

			void* p = (char*)Message->MessageData + ReadOffset;
			size_t buffLen = *(uint16_t*)p;

			return buffLen;
		}

		inline bool ReadBuffer(void* destinationBuffer)
		{
			void* p = Read(2);
			if (p == nullptr)
				return false;
			size_t buffLen = *(uint16_t*)p;

			p = Read(buffLen);
			if (p == nullptr)
				return false;

			memcpy(destinationBuffer, p, buffLen);

			return true;
		}
	};

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