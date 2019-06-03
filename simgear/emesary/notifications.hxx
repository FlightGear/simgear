/*---------------------------------------------------------------------------
*
*	Title                : Emesary - class based inter-object communication
*
*	File Type            : Implementation File
*
*	Description          : Provides generic inter-object communication. For an object to receive a message it
*	                     : must first register with a Transmitter, such as GlobalTransmitter, and implement the
*	                     : IReceiver interface. That's it.
*	                     : To send a message use a Transmitter with an object. That's all there is to it.
*
*  References           : http://www.chateau-logic.com/content/class-based-inter-object-communication
*
*	Author               : Richard Harrison (richard@zaretto.com)
*
*	Creation Date        : 18 March 2002, rewrite 2017
*
*	Version              : $Header: $
*
*  Copyright © 2002 - 2017 Richard Harrison           All Rights Reserved.
*
*---------------------------------------------------------------------------*/
#include <typeinfo>

#include <string>
#include <list>
#include <set>
#include <vector>
#include <Windows.h>
#include <process.h>
#include <atomic>
#include <simgear/emesary/emesary.hxx>

namespace simgear
{
    namespace Notifications
    {
        class MainLoopNotification : public simgear::Emesary::INotification
        {
        public:
            enum Type { Started, Stopped, Begin, End };
            MainLoopNotification(Type v) : Type(v) {}

            virtual Type GetValue() { return Type; }
            virtual const char *GetType() { return "MainLoop"; }

        protected:
            Type Type;
        };

        class NasalGarbageCollectionConfigurationNotification : public simgear::Emesary::INotification
        {
        public:
            NasalGarbageCollectionConfigurationNotification(bool canWait, bool active) : CanWait(canWait), Active(active) {}

            virtual bool GetCanWait() { return CanWait; }
            virtual bool GetActive() { return Active; }
            virtual const char *GetType() { return "NasalGarbageCollectionConfiguration"; }
            virtual bool SetWait(bool wait) {
                if (wait == CanWait)
                    return false;
                CanWait = wait;
                return true;
            }
            virtual bool SetActive(bool active) {
                if (active == Active)
                    return false;
                Active = active;
                return true;
            }
        public:
            bool CanWait;
            bool Active;
        };
    }
}