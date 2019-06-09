#ifndef IRECEIVER_hxx
#define IRECEIVER_hxx
/*---------------------------------------------------------------------------
*
*  Title                : Emesary - Receiver base class
*
*  File Type            : Implementation File
*
*  Description          : Base class for all recipients.
*
*  References           : http://www.chateau-logic.com/content/class-based-inter-object-communication
*
*  Author               : Richard Harrison (richard@zaretto.com)
*
*  Creation Date        : 18 March 2002, rewrite 2017, simgear version 2019
*
*  Version              : $Header: $
*
*  Copyright (C)2019 Richard Harrison            Licenced under GPL2 or later.
*
*---------------------------------------------------------------------------*/
namespace simgear
{
    namespace Emesary
    {

        /// Interface (base class) for a recipeint.
        class IReceiver
        {
        public:
            /// Receive notification - must be implemented
            virtual ReceiptStatus Receive(INotification& message) = 0;

            /// Called when registered at a transmitter
            virtual void OnRegisteredAtTransmitter(class Transmitter *p)
            {
            }

            /// Called when de-registered at a transmitter
            virtual void OnDeRegisteredAtTransmitter(class Transmitter *p)
            {
            }
        };

    }
}
#endif