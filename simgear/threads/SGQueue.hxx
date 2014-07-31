#ifndef SGQUEUE_HXX_INCLUDED
#define SGQUEUE_HXX_INCLUDED 1

#include <simgear/compiler.h>

#include <cassert>
#include <queue>
#include "SGGuard.hxx"
#include "SGThread.hxx"

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
     * @param item  object to add.
     */
    virtual void push( const T& item ) = 0;

    /**
     * View the item from the head of the queue.
     *
     * @return The next available object.
     */
    virtual T front() = 0;

    /**
     * Get an item from the head of the queue.
     *
     * @return The next available object.
     */
    virtual T pop() = 0;

    /**
     * Query the size of the queue
     *
     * @return size_t size of queue.
     */
    virtual size_t size() = 0;

protected:
    /**
     *
     */
    std::queue<T> fifo;
};

/**
 * A simple thread safe queue.  All access functions are guarded with a mutex.
 */
template<class T>
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
    virtual ~SGLockedQueue() {}

    /**
     * Returns whether this queue is empty (contains no elements).
     *
     * @return True if queue is empty, otherwise false.
     */
    virtual bool empty() {
	SGGuard<SGMutex> g(mutex);
	return this->fifo.empty();
    }

    /**
     * Add an item to the end of the queue.
     *
     * @param item object to add.
     */
    virtual void push( const T& item ) {
	SGGuard<SGMutex> g(mutex);
	this->fifo.push( item );
    }

    /**
     * View the item from the head of the queue.
     *
     * @return The next available object.
     */
    virtual T front() {
	SGGuard<SGMutex> g(mutex);
	assert( ! this->fifo.empty() );
	T item = this->fifo.front();
	return item;
    }

    /**
     * Get an item from the head of the queue.
     *
     * @return The next available object.
     */
    virtual T pop() {
	SGGuard<SGMutex> g(mutex);
	    if (this->fifo.empty()) return T(); // assumes T is default constructable
        
//  	if (fifo.empty())
//  	{
//  	    mutex.unlock();
//  	    pthread_exit( PTHREAD_CANCELED );
//  	}
	T item = this->fifo.front();
	this->fifo.pop();
	return item;
    }

    /**
     * Query the size of the queue
     *
     * @return Size of queue.
     */
    virtual size_t size() {
	SGGuard<SGMutex> g(mutex);
        return this->fifo.size();
    }

private:

    /**
     * Mutex to serialise access.
     */
    SGMutex mutex;

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
    virtual ~SGBlockingQueue() {}

    /**
     *
     */
    virtual bool empty() {
	SGGuard<SGMutex> g(mutex);
	return this->fifo.empty();
    }

    /**
     * Add an item to the end of the queue.
     *
     * @param item The object to add.
     */
    virtual void push( const T& item ) {
	SGGuard<SGMutex> g(mutex);
	this->fifo.push( item );
	not_empty.signal();
    }

    /**
     * View the item from the head of the queue.
     * Calling thread is not suspended
     *
     * @return The next available object.
     */
    virtual T front() {
	SGGuard<SGMutex> g(mutex);

	assert(this->fifo.empty() != true);
	//if (fifo.empty()) throw ??

	T item = this->fifo.front();
	return item;
    }

    /**
     * Get an item from the head of the queue.
     * If no items are available then the calling thread is suspended
     *
     * @return The next available object.
     */
    virtual T pop() {
	SGGuard<SGMutex> g(mutex);

	while (this->fifo.empty())
	    not_empty.wait(mutex);

	assert(this->fifo.empty() != true);
	//if (fifo.empty()) throw ??

	T item = this->fifo.front();
	this->fifo.pop();
	return item;
    }

    /**
     * Query the size of the queue
     *
     * @return Size of queue.
     */
    virtual size_t size() {
	SGGuard<SGMutex> g(mutex);
        return this->fifo.size();
    }

private:

    /**
     * Mutex to serialise access.
     */
    SGMutex mutex;

    /**
     * Condition to signal when queue not empty.
     */
    SGWaitCondition not_empty;

private:
    // Prevent copying.
    SGBlockingQueue( const SGBlockingQueue& );
    SGBlockingQueue& operator=( const SGBlockingQueue& );
};


/**
 * A guarded deque blocks threads trying to retrieve items
 * when none are available.
 */
template<class T>
class SGBlockingDeque
{
public:
    /**
     * Create a new SGBlockingDequeue.
     */
    SGBlockingDeque() {}

    /**
     * Destroy this dequeue.
     */
    virtual ~SGBlockingDeque() {}

    /**
     *
     */
    virtual void clear() {
    SGGuard<SGMutex> g(mutex);
    this->queue.clear();
    }

    /**
     *
     */
    virtual bool empty() {
    SGGuard<SGMutex> g(mutex);
    return this->queue.empty();
    }

    /**
     * Add an item to the front of the queue.
     *
     * @param item The object to add.
     */
    virtual void push_front( const T& item ) {
    SGGuard<SGMutex> g(mutex);
    this->queue.push_front( item );
    not_empty.signal();
    }

    /**
     * Add an item to the back of the queue.
     *
     * @param item The object to add.
     */
    virtual void push_back( const T& item ) {
    SGGuard<SGMutex> g(mutex);
    this->queue.push_back( item );
    not_empty.signal();
    }

    /**
     * View the item from the head of the queue.
     * Calling thread is not suspended
     *
     * @return The next available object.
     */
    virtual T front() {
    SGGuard<SGMutex> g(mutex);

    assert(this->queue.empty() != true);
    //if (queue.empty()) throw ??

    T item = this->queue.front();
    return item;
    }

    /**
     * Get an item from the head of the queue.
     * If no items are available then the calling thread is suspended
     *
     * @return The next available object.
     */
    virtual T pop_front() {
    SGGuard<SGMutex> g(mutex);

    while (this->queue.empty())
        not_empty.wait(mutex);

    assert(this->queue.empty() != true);
    //if (queue.empty()) throw ??

    T item = this->queue.front();
    this->queue.pop_front();
    return item;
    }

    /**
     * Get an item from the tail of the queue.
     * If no items are available then the calling thread is suspended
     *
     * @return The next available object.
     */
    virtual T pop_back() {
    SGGuard<SGMutex> g(mutex);

    while (this->queue.empty())
        not_empty.wait(mutex);

    assert(this->queue.empty() != true);
    //if (queue.empty()) throw ??

    T item = this->queue.back();
    this->queue.pop_back();
    return item;
    }

    /**
     * Query the size of the queue
     *
     * @return Size of queue.
     */
    virtual size_t size() {
    SGGuard<SGMutex> g(mutex);
        return this->queue.size();
    }

    void waitOnNotEmpty() {
    	SGGuard<SGMutex> g(mutex);
    	while (this->queue.empty())
    	    not_empty.wait(mutex);
    }
private:

    /**
     * Mutex to serialise access.
     */
    SGMutex mutex;

    /**
     * Condition to signal when queue not empty.
     */
    SGWaitCondition not_empty;

private:
    // Prevent copying.
    SGBlockingDeque( const SGBlockingDeque& );
    SGBlockingDeque& operator=( const SGBlockingDeque& );

protected:
    std::deque<T> queue;
};

#endif // SGQUEUE_HXX_INCLUDED
