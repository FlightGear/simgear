#include <simgear/compiler.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#  include <time.h>
#else
#  if defined ( sgi ) && !defined( __GNUC__ )
     // This works around a bug triggered when using MipsPro 7.4.1
     // and (at least) IRIX 6.5.20
#    include <iostream>
#  endif
#  include <sys/time.h>
#endif
#if _MSC_VER >= 1300
#  include "winsock2.h"
#endif

#include "SGThread.hxx"

void*
start_handler( void* arg )
{
    SGThread* thr = static_cast<SGThread*>(arg);
    thr->run();
    return 0;
}

void
SGThread::set_cancel( cancel_t mode )
{
    switch (mode)
    {
    case CANCEL_DISABLE:
	pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, 0 );
	break;
    case CANCEL_DEFERRED:
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, 0 );
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, 0 );
	break;
    case CANCEL_IMMEDIATE:
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, 0 );
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, 0 );
	break;
    default:
	break;
    }
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

#if defined(_MSC_VER) || defined(__MINGW32__)
int gettimeofday(struct timeval* tp, void* tzp) {
    LARGE_INTEGER t;

    if(QueryPerformanceCounter(&t)) {
        /* hardware supports a performance counter */
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        tp->tv_sec = t.QuadPart/f.QuadPart;
        tp->tv_usec = ((float)t.QuadPart/f.QuadPart*1000*1000)
            - (tp->tv_sec*1000*1000);
    } else {
        /* hardware doesn't support a performance counter, so get the
           time in a more traditional way. */
        DWORD t;
        t = timeGetTime();
        tp->tv_sec = t / 1000;
        tp->tv_usec = t % 1000;
    }

    /* 0 indicates that the call succeeded. */
    return 0;
}
#endif

bool
SGPthreadCond::wait( SGMutex& mutex, unsigned long ms )
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

