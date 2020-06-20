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
#include "client/ClientWorld.h"
#include "MutexedMessageBuffer.h"

namespace EntityNetwork
{
	namespace Client
	{
		void ClientWorld::Update()
		{
			if (Self == nullptr || EntityControllerProperties.Size() == 0)
				return;

			std::vector<MessageBuffer::Ptr> pendingMods;
			MessageBufferBuilder builder;
			builder.Command = MessageCodes::SetControllerPropertyValues;
			for (auto prop : Self->GetDirtyProperties())
			{
				if (prop->Descriptor.UpdateFromClient())
				{
					builder.AddInt(prop->Descriptor.ID);
					prop->PackValue(builder);
				}
				pendingMods.push_back(builder.Pack());
			}

			for (auto msg : pendingMods)
				Send(msg);
		}

		void ClientWorld::AddInboundData(MessageBuffer::Ptr message)
		{
			MessageBufferReader reader(message);

			switch (reader.Command)
			{
			case MessageCodes::AddControllerProperty:
			case MessageCodes::RemoveControllerProperty:
				HandlePropteryDescriptorMessage(reader);
				break;

			case MessageCodes::AcceptController:
			{
				auto id = reader.ReadID();
				Self = CreateController(id, true);
				Self->IsSelf = true;
			}
			break;

			case MessageCodes::AddController:
			{
				auto id = reader.ReadID();
				ClientEntityController::Ptr subject = nullptr;

				if (Self != nullptr && id == Self->GetID())
					subject = Self;
				else
				{
					subject = CreateController(id, false);
					Peers.Insert(id, subject);
				}
				SetupEntityController(static_cast<EntityController&>(*subject));

				while (!reader.Done())
				{
					auto prop = subject->FindPropertyByID(reader.ReadByte());
					prop->UnpackValue(reader, true); // always save the initial data 
				}
				
				ControllerEvents.Call(subject->IsSelf ? ControllerEventTypes::SelfCreated : ControllerEventTypes::RemoteCreated, [&subject](auto func) {func(subject); });
				subject->GetDirtyProperties(); // just clear out anything that god dirty due to an unpack, we always start clean

				if (subject->IsSelf)
					StateEvents.Call(StateEventTypes::ActiveSyncing, [](auto func) {func(StateEventTypes::ActiveSyncing); });
			}
			break;

			case MessageCodes::RemoveController:
				break;

			case MessageCodes::SetControllerPropertyValues:
			{
				auto subject = PeerFromID(reader.ReadID());
				if (subject != nullptr)
				{
					while (!reader.Done())
					{
						auto prop = subject->FindPropertyByID(reader.ReadByte());
						prop->UnpackValue(reader, prop->Descriptor.UpdateFromServer());
						
						if (prop->Descriptor.UpdateFromServer())
							PropertyEvents.Call(subject->IsSelf ? PropertyEventTypes::SelfPropteryChanged : PropertyEventTypes::RemoteControllerPropertyChanged, [&subject,&prop](auto func) {func(subject, prop->Descriptor.ID); });

						prop->SetClean(); // remote properties are never dirty, only locally set ones
					}
				}
			}
			break;

			case MessageCodes::NoOp:
			default:
				break;
			}
		}

		MessageBuffer::Ptr ClientWorld::PopOutboundData()
		{
			if (Self == nullptr)
				return nullptr;

			return Self->OutboundMessages.Pop();
		}

		void ClientWorld::Send(MessageBuffer::Ptr message)
		{
			if (Self == nullptr)
				return;

			Self->OutboundMessages.Push(message);
		}

		ClientEntityController::Ptr ClientWorld::PeerFromID(int64_t id)
		{
			if (Self != nullptr && Self->GetID() == id)
				return Self;
			
			auto peer = Peers.Find(id);
			if (peer == std::nullopt)
				return nullptr;
				return nullptr;

			return *peer;	
		}

		void  ClientWorld::HandlePropteryDescriptorMessage(MessageBufferReader& reader)
		{
			if (EntityControllerProperties.Size() == 0) // first data of anything we get is property descriptors
				StateEvents.Call(StateEventTypes::Negotiating, [](auto func) {func(StateEventTypes::Negotiating); });

			if (reader.Command == MessageCodes::AddControllerProperty)
			{
				PropertyDesc desc;
				desc.ID = reader.ReadInt();
				desc.Name = reader.ReadString();
				desc.DataType = static_cast<PropertyDesc::DataTypes>(reader.ReadByte());
				desc.Scope = static_cast<PropertyDesc::Scopes>(reader.ReadByte());

				RegisterControllerPropertyDesc(desc);
				if (Self != nullptr)	// if we are connected, fix all the other peers
				{
					SetupEntityController(static_cast<EntityController&>(*Self));
					Peers.DoForEach([this](auto id, auto peer) {SetupEntityController(*peer); });
				}
				PropertyEvents.Call(PropertyEventTypes::ControllerPropertyDefAdded, [&desc](auto func) {func(nullptr, desc.ID); });
			}
			else if (reader.Command == MessageCodes::RemoveControllerProperty)
			{
				auto id = reader.ReadInt();

				PropertyEvents.Call(PropertyEventTypes::ControllerPropertyDefRemoved, [id](auto func) {func(nullptr,id); });
				EntityControllerProperties.EraseIf([id](PropertyDesc& desc) {return desc.ID == id; });

				if (Self != nullptr)	// if we are connected, fix all the other peers
				{
					SetupEntityController(static_cast<EntityController&>(*Self));
					Peers.DoForEach([this](auto id, auto peer) {SetupEntityController(*peer); });
				}
			}
		}
	}
}