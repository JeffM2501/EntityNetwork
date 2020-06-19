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
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace EntityNetwork
{
	class PropertyData
	{
	public:
		typedef std::shared_ptr<PropertyData> Ptr;
		typedef std::vector<PropertyData> Vec;

		const PropertyDesc& Descriptor;

		void*	DataPtr = nullptr;
		size_t DataLenght = 0;

		PropertyData(const PropertyDesc& desc) : Descriptor(desc)
		{
			DataLenght = 0;
			switch (desc.DataType)
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
				DataLenght = desc.BufferSize == 0 ? 64 : desc.BufferSize;
				break;

			case PropertyDesc::DataTypes::Buffer:
				DataLenght = desc.BufferSize;
				break;
			}

			DataPtr = (void*) new char[DataLenght];
		}

		static inline Ptr MakeShared(const PropertyDesc& desc)
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
			if (Descriptor.DataType != PropertyDesc::DataTypes::Integer)
				return;

			*(int*)DataPtr = val;
		}

		inline int GetValueI()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Integer)
				return 0;

			return *(int*)DataPtr;
		}

		inline void SetValue3I(int val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3I)
				return;
			for(int i = 0; i < 3; i++)
				((int*)DataPtr)[i] = val[i];
		}

		inline int* GetValue3I()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3I)
				return nullptr;

			return (int*)DataPtr;
		}

		inline void SetValue4I(int val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4I)
				return;
			for (int i = 0; i < 4; i++)
				((int*)DataPtr)[i] = val[i];
		}

		inline int* GetValue4I()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4I)
				return nullptr;

			return (int*)DataPtr;
		}

		inline void SetValueF(float val)
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Float)
				return;

			*(float*)DataPtr = val;
		}

		inline float GetValueF()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Float)
				return 0;

			return *(float*)DataPtr;
		}

		inline void SetValue3F(float val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3F)
				return;
			for (int i = 0; i < 3; i++)
				((float*)DataPtr)[i] = val[i];
		}

		inline float* GetValue3F()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3F)
				return nullptr;

			return (float*)DataPtr;
		}

		inline void SetValue4F(float val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4F)
				return;
			for (int i = 0; i < 4; i++)
				((float*)DataPtr)[i] = val[i];
		}

		inline float* GetValue4F()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4F)
				return nullptr;

			return (float*)DataPtr;
		}

		inline void SetValueD(double val)
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Double)
				return;
			*(double*)DataPtr = val;
		}

		inline double GetValueD()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Double)
				return 0;

			return *(double*)DataPtr;
		}

		inline void SetValue3D(double val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3D)
				return;
			for (int i = 0; i < 3; i++)
				((double*)DataPtr)[i] = val[i];
		}

		inline double* GetValue3D()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector3D)
				return nullptr;

			return (double*)DataPtr;
		}

		inline void SetValue4D(double val[])
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4D)
				return;
			for (int i = 0; i < 4; i++)
				((double*)DataPtr)[i] = val[i];
		}

		inline double* GetValue4D()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Vector4D)
				return nullptr;

			return (double*)DataPtr;
		}

		inline void SetValueStr(const char* value)
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::String)
				return;

			DataLenght = strlen(value)+1;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[DataLenght];
			memcpy(DataPtr, value, DataLenght-1);
			((char*)DataPtr)[DataLenght] = '\0';
		}

		inline std::string GetValueStr()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::String)
				return std::string();

			return std::string((const char*)DataPtr);
		}

		inline void SetValueBuffer(void* value, size_t lenght)
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Buffer)
				return;

			DataLenght = lenght;
			if (DataPtr != nullptr)
				delete[] DataPtr;

			DataPtr = (void*) new char[lenght];
			memcpy(DataPtr, value, DataLenght);
		}

		inline void* GetValueBuffer()
		{
			if (Descriptor.DataType != PropertyDesc::DataTypes::Buffer)
				return nullptr;

			return DataPtr;
		}

		inline void PackValue(MessageBufferBuilder& builder)
		{
			builder.AddByte(Descriptor.ID);
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
			}
		}
	};
}