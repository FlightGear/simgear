#ifndef EMESARY_hxx
#define EMESARY_hxx

// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2019 Richard Harrison <richard@zaretto.com>

/**
 * @file
 * @brief Emesary main implementation - class based inter-object communication.
 *
 * Provides generic inter-object communication. For an object to receive a message
 * it must first register with a Transmitter, such as GlobalTransmitter, and implement
 * the IReceiver interface. That's it. To send a message use a Transmitter with an object.
 * That's all there is to it.
 *
 * @see http://www.chateau-logic.com/content/class-based-inter-object-communication
 */

#include <typeinfo>

#include "ReceiptStatus.hxx"
#include "INotification.hxx"
#include "IReceiver.hxx"
#include "ITransmitter.hxx"
#include "Transmitter.hxx"
#include <simgear/structure/Singleton.hxx>

namespace simgear
{
    namespace Emesary
    {
        class GlobalTransmitter : public simgear::Singleton<Transmitter>
        {
        public:
            GlobalTransmitter()
            {
            }
            virtual ~GlobalTransmitter() {}
        };
    }
}
#endif
