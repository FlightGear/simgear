#ifndef NASALEMESARYINTERFACE_INCLUDED
#define NASALEMESARYINTERFACE_INCLUDED 1
// Nasal Emesary receipient interface.
//
// Copyright (C) 2019  Richard Harrison rjh@zaretto.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>

#include <simgear/emesary/Emesary.hxx>
#include <simgear/emesary/notifications.hxx>

#include <simgear/debug/logstream.hxx>

#include <simgear/threads/SGThread.hxx>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace nasal
{
    extern"C" {
        extern int GCglobalAlloc();
        extern int naGarbageCollect();
        // these are used by the detailed debug in the Nasal GC.
        SGTimeStamp global_timestamp;
        void global_stamp() {
            global_timestamp.stamp();
        }
        extern int global_elapsedUSec()
        {
            return global_timestamp.elapsedUSec();
        }
    }

    class ThreadedGarbageCollector : public SGExclusiveThread {
    public:
        ThreadedGarbageCollector() : SGExclusiveThread() {}
        virtual ~ThreadedGarbageCollector() {}

        virtual int process(){
            return naGarbageCollect();
        }
    };

    class NasalMainLoopRecipient : public simgear::Emesary::IReceiver {
    public:
        NasalMainLoopRecipient() : receiveCount(0), Active(false), CanWait(false) {
            simgear::Emesary::GlobalTransmitter::instance()->Register(*this);
        }
        virtual ~NasalMainLoopRecipient() {
            simgear::Emesary::GlobalTransmitter::instance()->DeRegister(*this);
        }

        std::atomic<int> receiveCount;
        virtual simgear::Emesary::ReceiptStatus Receive(simgear::Emesary::INotification &n)
        {

            simgear::Notifications::MainLoopNotification *mln = dynamic_cast<simgear::Notifications::MainLoopNotification *>(&n);

            if (mln) {
                switch (mln->GetValue()) {
                case simgear::Notifications::MainLoopNotification::Type::Begin:
                    if (gct.is_running()) {
                        if (Active && CanWait)
                            gct.awaitCompletion();
                        else
                            gct.clearAwaitCompletionTime();
                    }
                    break;
                case simgear::Notifications::MainLoopNotification::Type::End:
                    if (Active) {
                        if (gct.is_running())
                            gct.release();
                    }
                    break;
                case simgear::Notifications::MainLoopNotification::Type::Started:
                    gct.ensure_running();
                    break;
                case simgear::Notifications::MainLoopNotification::Type::Stopped:
                    gct.terminate();
                    break;
                }
                return simgear::Emesary::ReceiptStatusOK;
            }

            auto *gccn = dynamic_cast<simgear::Notifications::NasalGarbageCollectionConfigurationNotification *>(&n);
            if (gccn) {
                CanWait = gccn->GetCanWait();
                Active = gccn->GetActive();
                return simgear::Emesary::ReceiptStatusOK;
            }
            return simgear::Emesary::ReceiptStatusNotProcessed;
        }
    protected:
        bool CanWait;
        bool Active;
        ThreadedGarbageCollector gct;
    };
   
} // namespace nasal
#endif
