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

#ifndef SGTHREAD_HXX_INCLUDED
#define SGTHREAD_HXX_INCLUDED 1

#include <simgear/compiler.h>

/**
 * Encapsulate generic threading methods.
 * Users derive a class from SGThread and implement the run() member function.
 */
class SGThread {
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
    bool start();

    /**
     * Suspends the exection of the calling thread until this thread
     * terminates.
     */
    void join();

    /**
     *Retreive the current thread id.
     */
    static long current( void );

protected:
    /**
     * Destroy a thread object.
     * This is protected so that its illegal to simply delete a thread
     * - it must return from its run() function.
     */
    virtual ~SGThread();

    /**
     * All threads execute by deriving the run() method of SGThread.
     * If this function terminates then the thread also terminates.
     */
    virtual void run() = 0;

private:
    // Disable copying.
    SGThread(const SGThread&);
    SGThread& operator=(const SGThread&);

    struct PrivateData;
    PrivateData* _privateData;

    friend struct PrivateData;
};

class SGWaitCondition;

/**
 * A mutex is used to protect a section of code such that at any time
 * only a single thread can execute the code.
 */
class SGMutex {
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
     */
    void lock();

    /**
     * Unlock this mutex.
     * It is assumed that the mutex is locked and owned by the calling thread.
     */
    void unlock();

private:
    struct PrivateData;
    PrivateData* _privateData;

    friend class SGWaitCondition;
};

/**
 * A condition variable is a synchronization device that allows threads to
 * suspend execution until some predicate on shared data is satisfied.
 * A condition variable is always associated with a mutex to avoid race
 * conditions.
 */
class SGWaitCondition {
public:
    /**
     * Create a new condition variable.
     */
    SGWaitCondition();

    /**
     * Destroy the condition object.
     */
    ~SGWaitCondition();

    /**
     * Wait for this condition variable to be signaled.
     *
     * @param mutex Reference to a locked mutex.
     */
    void wait(SGMutex& mutex);

    /**
     * Wait for this condition variable to be signaled for at most \a 'msec'
     * milliseconds.
     *
     * @param mutex Reference to a locked mutex.
     * @param msec  Milliseconds to wait for a signal.
     *
     * @return
     */
    bool wait(SGMutex& mutex, unsigned msec);

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
    SGWaitCondition(const SGWaitCondition&);
    SGWaitCondition& operator=(const SGWaitCondition&);

    struct PrivateData;
    PrivateData* _privateData;
};

#endif /* SGTHREAD_HXX_INCLUDED */
