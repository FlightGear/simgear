// SGThread - Simple pthread class wrappers.
//
// Written by Bernie Bright, started April 2001.
//
// Copyright (C) 2001  Bernard Bright - bbright@bigpond.net.au
// Copyright (C) 2011  Mathias Froehlich
// Copyright (C) 2020  Erik Hofman
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
# include <windows.h>
#else
# include <pthread.h>
#endif


SGThread::SGThread()
{
}

SGThread::~SGThread()
{
    // If we are still having a started thread and nobody waited,
    // now detach ...
    if (!_started)
        return;
    _thread.detach();
}

bool
SGThread::start()
{
    if (_started)
        return false;

    try {
       _thread = std::thread(start_routine, this);
    } catch (std::runtime_error &ex) {
        return false;
    }

    _started = true;
    return true;
}

void
SGThread::join()
{
    if (!_started)
        return;

    _thread.join();
    _started = false;
}

long SGThread::current( void )
{
#ifdef _WIN32
    return (long)GetCurrentThreadId();
#else
    return (long)pthread_self();
#endif
}

void *SGThread::start_routine(void* data)
{
    SGThread* thread = reinterpret_cast<SGThread*>(data);
    thread->run();
    return 0;
}


SGWaitCondition::SGWaitCondition()
{
}

SGWaitCondition::~SGWaitCondition()
{
}

void
SGWaitCondition::wait(std::mutex& mutex)
{
#ifndef NDEBUG
    try {
        mutex.unlock();
    } catch(const std::system_error& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unlocking error " << e.what() );
    }
#else
    mutex.unlock();
#endif

    {
        std::unique_lock<std::mutex> lock(_mtx);
        _condition.wait(lock, [this]{ return ready; } );
        ready = false;
    }

#ifndef NDEBUG
    try {
        mutex.lock();
    } catch(const std::system_error& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Locking error " << e.what() );
    }
#else
    mutex.lock();
#endif
}

bool
SGWaitCondition::wait(std::mutex& mutex, unsigned msec)
{
    auto timeout = std::chrono::milliseconds(msec);

    mutex.unlock();

    {
        std::unique_lock<std::mutex> lock(_mtx);
        _condition.wait_for(lock, timeout, [this]{ return ready; } );
        ready = false;
    }

    mutex.lock();

    return true;
}

void
SGWaitCondition::signal()
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

void
SGWaitCondition::broadcast()
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
