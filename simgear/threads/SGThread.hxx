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

#include <simgear/compiler.h>

#include <pthread.h>
#if defined ( SG_HAVE_STD_INCLUDES )
#  include <cassert>
#  include <cerrno>
#else
#  include <assert.h>
#  include <sys/errno.h>
#endif

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
     * SGThread cancelation modes.
     */
    enum cancel_t
    {
	CANCEL_DISABLE = 0,
	CANCEL_DEFERRED,
	CANCEL_IMMEDIATE
    };
public:

    /**
     * Create a new thread object.
     * When a SGThread object is created it does not begin execution
     * immediately.  It is started by calling the start() member function.
     */
    SGThread();

    /**
     * Start the underlying thread of execution.
     * @return Pthread error code if execution fails, otherwise returns 0.
     */
    int start();

    /**
     * Sends a cancellation request to the underlying thread.  The target
     * thread will either ignore the request, honor it immediately or defer
     * it until it reaches a cancellation point.
     */
    void cancel();

    /**
     * Suspends the exection of the calling thread until this thread
     * terminates.
     */
    void join();

protected:
    /**
     * Destroy a thread object.
     * This is protected so that its illegal to simply delete a thread
     * - it must return from its run() function.
     */
    virtual ~SGThread();

    /**
     * Set the threads cancellation mode.
     * @param mode The required cancellation mode.
     */
    void set_cancel( cancel_t mode );

    /**
     * All threads execute by deriving the run() method of SGThread.
     * If this function terminates then the thread also terminates.
     */
    virtual void run() = 0;

private:

    /**
     * Pthread thread identifier.
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
    friend class SGPthreadCond;

public:

    /**
     * Create a new mutex.
     * Under Linux this is a 'fast' mutex.
     */
    SGMutex();

    /**
     * Destroy a mutex object.
     * Note: it is the responsibility of the caller to ensure the mutex is
     * unlocked before destruction occurs.
     */
    ~SGMutex();

    /**
     * Lock this mutex.
     * If the mutex is currently unlocked, it becomes locked and owned by
     * the calling thread.  If the mutex is already locked by another thread,
     * the calling thread is suspended until the mutex is unlocked.  If the
     * mutex is already locked and owned by the calling thread, the calling
     * thread is suspended until the mutex is unlocked, effectively causing
     * the calling thread to deadlock.
     *
     * @see SGMutex::trylock
     */
    void lock();

    /**
     * Try to lock the mutex for the current thread.  Behaves like lock except
     * that it doesn't block the calling thread.
     * @return true if mutex was successfully locked, otherwise false.
     * @see SGMutex::lock
     */
    bool trylock();

    /**
     * Unlock this mutex.
     * It is assumed that the mutex is locked and owned by the calling thread.
     */
    void unlock();

protected:

    /**
     * Pthread mutex.
     */
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
 * A condition variable is a synchronization device that allows threads to 
 * suspend execution until some predicate on shared data is satisfied.
 * A condition variable is always associated with a mutex to avoid race
 * conditions. 
 */
class SGPthreadCond
{
public:
    /**
     * Create a new condition variable.
     */
    SGPthreadCond();

    /**
     * Destroy the condition object.
     */
    ~SGPthreadCond();

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
     * @param mutex reference to a locked mutex.
     * @param ms milliseconds to wait for a signal.
     *
     * @return 
     */
    bool wait( SGMutex& mutex, unsigned long ms );

    /**
     * Wake one thread waiting on this condition variable.
     * Nothing happens if no threads are waiting.
     * If several threads are waiting exactly one thread is restarted.  It
     * is not specified which.
     */
    void signal();

    /**
     * Wake all threads waiting on this condition variable.
     * Nothing happens if no threads are waiting.
     */
    void broadcast();

private:
    // Disable copying.
    SGPthreadCond(const SGPthreadCond& );
    SGPthreadCond& operator=(const SGPthreadCond& );

private:

    /**
     * The Pthread conditon variable.
     */
    pthread_cond_t cond;
};

inline SGPthreadCond::SGPthreadCond()
{
    int status = pthread_cond_init( &cond, 0 );
    assert( status == 0 );
}

inline SGPthreadCond::~SGPthreadCond()
{
    int status = pthread_cond_destroy( &cond );
    assert( status == 0 );
}

inline void SGPthreadCond::signal()
{
    int status = pthread_cond_signal( &cond );
    assert( status == 0 );
}

inline void SGPthreadCond::broadcast()
{
    int status = pthread_cond_broadcast( &cond );
    assert( status == 0 );
}

inline void SGPthreadCond::wait( SGMutex& mutex )
{
    int status = pthread_cond_wait( &cond, &mutex.mutex );
    assert( status == 0 );
}

#endif /* SGTHREAD_HXX_INCLUDED */
