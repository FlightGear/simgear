// SGThread - Simple pthread class wrappers.
//
// Written by Bernie Bright, started April 2001.
//
// Copyright (C) 2001  Bernard Bright - bbright@bigpond.net.au
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifndef SGTHREAD_HXX_INCLUDED
#define SGTHREAD_HXX_INCLUDED 1

#include <pthread.h>
#include <cassert>
#include <cerrno>

class SGThread;

extern "C" {
    void* start_handler( void* );
};

/**
 * Encapsulate generic threading methods.
 * Users derive a class from SGThread and implement the run() member function.
 */
class SGThread
{
public:

    /**
     * 
     */
    SGThread();

    /**
     * 
     */
    virtual ~SGThread();

    /**
     * 
     */
    int start();

    /**
     * 
     */
    void cancel();

    /**
     * 
     */
    void join();

protected:

    /**
     * 
     */
    virtual void run() = 0;

private:

    /**
     * 
     */
    pthread_t tid;

    friend void* start_handler( void* );

private:
    // Disable copying.
    SGThread( const SGThread& );
    SGThread& operator=( const SGThread& );
};

inline
SGThread::SGThread()
{
}

inline
SGThread::~SGThread()
{
}

inline int
SGThread::start()
{
    int status = pthread_create( &tid, 0, start_handler, this );
    assert( status == 0 );
    return status;
}

inline void
SGThread::join()
{
    int status = pthread_join( tid, 0 );
    assert( status == 0 );
}

inline void
SGThread::cancel()
{
    int status = pthread_cancel( tid );
    assert( status == 0 );
}

/**
 * A mutex is used to protect a section of code such that at any time
 * only a single thread can execute the code.
 */
class SGMutex
{
    friend class SGCondition;

public:

    /**
     * Create a new mutex.
     */
    SGMutex();

    /**
     * Destroy a mutex object.
     */
    ~SGMutex();

    /**
     * 
     */
    void lock();

    /**
     * 
     */
    bool trylock();

    /**
     * 
     */
    void unlock();

protected:
    pthread_mutex_t mutex;
};

inline SGMutex::SGMutex()
{
    int status = pthread_mutex_init( &mutex, 0 );
    assert( status == 0 );
}

inline SGMutex::~SGMutex()
{
    int status = pthread_mutex_destroy( &mutex );
    assert( status == 0 );
}

inline void SGMutex::lock()
{
    int status = pthread_mutex_lock( &mutex );
    assert( status == 0 );
}

inline void SGMutex::unlock()
{
    int status = pthread_mutex_unlock( &mutex );
    assert( status == 0 );
}

/**
 * 
 */
class SGCondition
{
public:
    /**
     * Create a new condition variable.
     */
    SGCondition();

    /**
     * Destroy the condition object.
     */
    ~SGCondition();

    /**
     * Wait for this condition variable to be signaled.
     *
     * @param SGMutex& reference to a locked mutex.
     */
    void wait( SGMutex& );

    /**
     * Wait for this condition variable to be signaled for at most
     * 'ms' milliseconds.
     *
     * @param SGMutex& reference to a locked mutex.
     * @param unsigned long milliseconds to wait for a signal.
     *
     * @return 
     */
    bool wait( SGMutex& mutex, unsigned long ms );

    /**
     * Wake one thread waiting on this condition variable.
     */
    void signal();

    /**
     * Wake all threads waiting on this condition variable.
     */
    void broadcast();

private:
    // Disable copying.
    SGCondition(const SGCondition& );
    SGCondition& operator=(const SGCondition& );

private:

    pthread_cond_t cond;
};

inline SGCondition::SGCondition()
{
    int status = pthread_cond_init( &cond, 0 );
    assert( status == 0 );
}

inline SGCondition::~SGCondition()
{
    int status = pthread_cond_destroy( &cond );
    assert( status == 0 );
}

inline void SGCondition::signal()
{
    int status = pthread_cond_signal( &cond );
    assert( status == 0 );
}

inline void SGCondition::broadcast()
{
    int status = pthread_cond_broadcast( &cond );
    assert( status == 0 );
}

inline void SGCondition::wait( SGMutex& mutex )
{
    int status = pthread_cond_wait( &cond, &mutex.mutex );
    assert( status == 0 );
}

#endif /* SGTHREAD_HXX_INCLUDED */
