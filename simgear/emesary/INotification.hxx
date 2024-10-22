#ifndef INOTIFICATION_hxx
#define INOTIFICATION_hxx

// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2019 Richard Harrison <richard@zaretto.com>

/**
 * @file
 * @brief Base class (interface) for all Notifications.
 *
 * This is also compatible with the usual implementation of how we
 * implement queued notifications.
 *
 * @see http://www.chateau-logic.com/content/class-based-inter-object-communication
 */

#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear
{
    namespace Emesary
    {
        /// Interface (base class) for all notifications. 
        class INotification: public SGReferenced
        {
        public:
            virtual ~INotification()
            {
                
            }
            // text representation of notification type. must be unique across all notifications
            virtual const char *GetType() = 0;

            /// Used to control the sending of notifications. If this returns false then the Transmitter 
            /// should not send this notification.
            virtual bool IsReadyToSend() { return true; }

            /// Used to control the timeout. If this notification has timed out - then the processor is entitled 
            /// to true.
            virtual bool IsTimedOut() { return false; }

            /// when this notification has completed the processing recipient must set this to true.
            /// the processing recipient is responsible for follow on notifications.
            /// a notification can remain as complete until the transmit queue decides to remove it from the queue.
            /// there is no requirement that elements are removed immediately upon completion merely that once complete
            /// the transmitter should not notify any more elements.
            /// The current notification loop may be completed - following the usual convention unless Completed or Abort
            /// is returned as the status.
            virtual bool IsComplete() { return true; }
        };
        typedef SGSharedPtr<INotification> INotificationPtr;
    }
}
#endif
