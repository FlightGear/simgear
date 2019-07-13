#ifndef EMESARY_hxx
#define EMESARY_hxx
/*---------------------------------------------------------------------------
*
*  Title                : Emesary - class based inter-object communication
*
*  File Type            : Implementation File
*
*  Description          : Provides generic inter-object communication. For an object to receive a message it
*                       : must first register with a Transmitter, such as GlobalTransmitter, and implement the
*                       : IReceiver interface. That's it.
*                       : To send a message use a Transmitter with an object. That's all there is to it.
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
