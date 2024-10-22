// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2001 Bernard Bright <bbright@bigpond.net.au>
// SPDX-FileCopyrightText: 2011 Mathias Froehlichi
// SPDX-FileCopyrightText: 2020 Erik Hofman

/**
 * @file
 * @brief Simple pthread class wrappers
 */

#ifndef SGTHREAD_HXX_INCLUDED
#define SGTHREAD_HXX_INCLUDED 1

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <simgear/compiler.h>
#include <simgear/timing/timestamp.hxx>


// backwards compatibility, just needs a recompile
#define SGMutex		std::mutex


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

    /**
     * General thread starter routine.
     */
    static void *start_routine(void* data);

private:
    // Disable copying.
    SGThread(const SGThread&);
    SGThread& operator=(const SGThread&);

    std::thread _thread;
    bool _started = false;
};


/**
 * A condition variable is a synchronization device that allows threads to
 * suspend execution until some predicate on shared data is satisfied.
 * A condition variable is always associated with a mutex to avoid race
 * conditions.
 */
class SGWaitCondition final {
public:
    /**
     * Create a new condition variable.
     */
    SGWaitCondition();

    /**
     * Destroy the condition object.
     */
    ~SGWaitCondition();     // non-virtual intentional

    /**
     * Wait for this condition variable to be signaled.
     *
     * @param mutex Reference to a locked mutex.
     */
    void wait(std::mutex& mutex);

    /**
     * Wait for this condition variable to be signaled for at most \a 'msec'
     * milliseconds.
     *
     * @param mutex Reference to a locked mutex.
     * @param msec  Milliseconds to wait for a signal.
     *
     * @return
     */
    bool wait(std::mutex& mutex, unsigned msec);

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

    bool ready = false;
    std::mutex _mtx;
    std::condition_variable _condition;
};

///
/// an exclusive thread is one that is designed for frame processing;
/// it has the ability to synchronise such that the caller can await
/// the processing to finish.
class SGExclusiveThread : public SGThread{
private:
    std::mutex mutex_;
    std::condition_variable condVar;
    SGTimeStamp timestamp;
    std::mutex Cmutex_;
    std::condition_variable CcondVar;

    bool _started;
    bool _terminated;
    int last_await_time;

    std::atomic<int> dataReady;
    std::atomic<int> complete;
    std::atomic<int> process_ran;
    std::atomic<int> process_running;

public:
    SGExclusiveThread();
    virtual ~SGExclusiveThread();
    void release();
    void wait();
    void clearAwaitCompletionTime();
    virtual void awaitCompletion();
    void setCompletion();
    virtual int process() = 0;
    virtual void run();
    void terminate();
    bool stop();
    void ensure_running();
    bool is_running();
};

#endif /* SGTHREAD_HXX_INCLUDED */
