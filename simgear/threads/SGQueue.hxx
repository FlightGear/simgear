#ifndef SGQUEUE_HXX_INCLUDED
#define SGQUEUE_HXX_INCLUDED 1

#include <simgear/compiler.h>

#if defined ( SG_HAVE_STD_INCLUDES )
#  include <cassert>
#else
#  include <assert.h>
#endif

#include <queue>
#include "SGThread.hxx"
#include "SGGuard.hxx"

/**
 * SGQueue defines an interface for a FIFO.
 * It can be implemented using different types of synchronization
 * and protection.
 */
template<class T>
class SGQueue
{
public:

    /**
     * Create a new SGQueue object.
     */
    SGQueue() {}

    /**
     * Destroy this object.
     */
    virtual ~SGQueue() {}

    /**
     * Returns whether this queue is empty (contains no elements).
     *
     * @return bool True if queue is empty, otherwisr false.
     */
    virtual bool empty() = 0;

    /**
     * Add an item to the end of the queue.
     *
     * @param T object to add.
     */
    virtual void push( const T& item ) = 0;

    /**
     * View the item from the head of the queue.
     *
     * @return T next available object.
     */
    virtual T front() = 0;

    /**
     * Get an item from the head of the queue.
     *
     * @return T next available object.
     */
    virtual T pop() = 0;

protected:
    /**
     * 
     */
    std::queue<T> fifo;
};

/**
 * A simple thread safe queue.  All access functions are guarded with a mutex.
 */
template<class T, class SGLOCK=SGMutex>
class SGLockedQueue : public SGQueue<T>
{
public:

    /**
     * Create a new SGLockedQueue object.
     */
    SGLockedQueue() {}

    /**
     * Destroy this object.
     */
    ~SGLockedQueue() {}

    /**
     * Returns whether this queue is empty (contains no elements).
     *
     * @return bool True if queue is empty, otherwisr false.
     */
    virtual bool empty() {
	SGGuard<SGLOCK> g(mutex);
	return fifo.empty();
    }

    /**
     * Add an item to the end of the queue.
     *
     * @param T object to add.
     */
    virtual void push( const T& item ) {
	SGGuard<SGLOCK> g(mutex);
	fifo.push( item );
    }

    /**
     * View the item from the head of the queue.
     *
     * @return T next available object.
     */
    virtual T front() {
	SGGuard<SGLOCK> g(mutex);
	assert( ! fifo.empty() );
	T item = fifo.front();
	return item;
    }

    /**
     * Get an item from the head of the queue.
     *
     * @return T next available object.
     */
    virtual T pop() {
	SGGuard<SGLOCK> g(mutex);
	//if (fifo.empty()) throw NoSuchElementException();
	assert( ! fifo.empty() );
//  	if (fifo.empty())
//  	{
//  	    mutex.unlock();
//  	    pthread_exit( PTHREAD_CANCELED );
//  	}
	T item = fifo.front();
	fifo.pop();
	return item;
    }
private:

    /**
     * Mutex to serialise access.
     */
    SGLOCK mutex;

private:
    // Prevent copying.
    SGLockedQueue(const SGLockedQueue&);
    SGLockedQueue& operator= (const SGLockedQueue&);
};

/**
 * A guarded queue blocks threads trying to retrieve items
 * when none are available.
 */
template<class T>
class SGBlockingQueue : public SGQueue<T>
{
public:
    /**
     * Create a new SGBlockingQueue.
     */
    SGBlockingQueue() {}

    /**
     * Destroy this queue.
     */
    ~SGBlockingQueue() { mutex.unlock(); }

    /**
     * 
     */
    virtual bool empty() {
	SGGuard<SGMutex> g(mutex);
	return fifo.empty();
    }

    /**
     * Add an item to the end of the queue.
     *
     * @param T object to add.
     */
    virtual void push( const T& item ) {
	SGGuard<SGMutex> g(mutex);
	fifo.push( item );
	not_empty.signal();
    }

    /**
     * View the item from the head of the queue.
     * Calling thread is not suspended
     *
     * @return T next available object.
     */
    virtual T front() {
	SGGuard<SGMutex> g(mutex);

	assert(fifo.empty() != true);
	//if (fifo.empty()) throw ??

	T item = fifo.front();
	return item;
    }

    /**
     * Get an item from the head of the queue.
     * If no items are available then the calling thread is suspended
     *
     * @return T next available object.
     */
    virtual T pop() {
	SGGuard<SGMutex> g(mutex);

	while (fifo.empty())
	    not_empty.wait(mutex);

	assert(fifo.empty() != true);
	//if (fifo.empty()) throw ??

	T item = fifo.front();
	fifo.pop();
	return item;
    }

private:

    /**
     * Mutex to serialise access.
     */
    SGMutex mutex;

    /**
     * Condition to signal when queue not empty.
     */
    SGPthreadCond not_empty;

private:
    // Prevent copying.
    SGBlockingQueue( const SGBlockingQueue& );
    SGBlockingQueue& operator=( const SGBlockingQueue& );
};

#endif // SGQUEUE_HXX_INCLUDED
