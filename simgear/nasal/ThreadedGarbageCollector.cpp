//#include "nasal.h"
//#include "data.h"
//#include "code.h"

#include <simgear/timing/timestamp.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/debug/logstream.hxx>
#include <mutex>
#include <condition_variable>
#include <atomic>
extern "C" {
    extern int __bg_gc;
    extern int GCglobalAlloc();
    extern int naGarbageCollect();
}

class SGExclusiveThread : public SGThread
{
private:
    std::mutex mutex_;
    std::condition_variable condVar;
    SGTimeStamp timestamp;
    std::mutex Cmutex_;
    std::condition_variable CcondVar;

    bool _started;
    bool _terminated;
    int last_await_time;

    std::atomic<bool> dataReady;
    std::atomic<bool> complete;
    std::atomic<bool> process_ran;
    std::atomic<bool> process_running;

public:
    SGExclusiveThread() : 
        _started(false), _terminated(false), last_await_time(0), 
        dataReady(false), complete(true), process_ran(false), process_running(false)
    {
    }

    virtual ~SGExclusiveThread()
    {

    }

    void release() {
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
    void wait() {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!dataReady)
        {
            do
            {
                condVar.wait(lck);
            } while (!dataReady);
        }
    }
    void clearAwaitCompletionTime() {
        last_await_time = 0;
    }
    virtual void awaitCompletion() {
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
            printf("await %5.1f ", last_await_time / 1000.0);
            process_ran = 0;
        }
    }

    void setCompletion() {
        std::unique_lock<std::mutex> lck(Cmutex_);
        if (!dataReady.exchange(false))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  atomic operation on dataReady failed (5)\n");

        if (complete.exchange(true))
            SG_LOG(SG_NASAL, SG_ALERT, "[SGExclusiveThread]  atomic operation on complete failed (5)\n");
        CcondVar.notify_one();
    }
    virtual int process() = 0;
    virtual void run()
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

    void terminate() {
        _terminated = true;
    }
    bool stop()
    {
        return true;
    }
    void ensure_running()
    {
        if (!_started)
        {
            _started = true;
            start();
        }
    }
    bool is_running()
    {
        return process_running;
    }

};

class ThreadedGarbageCollector : public SGExclusiveThread
{
public:
    ThreadedGarbageCollector() : SGExclusiveThread()
    {
    }
    virtual ~ThreadedGarbageCollector()
    {

    }

    virtual int process()
    {
        return naGarbageCollect();
    }
};

ThreadedGarbageCollector gct;
extern"C" {
    void startNasalBackgroundGarbageCollection()
    {
        gct.ensure_running();
    }
    void stopNasalBackgroundGarbageCollection()
    {
        gct.terminate();
    }
    void performNasalBackgroundGarbageCollection()
    {
        if (gct.is_running())
            gct.release();
    }
    void awaitNasalGarbageCollectionComplete(bool can_wait)
    {
        if (gct.is_running())
        {
            if (can_wait)
                gct.awaitCompletion();
            else
                gct.clearAwaitCompletionTime();
        }
    }
}
