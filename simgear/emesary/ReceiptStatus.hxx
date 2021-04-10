#ifndef RECEIPTSTATUS_hxx
#define RECEIPTSTATUS_hxx
/*---------------------------------------------------------------------------
*
*  Title                : Emesary - Transmitter base class
*
*  File Type            : Implementation File
*
*  Description          : Defines the receipt status that can be returned from 
*                       : a receive method.
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
    enum class ReceiptStatus
        {
            /// Processing completed successfully
        OK = 0,

            /// Individual item failure
        Fail = 1,

            /// Fatal error; stop processing any further recipieints of this message. Implicitly fail
        Abort = 2,

            /// Definitive completion - do not send message to any further recipieints
        Finished = 3,

            /// Return value when method doesn't process a message.
        NotProcessed = 4,

            /// Message has been sent but the return status cannot be determined as it has not been processed by the recipient. 
            /// e.g. a queue or outgoing bridge
        Pending = 5,

            /// Message has been definitively handled but the return value cannot be determined. The message will not be sent any further
            /// e.g. a point to point forwarding bridge
        PendingFinished = 6,
        };
    }
}
#endif
