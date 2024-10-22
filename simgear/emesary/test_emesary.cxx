// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file
 * @brief Test harness for Emesary.
 */

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>

#include <simgear/threads/SGThread.hxx>
#include <simgear/emesary/Emesary.hxx>
#include <list>
#include <simgear/misc/test_macros.hxx>

using std::cout;
using std::cerr;
using std::endl;

std::atomic<int> nthread {0};
std::atomic<int> noperations {0};
const int MaxIterationsBase = 9999999;
const int MaxIterationsThreaded = 90;
const int MaxIterationsBaseMultipleRecipients = 9999999;
const int num_threads = 220;


void summary(const SGTimeStamp& timeStamp, simgear::Emesary::Transmitter* transmitter, const char* id) {
    printf("[%s]: invocations %zu\n", id, transmitter->SentMessageCount());
    double elapsed_seconds = timeStamp.elapsedMSec() / 1000.0f;
    printf("[%s]:  -> elapsed %d\n", id, timeStamp.elapsedMSec());
    printf("[%s]: took %lf seconds which is %lf/sec\n", id, elapsed_seconds, transmitter->SentMessageCount() / elapsed_seconds);
}

class TestThreadNotification : public simgear::Emesary::INotification
{
protected:
    const char *baseValue;
public:
    TestThreadNotification(const char *v) : baseValue(v) {}

    virtual const char* GetType () { return baseValue; }
};

class TestThreadBaseRecipient : public simgear::Emesary::IReceiver
{
public:
    virtual simgear::Emesary::ReceiptStatus Receive(simgear::Emesary::INotificationPtr n)
    {
       return simgear::Emesary::ReceiptStatus::NotProcessed;
    }
};

class TestThreadRecipient : public simgear::Emesary::IReceiver
{
    simgear::Emesary::ITransmitter* transmitter;
public:
    TestThreadRecipient(simgear::Emesary::ITransmitter* _transmitter, bool addDuringReceive) 
        : transmitter(_transmitter), ourType("TestThread"), addDuringReceive(addDuringReceive), receiveCount(0)
    {
        r1 = new TestThreadBaseRecipient();
    }
    simgear::Emesary::IReceiver *r1;
    std::string ourType;
    bool addDuringReceive;
    std::atomic<int> receiveCount;

    virtual simgear::Emesary::ReceiptStatus Receive(simgear::Emesary::INotificationPtr n)
    {
        if (ourType == n->GetType())
        {
//            SGSharedPtr<TestThreadBaseRecipient> r1 = new TestThreadBaseRecipient();

            // Unused: TestThreadNotification *tn = dynamic_cast<TestThreadNotification *>(&n);
            receiveCount++;

            SGSharedPtr<TestThreadNotification> onwardNotification(new TestThreadNotification("AL"));
            transmitter->NotifyAll(onwardNotification);
            if (addDuringReceive) {
                transmitter->Register(r1);
                transmitter->NotifyAll(onwardNotification);
                transmitter->DeRegister(r1);
            }
            return simgear::Emesary::ReceiptStatus::OK;
        }
        return simgear::Emesary::ReceiptStatus::OK;
    }
};

class EmesaryTestThread : public SGThread
{
public:
    EmesaryTestThread(simgear::Emesary::ITransmitter* transmitter, bool _addDuringReceive)
        : transmitter(transmitter), addDuringReceive(_addDuringReceive) {
    }
protected:
    simgear::Emesary::ITransmitter* transmitter;
    bool addDuringReceive;
    virtual void run() {
        int threadId = nthread.fetch_add(1);

        //System.Threading.Interlocked.Increment(ref nthread);
        //var rng = new Random();
        TestThreadRecipient  *r = new TestThreadRecipient(transmitter, addDuringReceive);
        char temp[100];
        snprintf(temp, sizeof(temp), "Notif %d", threadId);
        printf("starting thread %s\n", temp);
        SGSharedPtr<TestThreadNotification> tn(new TestThreadNotification("TestThread"));

        for (int i = 0; i < MaxIterationsThreaded; i++)
        {
            transmitter->Register(r);
            transmitter->NotifyAll(tn);
            transmitter->DeRegister(r);
            //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
            noperations++;
        }
        printf("%s invocations %d\n", temp, (int)r->receiveCount);
        printf("finish thread %s\n", temp);
    }
};

class EmesaryTest
{
public:

    void Emesary_MultiThreadTransmitterTest(simgear::Emesary::ITransmitter *transmitter, bool addDuringReceive)
    {
        std::list<EmesaryTestThread*> threads;

        for (int i = 0; i < num_threads; i++)
        {
            EmesaryTestThread *thread = new EmesaryTestThread(transmitter, addDuringReceive);
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
    printf("Testing multithreaded operations\n");

    simgear::Emesary::Transmitter* globalTransmitter = simgear::Emesary::GlobalTransmitter::instance();

    TestThreadRecipient *r = new TestThreadRecipient(globalTransmitter, false);
    SGSharedPtr < TestThreadNotification> tn (new TestThreadNotification("TestThread"));
    globalTransmitter->Register(r);
    SGTimeStamp timeStamp;
    timeStamp.stamp();
    printf(" -- simple receive\n");
    for (int i = 0; i < MaxIterationsThreaded; i++)
    {
        globalTransmitter->NotifyAll(tn);
        //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
        noperations++;
    }

    globalTransmitter->DeRegister(r);
    printf("invocations %zu\n", globalTransmitter->SentMessageCount());
    double elapsed_seconds = timeStamp.elapsedMSec() / 1000.0f;
    printf(" -> elapsed %d\n", timeStamp.elapsedMSec());
    printf("took %lf seconds which is %lf/sec\n", elapsed_seconds , globalTransmitter->SentMessageCount() / elapsed_seconds);

    EmesaryTest t;
    t.Emesary_MultiThreadTransmitterTest(globalTransmitter, false);

    elapsed_seconds = timeStamp.elapsedMSec() / 1000.0f;
    printf(" -> elapsed %d\n", timeStamp.elapsedMSec());
    printf("took %lf seconds which is %lf/sec\n", elapsed_seconds, globalTransmitter->SentMessageCount() / elapsed_seconds);
}

void testEmesaryThreadedAddDuringReceive()
{
    SGTimeStamp timeStamp;
    timeStamp.stamp();
    simgear::Emesary::Transmitter* globalTransmitter = simgear::Emesary::GlobalTransmitter::instance();
    
    EmesaryTest t;
    t.Emesary_MultiThreadTransmitterTest(globalTransmitter, true);

    summary(timeStamp, globalTransmitter, "ThreadedAddReceive");
}
//////////////////////
/// basic tests

class TestBaseNotification : public simgear::Emesary::INotification
{
public:
    TestBaseNotification() : index(0) {}
    int index;
    virtual const char* GetType() { return "Test"; }
};

static size_t TestDerefNotification_destructor_count = 0;
class TestDerefNotification : public simgear::Emesary::INotification
{
public:
    TestDerefNotification() {}
    ~TestDerefNotification() override {
        TestDerefNotification_destructor_count ++;
    }

   virtual const char* GetType() { return "TestDerefNotification"; }
};

class TestBaseRecipient : public simgear::Emesary::IReceiver
{
public:
    TestBaseRecipient() : receiveCount(0)
    {
    }

    std::atomic<int> receiveCount;

    virtual simgear::Emesary::ReceiptStatus Receive(simgear::Emesary::INotificationPtr n)
    {
        if (std::string{n->GetType()} == std::string{"Test"})
        {
            auto tbn = dynamic_pointer_cast<TestBaseNotification>(n);
            SG_CHECK_EQUAL(receiveCount, tbn->index);
            receiveCount++;
            return simgear::Emesary::ReceiptStatus::OK;
        }

        return simgear::Emesary::ReceiptStatus::OK;
    }
};


void testEmesaryBase()
{
    printf("Testing base functions\n");

    simgear::Emesary::Transmitter* globalTransmitter = simgear::Emesary::GlobalTransmitter::instance();
    SGTimeStamp timeStamp;
    timeStamp.stamp();

    TestBaseRecipient *r = new TestBaseRecipient();

    globalTransmitter->Register(r);
     SG_CHECK_EQUAL(globalTransmitter->Count(), 1);
    for (int i = 0; i < MaxIterationsBase; i++)
    {
        SGSharedPtr<TestBaseNotification> tn (new TestBaseNotification());
        tn->index = i;
        globalTransmitter->NotifyAll(tn);
        //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
        noperations++;
    }
    globalTransmitter->DeRegister(r);
    {
        SGSharedPtr<TestBaseNotification> tn(new TestBaseNotification());
        SG_CHECK_EQUAL(globalTransmitter->Count(), 0);
        globalTransmitter->NotifyAll(tn);
        SG_CHECK_EQUAL(globalTransmitter->Count(), 0);
    }
    summary(timeStamp, globalTransmitter, "base");
}

void testEmesaryMultipleRecipients()
{
    printf("Testing multiple recipients\n");

    simgear::Emesary::Transmitter* globalTransmitter = simgear::Emesary::GlobalTransmitter::instance();
    SGTimeStamp timeStamp;
    timeStamp.stamp();



    // create and register test recipients
    std::vector<TestBaseRecipient*> recips;
    for (int i = 0; i < 5; i++)
    {
        TestBaseRecipient *newRecipient = new TestBaseRecipient;
        recips.push_back(newRecipient);
        globalTransmitter->Register(newRecipient);
    }

    // check that TestThreadNotification are ignored
    int rcount = globalTransmitter->SentMessageCount();
    SG_CHECK_EQUAL(globalTransmitter->Count(), recips.size());
    {
        // send a bunch of notifications.
        for (int i = 0; i < MaxIterationsBaseMultipleRecipients; i++)
        {
            SGSharedPtr<TestThreadNotification> ttn(new TestThreadNotification("TestThread"));
            globalTransmitter->NotifyAll(ttn);
            //System.Threading.Thread.Sleep(rng.Next(MaxSleep));
        }
        std::for_each(recips.begin(), recips.end(), [&globalTransmitter](TestBaseRecipient* r) {
            SG_CHECK_EQUAL(0, r->receiveCount);
            });
    }
    SG_CHECK_NE(rcount, static_cast<int>(globalTransmitter->SentMessageCount()));

    rcount = globalTransmitter->SentMessageCount();
    {
        SGSharedPtr < TestBaseNotification>tbn(new TestBaseNotification());
        for (int i = 0; i < MaxIterationsBaseMultipleRecipients; i++)
        {
            tbn->index = i;
            globalTransmitter->NotifyAll(tbn);
            noperations++;
        }
        std::for_each(recips.begin(), recips.end(), [&globalTransmitter](TestBaseRecipient* r) {
            SG_CHECK_EQUAL(r->receiveCount, MaxIterationsBaseMultipleRecipients);
            });

    }
    // now degregister all 
    std::for_each(recips.begin(), recips.end(), [&globalTransmitter](TestBaseRecipient* r) {
        globalTransmitter->DeRegister(r);
        });

    SG_CHECK_EQUAL(globalTransmitter->Count(), 0);
    {
        SGSharedPtr < TestThreadNotification >ttn(new TestThreadNotification("TestThread"));
        globalTransmitter->NotifyAll(ttn);
    }
    SG_CHECK_EQUAL(globalTransmitter->Count(), 0);

    summary(timeStamp, globalTransmitter, "base");
}
int main(int ac, char ** av)
{
    {
        SGSharedPtr<TestDerefNotification> tn(new TestDerefNotification());
    }
    SG_CHECK_EQUAL(TestDerefNotification_destructor_count, 1);

    // should still be deleted when the inner pointer goes out of scope.
    auto tn1 = new TestDerefNotification();
    {
        SGSharedPtr<TestDerefNotification> tn(tn1); 
    }
    SG_CHECK_EQUAL(TestDerefNotification_destructor_count, 2);

    testEmesaryBase();

    testEmesaryMultipleRecipients();

    testEmesaryThreaded();

    testEmesaryThreadedAddDuringReceive();

    std::cout << "all tests passed" << std::endl;
    return 0;
}
