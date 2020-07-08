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

#include "PropertyDescriptor.h"
#include "MutexedMessageBuffer.h"
#include "ThreadTools.h"
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace EntityNetwork
{
	typedef unsigned char revision_t;

	class PropertyData
	{
	private:
		bool Dirty = false;
		std::mutex DirtyMutex;

		revision_t Revision = 0;

		void SetDirty()
		{
			MutexGuardian guard(DirtyMutex);
			Dirty = true;
			Revision++;
		}

	public:
		typedef std::vector<PropertyData> Vec;
		typedef std::shared_ptr<PropertyData> Ptr;

		static const std::vector<Ptr> EmptyArgs;

		PropertyDesc::Ptr	 Descriptor;

		void*	DataPtr = nullptr;
		size_t DataLenght = 0;

		inline bool IsDirty()
		{
			MutexGuardian guard(DirtyMutex);
			return Dirty;
		}

		inline revision_t GetRevision()
		{
			MutexGuardian guard(DirtyMutex);
			return Revision;
		}

		inline void SetClean()
		{
			MutexGuardian guard(DirtyMutex);
			Dirty = false;
		}

		PropertyData(PropertyDesc::Ptr desc) : Descriptor(desc)
		{
			DataLenght = 0;
			if (desc == nullptr)
				return;

			switch (desc->DataType)
			{
			case PropertyDesc::DataTypes::Integer:
			case PropertyDesc::DataTypes::Float:
				DataLenght = 4;
				break;

			case PropertyDesc::DataTypes::Vector3I:
			case PropertyDesc::DataTypes::Vector3F:
				DataLenght = 4 * 3;
				break;

			case PropertyDesc::DataTypes::Vector4I:
			case PropertyDesc::DataTypes::Vector4F:
				DataLenght = 4 * 4;
				break;

			case PropertyDesc::DataTypes::Double:
				DataLenght = 8;
				break;

			case PropertyDesc::DataTypes::Vector3D:
				DataLenght = 8 * 3;
				break;

			case PropertyDesc::DataTypes::Vector4D:
				DataLenght = 8 * 4;
				break;

			case PropertyDesc::DataTypes::String:
				DataLenght = desc->BufferSize == 0 ? 64 : desc->BufferSize;
				break;

			case PropertyDesc::DataTypes::Buffer:
				DataLenght = desc->BufferSize;
				break;

			case PropertyDesc::DataTypes::StateV3F:
				DataLenght = 8 + (4 * 3);
				break;

			case PropertyDesc::DataTypes::StateV3FQ4F:
				DataLenght = 8 + (4 * 7);
				break;

			}

			DataPtr = (void*) new char[DataLenght];
		}

		static inline std::shared_ptr<PropertyData> MakeShared(PropertyDesc::Ptr desc)
		{
			return std::make_shared<PropertyData>(desc);
		}

		virtual ~PropertyData()
		{
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = nullptr;
			DataLenght = 0;
		}

		inline void SetValueI(int val)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Integer)
				return;

			*(int*)DataPtr = val;
			SetDirty();
		}

		inline int GetValueI()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Integer)
				return 0;

			return *(int*)DataPtr;
		}

		inline void SetValue3I(int val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3I)
				return;
			for(int i = 0; i < 3; i++)
				((int*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline int* GetValue3I()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3I)
				return nullptr;

			return (int*)DataPtr;
		}

		inline void SetValue4I(int val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4I)
				return;
			for (int i = 0; i < 4; i++)
				((int*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline int* GetValue4I()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4I)
				return nullptr;

			return (int*)DataPtr;
		}

		inline void SetValueF(float val)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Float)
				return;

			*(float*)DataPtr = val;
			SetDirty();
		}

		inline float GetValueF()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Float)
				return 0;

			return *(float*)DataPtr;
		}

		inline void SetValue3F(float val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3F)
				return;
			for (int i = 0; i < 3; i++)
				((float*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline float* GetValue3F()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3F)
				return nullptr;

			return (float*)DataPtr;
		}

		inline bool GetValue3F(float val[3])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3F)
				return false;

			memcpy(val, DataPtr, 4 * 3);

			return true;
		}

		inline void SetValue4F(float val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4F)
				return;
			for (int i = 0; i < 4; i++)
				((float*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline float* GetValue4F()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4F)
				return nullptr;

			return (float*)DataPtr;
		}

		inline bool GetValue4F(float val[4])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4F)
				return false;

			memcpy(val, DataPtr, 4 * 4);

			return true;
		}

		inline void SetValueD(double val)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Double)
				return;
			*(double*)DataPtr = val;
			SetDirty();
		}

		inline double GetValueD()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Double)
				return 0;

			return *(double*)DataPtr;
		}

		inline void SetValue3D(double val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3D)
				return;
			for (int i = 0; i < 3; i++)
				((double*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline double* GetValue3D()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3D)
				return nullptr;

			return (double*)DataPtr;
		}

		inline bool GetValue3D(double val[3])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector3D)
				return false;

			memcpy(val, DataPtr, 8 * 3);

			return true;
		}

		inline void SetValue4D(double val[])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4D)
				return;
			for (int i = 0; i < 4; i++)
				((double*)DataPtr)[i] = val[i];
			SetDirty();
		}

		inline double* GetValue4D()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4D)
				return nullptr;

			return (double*)DataPtr;
		}

		inline bool GetValue4F(double val[4])
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Vector4D)
				return false;

			memcpy(val, DataPtr, 8 * 4);

			return true;
		}

		inline void SetValueStr(const char* value)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::String)
				return;

			DataLenght = strlen(value)+1;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[DataLenght];
			memcpy(DataPtr, value, DataLenght-1);
			((char*)DataPtr)[DataLenght - 1] = '\0';
			SetDirty();
		}

		inline void SetValueStr(const std::string& value)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::String)
				return;

			DataLenght = value.size() + 1;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[DataLenght];
			memcpy(DataPtr, value.c_str(), value.size());
			((char*)DataPtr)[value.size()] = '\0';
			SetDirty();
		}

		inline std::string GetValueStr()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::String)
				return std::string();

			return std::string((const char*)DataPtr);
		}

		inline void SetValueBuffer(void* value, size_t lenght)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Buffer)
				return;

			DataLenght = lenght;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[lenght];
			memcpy(DataPtr, value, DataLenght);
			SetDirty();
		}

		inline void* GetValueBuffer()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Buffer)
				return nullptr;

			return DataPtr;
		}

		inline void SetValueStateUpdatePos(StateUpdatePos val)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::StateV3F)
				return;
			memcpy(DataPtr, &val, DataLenght);
			SetDirty();
		}

		inline StateUpdatePos GetValueStateUpdatePos()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::StateV3F)
				return StateUpdatePos();

			return *(StateUpdatePos*)DataPtr;
		}

		inline void SetValueStateUpdatePosRot(StateUpdatePosRot val)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::StateV3FQ4F)
				return;
			memcpy(DataPtr, &val, DataLenght);
			SetDirty();
		}

		inline StateUpdatePosRot GetValueStateUpdatePosRot()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::StateV3FQ4F)
				return StateUpdatePosRot();

			return *(StateUpdatePosRot*)DataPtr;
		}

		inline void SetValueWriter(MessageBufferBuilder& builder)
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Buffer)
				return;

			int offset = 0;
			if (builder.Command != MessageCodes::NoCode)
				offset = 1;

			DataLenght = builder.Data.size() - offset;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[DataLenght];
			memcpy(DataPtr, &builder.Data[offset], DataLenght);
			SetDirty();
		}

		inline MessageBufferReader GetValueReader()
		{
			if (Descriptor->DataType != PropertyDesc::DataTypes::Buffer)
				return MessageBufferReader(nullptr);

			return MessageBufferReader(MessageBuffer::MakeShared(DataPtr, DataLenght, false), false);
		}

		inline void PackValue(MessageBufferBuilder& builder)
		{
			builder.AddByte(Descriptor->ID);
			builder.AddBuffer(DataPtr, DataLenght);
		}

		inline void UnpackValue(MessageBufferReader& reader, bool save)
		{
			if (!save)
				reader.Advance(reader.PeakBufferSize() + 2);
			else
			{
				DataLenght = reader.PeakBufferSize();
				if (DataPtr != nullptr)
					delete[] DataPtr;

				DataPtr = (void*) new char[DataLenght];
				reader.ReadBuffer(DataPtr);

				MutexGuardian guard(DirtyMutex);
				Dirty = true;
			}
		}
	};
}