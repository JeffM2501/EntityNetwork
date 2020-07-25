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
#include "EntityDescriptor.h"

namespace EntityNetwork
{
	namespace Client
	{
		void ClientWorld::ProcessAddController(MessageBufferReader& reader)
		{
			auto id = reader.ReadID();
			ClientEntityController::Ptr subject = nullptr;

			if (Self != nullptr && id == Self->GetID())
				subject = Self;
			else
				subject = CreateController(id, false);

			Peers.Insert(id, subject);

			SetupEntityController(subject);

			while (!reader.Done())
			{
				auto prop = subject->FindPropertyByID(reader.ReadByte());
				if (prop == nullptr)
					break;
				prop->UnpackValue(reader, true); // always save the initial data 
			}

			ControllerEvents.Call(subject->IsSelf ? ControllerEventTypes::SelfCreated : ControllerEventTypes::RemoteCreated, [&subject](auto func) {func(subject); });
			subject->GetDirtyProperties(); // just clear out anything that god dirty due to an unpack, we always start clean

			if (subject->IsSelf)
			{
				CurrentState = StateEventTypes::ActiveSyncing;
				StateEvents.Call(StateEventTypes::ActiveSyncing, [](auto func) {func(StateEventTypes::ActiveSyncing); });
			}
		}

		void ClientWorld::ProcessSetControllerPropertyData(MessageBufferReader& reader)
		{
			auto subject = PeerFromID(reader.ReadID());
			if (subject != nullptr)
			{
				while (!reader.Done())
				{
					int id = reader.ReadByte();
					auto prop = subject->FindPropertyByID(id);
					if (prop == nullptr)
						return;

					prop->UnpackValue(reader, prop->Descriptor->UpdateFromServer());

					if (prop->Descriptor->UpdateFromServer())
						PropertyEvents.Call(subject->IsSelf ? PropertyEventTypes::SelfPropteryChanged : PropertyEventTypes::RemoteControllerPropertyChanged, [&subject, &prop](auto func) {func(subject, prop->Descriptor->ID); });

					prop->SetClean(); // remote properties are never dirty, only locally set ones
				}
			}
		}
	}
}