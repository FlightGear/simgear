// SGThread - Simple pthread class wrappers.
//
// Written by Bernie Bright, started April 2001.
//
// Copyright (C) 2001  Bernard Bright - bbright@bigpond.net.au
// Copyright (C) 2011  Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
# include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

#include "SGThread.hxx"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <climits>

#if _WIN32
# include <list>
# include <windows.h>
#else
# include <pthread.h>
# include <cassert>
# include <cerrno>
# include <sys/time.h>
#endif

struct SGWaitCondition::PrivateData {
    PrivateData(void)
    {
    }
    ~PrivateData(void)
    {
    }

    void signal(void)
    {
        bool waiting;

        {
            std::lock_guard<std::mutex> lock(_mtx);
            waiting = !ready;
            ready = true;
        }

        if (waiting) {
            _condition.notify_one();
        }
    }

    void broadcast(void)
    {
        bool waiting;

        {
            std::lock_guard<std::mutex> lock(_mtx);
            waiting = !ready;
            ready = true;
        }

        if (waiting) {
            _condition.notify_all();
        }
    }

    void wait(std::mutex& mutex) noexcept
    {
        mutex.unlock();

        {
            std::unique_lock<std::mutex> lock(_mtx);
            _condition.wait(lock, [this]{ return ready; } );
            ready = false;
        }

#ifndef NDEBUG
# if _WIN32
        native_handle_type *m = mutex.native_handle();
        assert(m->LockCount == 0);
# else
        pthread_mutex_t *m = mutex.native_handle();
        assert(m->__data.__count == 0);
# endif
#endif
        mutex.lock();
    }

    bool wait(std::mutex& mutex, unsigned msec) noexcept
    {
        auto timeout = std::chrono::milliseconds(msec);

        mutex.unlock();

        {
            std::unique_lock<std::mutex> lock(_mtx);
            _condition.wait_for(lock, timeout, [this]{ return ready; } );
            ready = false;
        }

#ifndef NDEBUG
# if _WIN32
        native_handle_type *m = mutex.native_handle();
        assert(m->LockCount == 0);
# else
        pthread_mutex_t *m = mutex.native_handle();
        assert(m->__data.__count == 0);
# endif
#endif
        mutex.lock();

        return true;
    }

    std::mutex _mtx;
    std::condition_variable _condition;
    std::atomic<std::thread::id> m_holder;

private:
    bool ready = false;
};

struct SGThread::PrivateData {
    PrivateData()
    {
    }
    ~PrivateData()
    {
        // If we are still having a started thread and nobody waited,
        // now detach ...
        if (!_started)
            return;
        _thread.detach();
    }

    static void *start_routine(void* data)
    {
        SGThread* thread = reinterpret_cast<SGThread*>(data);
        thread->run();
        return 0;
    }

    bool start(SGThread& thread)
    {
        if (_started)
            return false;

        try {
           _thread = std::thread(start_routine, &thread);
        } catch (std::runtime_error &ex) {
            return false;
        }

        _started = true;
        return true;
    }

    void join()
    {
        if (!_started)
            return;

        _thread.join();
        _started = false;
    }

    std::thread _thread;
    bool _started = false;
};

SGThread::SGThread() :
    _privateData(new PrivateData)
{
}

SGThread::~SGThread()
{
    delete _privateData;
    _privateData = 0;
}

bool
SGThread::start()
{
    return _privateData->start(*this);
}

void
SGThread::join()
{
    _privateData->join();
}

long SGThread::current( void )
{
#ifdef _WIN32
    return (long)GetCurrentThreadId();
#else
    return (long)pthread_self();
#endif
}


SGWaitCondition::SGWaitCondition() :
    _privateData(new PrivateData)
{
}

SGWaitCondition::~SGWaitCondition()
{
    delete _privateData;
    _privateData = 0;
}

void
SGWaitCondition::wait(std::mutex& mutex)
{
    _privateData->wait(mutex);
}

bool
SGWaitCondition::wait(std::mutex& mutex, unsigned msec)
{
    return _privateData->wait(mutex, msec);
}

void
SGWaitCondition::signal()
{
    _privateData->signal();
}

void
SGWaitCondition::broadcast()
{
    _privateData->broadcast();
}


SGExclusiveThread::SGExclusiveThread() :
        _started(false), _terminated(false), last_await_time(0),
        dataReady(false), complete(true), process_ran(false), process_running(false)
    {
    }

    SGExclusiveThread::~SGExclusiveThread()
    {

    }

    void SGExclusiveThread::release() {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!complete) {
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  not finished - skipping");
            return;
        }
        if (!complete.exchange(false))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  concurrent failure (2)");
        if (dataReady.exchange(true))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  concurrent failure (1)");
        condVar.notify_one();
    }
    void SGExclusiveThread::wait() {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!dataReady)
        {
            do
            {
                condVar.wait(lck);
            } while (!dataReady);
        }
    }
    void SGExclusiveThread::clearAwaitCompletionTime() {
        last_await_time = 0;
    }
    void SGExclusiveThread::awaitCompletion() {
        timestamp.stamp();
        std::unique_lock<std::mutex> lck(Cmutex_);
        if (!complete)
        {
            do {
                CcondVar.wait(lck);
            } while (!complete.load());
        }

        if (process_ran) {
            last_await_time = timestamp.elapsedUSec();
            process_ran = 0;
        }
    }

    void SGExclusiveThread::setCompletion() {
        std::unique_lock<std::mutex> lck(Cmutex_);
        if (!dataReady.exchange(false))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  atomic operation on dataReady failed (5)\n");

        if (complete.exchange(true))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  atomic operation on complete failed (5)\n");
        CcondVar.notify_one();
    }
    void SGExclusiveThread::run()
    {
        process_running = true;
        while (!_terminated) {
            wait();
            process_ran = process();
            setCompletion();
        }
        process_running = false;
        _terminated = false;
        _started = false;
    }

    void SGExclusiveThread::terminate() {
        _terminated = true;
        release();
        join();
    }
    bool SGExclusiveThread::stop()
    {
        return true;
    }
    void SGExclusiveThread::ensure_running()
    {
        if (!_started)
        {
            _started = true;
            start();
        }
    }
    bool SGExclusiveThread::is_running()
    {
        return process_running;
    }
