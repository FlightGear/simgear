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

#include "SGThread.hxx"

#ifdef _WIN32

/////////////////////////////////////////////////////////////////////////////
/// win32 threads
/////////////////////////////////////////////////////////////////////////////

#include <list>
#include <windows.h>

struct SGThread::PrivateData {
    PrivateData() :
        _handle(INVALID_HANDLE_VALUE)
    {
    }
    ~PrivateData()
    {
        if (_handle == INVALID_HANDLE_VALUE)
            return;
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }

    static DWORD WINAPI start_routine(LPVOID data)
    {
        SGThread* thread = reinterpret_cast<SGThread*>(data);
        thread->run();
        return 0;
    }

    bool start(SGThread& thread)
    {
        if (_handle != INVALID_HANDLE_VALUE)
            return false;
        _handle = CreateThread(0, 0, start_routine, &thread, 0, 0);
        if (_handle == INVALID_HANDLE_VALUE)
            return false;
        return true;
    }

    void join()
    {
        if (_handle == INVALID_HANDLE_VALUE)
            return;
        DWORD ret = WaitForSingleObject(_handle, INFINITE);
        if (ret != WAIT_OBJECT_0)
            return;
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }

    HANDLE _handle;
};

long SGThread::current( void ) {
    return (long)GetCurrentThreadId();
}

struct SGMutex::PrivateData {
    PrivateData()
    {
        InitializeCriticalSection((LPCRITICAL_SECTION)&_criticalSection);
    }

    ~PrivateData()
    {
        DeleteCriticalSection((LPCRITICAL_SECTION)&_criticalSection);
    }

    void lock(void)
    {
        EnterCriticalSection((LPCRITICAL_SECTION)&_criticalSection);
    }

    void unlock(void)
    {
        LeaveCriticalSection((LPCRITICAL_SECTION)&_criticalSection);
    }

    CRITICAL_SECTION _criticalSection;
};

struct SGWaitCondition::PrivateData {
    ~PrivateData(void)
    {
        // The waiters list should be empty anyway
        _mutex.lock();
        while (!_pool.empty()) {
            CloseHandle(_pool.front());
            _pool.pop_front();
        }
        _mutex.unlock();
    }

    void signal(void)
    {
        _mutex.lock();
        if (!_waiters.empty())
            SetEvent(_waiters.back());
        _mutex.unlock();
    }

    void broadcast(void)
    {
        _mutex.lock();
        for (std::list<HANDLE>::iterator i = _waiters.begin(); i != _waiters.end(); ++i)
            SetEvent(*i);
        _mutex.unlock();
    }

    bool wait(SGMutex::PrivateData& externalMutex, DWORD msec)
    {
        _mutex.lock();
        if (_pool.empty())
            _waiters.push_front(CreateEvent(NULL, FALSE, FALSE, NULL));
        else
            _waiters.splice(_waiters.begin(), _pool, _pool.begin());
        std::list<HANDLE>::iterator i = _waiters.begin();
        _mutex.unlock();

        externalMutex.unlock();

        DWORD result = WaitForSingleObject(*i, msec);

        externalMutex.lock();

        _mutex.lock();
        if (result != WAIT_OBJECT_0)
            result = WaitForSingleObject(*i, 0);
        _pool.splice(_pool.begin(), _waiters, i);
        _mutex.unlock();

        return result == WAIT_OBJECT_0;
    }

    void wait(SGMutex::PrivateData& externalMutex)
    {
        wait(externalMutex, INFINITE);
    }

    // Protect the list of waiters
    SGMutex::PrivateData _mutex;

    std::list<HANDLE> _waiters;
    std::list<HANDLE> _pool;
};

#else
/////////////////////////////////////////////////////////////////////////////
/// posix threads
/////////////////////////////////////////////////////////////////////////////

#include <pthread.h>
#include <cassert>
#include <cerrno>
#include <sys/time.h>

struct SGThread::PrivateData {
    PrivateData() :
        _started(false)
    {
    }
    ~PrivateData()
    {
        // If we are still having a started thread and nobody waited,
        // now detach ...
        if (!_started)
            return;
        pthread_detach(_thread);
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

        int ret = pthread_create(&_thread, 0, start_routine, &thread);
        if (0 != ret)
            return false;

        _started = true;
        return true;
    }

    void join()
    {
        if (!_started)
            return;

        pthread_join(_thread, 0);
        _started = false;
    }

    pthread_t _thread;
    bool _started;
};

long SGThread::current( void ) {
    return (long)pthread_self();
}

struct SGMutex::PrivateData {
    PrivateData()
    {
        int err = pthread_mutex_init(&_mutex, 0);
        assert(err == 0);
        (void)err;
    }

    ~PrivateData()
    {
        int err = pthread_mutex_destroy(&_mutex);
        assert(err == 0);
        (void)err;
    }

    void lock(void)
    {
        int err = pthread_mutex_lock(&_mutex);
        assert(err == 0);
        (void)err;
    }

    void unlock(void)
    {
        int err = pthread_mutex_unlock(&_mutex);
        assert(err == 0);
        (void)err;
    }

    pthread_mutex_t _mutex;
};

struct SGWaitCondition::PrivateData {
    PrivateData(void)
    {
        int err = pthread_cond_init(&_condition, NULL);
        assert(err == 0);
        (void)err;
    }
    ~PrivateData(void)
    {
        int err = pthread_cond_destroy(&_condition);
        assert(err == 0);
        (void)err;
    }

    void signal(void)
    {
        int err = pthread_cond_signal(&_condition);
        assert(err == 0);
        (void)err;
    }

    void broadcast(void)
    {
        int err = pthread_cond_broadcast(&_condition);
        assert(err == 0);
        (void)err;
    }

    void wait(SGMutex::PrivateData& mutex)
    {
        int err = pthread_cond_wait(&_condition, &mutex._mutex);
        assert(err == 0);
        (void)err;
    }

    bool wait(SGMutex::PrivateData& mutex, unsigned msec)
    {
        struct timespec ts;
#ifdef HAVE_CLOCK_GETTIME
        if (0 != clock_gettime(CLOCK_REALTIME, &ts))
            return false;
#else
        struct timeval tv;
        if (0 != gettimeofday(&tv, NULL))
            return false;
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
#endif

        ts.tv_nsec += 1000000*(msec % 1000);
        if (1000000000 <= ts.tv_nsec) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec += 1;
        }
        ts.tv_sec += msec / 1000;

        int evalue = pthread_cond_timedwait(&_condition, &mutex._mutex, &ts);
        if (evalue == 0)
          return true;

        assert(evalue == ETIMEDOUT);
        return false;
    }

    pthread_cond_t _condition;
};

#endif

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

SGMutex::SGMutex() :
    _privateData(new PrivateData)
{
}

SGMutex::~SGMutex()
{
    delete _privateData;
    _privateData = 0;
}

void
SGMutex::lock()
{
    _privateData->lock();
}

void
SGMutex::unlock()
{
    _privateData->unlock();
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
SGWaitCondition::wait(SGMutex& mutex)
{
    _privateData->wait(*mutex._privateData);
}

bool
SGWaitCondition::wait(SGMutex& mutex, unsigned msec)
{
    return _privateData->wait(*mutex._privateData, msec);
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
