#ifndef IRECEIVER_hxx
#define IRECEIVER_hxx

// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2019 Richard Harrison <richard@zaretto.com>

/**
 * @file
 * @brief Base class for all recipients.
 * @see http://www.chateau-logic.com/content/class-based-inter-object-communication
 */

namespace simgear
{
    namespace Emesary
    {

        /// Interface (base class) for a recipeint.
        class IReceiver
        {
        public:
            virtual ~IReceiver() = default;

            /// Receive notification - must be implemented
            virtual ReceiptStatus Receive(INotificationPtr message) = 0;

            /// Called when registered at a transmitter
            virtual void OnRegisteredAtTransmitter(class Transmitter *p)
            {
            }

            /// Called when de-registered at a transmitter; i.e. as a result of 
            /// Transmitter::DeRegister.
            virtual void OnDeRegisteredAtTransmitter(class Transmitter *p)
            {
            }
        };
        typedef IReceiver* IReceiverPtr;

    }
}
#endif
