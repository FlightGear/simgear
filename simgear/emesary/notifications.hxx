#ifndef NOTIFICATIONS_hxx
#define NOTIFICATIONS_hxx
/*---------------------------------------------------------------------------
*
*	Title                : Emesary - class based inter-object communication
*
*	File Type            : Implementation File
*
*	Description          : simgear notifications
*
*  References           : http://www.chateau-logic.com/content/class-based-inter-object-communication
*
*	Author               : Richard Harrison (richard@zaretto.com)
*
*	Creation Date        : 18 March 2002, rewrite 2017
*
*	Version              : $Header: $
*
*  Copyright � 2002 - 2017 Richard Harrison           All Rights Reserved.
*
*---------------------------------------------------------------------------*/

#include "INotification.hxx"

namespace simgear
{
    namespace Notifications
    {
        class MainLoopNotification : public simgear::Emesary::INotification
        {
        public:
            enum class Type { Started, Stopped, Begin, End };
            MainLoopNotification(Type v) : _type(v) {}

            virtual Type GetValue() { return _type; }
            virtual const char *GetType() { return "MainLoop"; }

        protected:
            Type _type;
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
#endif
