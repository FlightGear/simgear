#ifndef ITRANSMITTER_hxx
#define ITRANSMITTER_hxx
/*---------------------------------------------------------------------------
*
*  Title                : Emesary - Transmitter base class
*
*  File Type            : Implementation File
*
*  Description          : Base class for all transmitters.
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
        /// Interface (base clasee) for a transmitter.
        ///  Transmits Message derived objects. Each instance of this class provides a
        ///  event/databus to which any number of receivers can attach to.
        class ITransmitter
        {
        public:
            // Registers a recipient to receive message from this transmitter
            virtual void Register(IReceiver& R) = 0;
            // Removes a recipient from from this transmitter
            virtual void DeRegister(IReceiver& R) = 0;


            //Notify all registered recipients. Stop when receipt status of abort or finished are received.
            //The receipt status from this method will be
            // - OK > message handled
            // - Fail > message not handled. A status of Abort from a recipient will result in our status
            //          being fail as Abort means that the message was not and cannot be handled, and
            //          allows for usages such as access controls.
            virtual ReceiptStatus NotifyAll(INotification& M) = 0;

            /// number of recipients
            virtual int Count() = 0;
        };
    }
}
#endif
