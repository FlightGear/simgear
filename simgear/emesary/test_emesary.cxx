////////////////////////////////////////////////////////////////////////
// Test harness for Emesary.
////////////////////////////////////////////////////////////////////////

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>

#include <simgear/threads/SGThread.hxx>
#include <simgear/emesary/Emesary.hxx>

using std::cout;
using std::cerr;
using std::endl;

std::atomic<int> nthread {0};
std::atomic<int> noperations {0};
const int MaxIterations = 9999;

class TestThreadNotification : public simgear::Emesary::INotification
{
protected:
    const char *baseValue;
public:
    TestThreadNotification(const char *v) : baseValue(v) {}

    virtual const char* GetType () { return baseValue; }
};

class TestThreadRecipient : public simgear::Emesary::IReceiver
{
public:
    TestThreadRecipient() : receiveCount(0)
    {
    }

    std::atomic<int> receiveCount;

    virtual simgear::Emesary::ReceiptStatus Receive(simgear::Emesary::INotification &n)
    {
        if (n.GetType() == (const char*)this)
        {
            // Unused: TestThreadNotification *tn = dynamic_cast<TestThreadNotification *>(&n);
            receiveCount++;

            TestThreadNotification onwardNotification("AL");
            simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(onwardNotification);
            
            return simgear::Emesary::ReceiptStatusOK;
        }

        return simgear::Emesary::ReceiptStatusOK;
    }
};

class EmesaryTestThread : public SGThread
{
protected:
    virtual void run() {
        int threadId = nthread.fetch_add(1);

        //System.Threading.Interlocked.Increment(ref nthread);
        //var rng = new Random();
        TestThreadRecipient r;
        char temp[100];
        sprintf(temp, "Notif %d", threadId);
        printf("starting thread %s\n", temp);
        TestThreadNotification tn((const char*)&r);
        for (int i = 0; i < MaxIterations; i++)
        {
            simgear::Emesary::GlobalTransmitter::instance()->Register(r);
            simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(tn);
            simgear::Emesary::GlobalTransmitter::instance()->DeRegister(r);
            //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
            noperations++;
        }
        printf("%s invocations %d\n", temp, (int)r.receiveCount);
        printf("finish thread %s\n", temp);
    }
};

class EmesaryTest
{
public:

    void Emesary_MultiThreadTransmitterTest()
    {
        int num_threads = 12;
        std::list<EmesaryTestThread*> threads;

        for (int i = 0; i < num_threads; i++)
        {
            EmesaryTestThread *thread = new EmesaryTestThread();
            threads.push_back(thread);
            thread->start();
        }
        for (std::list<EmesaryTestThread*>::iterator i = threads.begin(); i != threads.end(); i++)
        {
            (*i)->join();
        }
    }
};

void testEmesaryThreaded()
{
    TestThreadRecipient r;
    TestThreadNotification tn((const char*)&r);
    simgear::Emesary::GlobalTransmitter::instance()->Register(r);
    for (int i = 0; i < MaxIterations*MaxIterations; i++)
    {
        simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(tn);
        //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
        noperations++;
    }
    simgear::Emesary::GlobalTransmitter::instance()->DeRegister(r);
    printf("invocations %d\n", simgear::Emesary::GlobalTransmitter::instance()->SentMessageCount());

    EmesaryTest t;
    t.Emesary_MultiThreadTransmitterTest();
}


int main(int ac, char ** av)
{
    testEmesaryThreaded();

    std::cout << "all tests passed" << std::endl;
    return 0;
}
