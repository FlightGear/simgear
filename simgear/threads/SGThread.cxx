#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <sys/time.h>

#include "SGThread.hxx"

void*
start_handler( void* arg )
{
    SGThread* thr = static_cast<SGThread*>(arg);
    thr->run();
    return 0;
}

bool
SGMutex::trylock()
{
    int status = pthread_mutex_lock( &mutex );
    if (status == EBUSY)
    {
	return false;
    }
    assert( status == 0 );
    return true;
}

bool
SGCondition::wait( SGMutex& mutex, unsigned long ms )
{
    struct timeval now;
    ::gettimeofday( &now, 0 );

    // Wait time is now + ms milliseconds
    unsigned int sec = ms / 1000;
    unsigned int nsec = (ms % 1000) * 1000;
    struct timespec abstime;
    abstime.tv_sec = now.tv_sec + sec;
    abstime.tv_nsec = now.tv_usec*1000 + nsec;

    int status = pthread_cond_timedwait( &cond, &mutex.mutex, &abstime );
    if (status == ETIMEDOUT)
    {
	return false;
    }

    assert( status == 0 );
    return true;
}

