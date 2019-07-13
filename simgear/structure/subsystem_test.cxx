#include <simgear_config.h>

#include <cstdio>
#include <algorithm>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/test_macros.hxx>
#include <simgear/props/props.hxx>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

///////////////////////////////////////////////////////////////////////////////
// sample subsystems 

class MySub1 : public SGSubsystem
{
public:
    static const char* staticSubsystemClassId() { return "mysub"; }

    void init() override
    {
        wasInited = true;
    }
    
    void bind() override
    {
        auto node = get_manager()->root_node();
        if (node)
            node->setIntValue("mysub/foo", 42);
    }
    
    void update(double dt) override
    {
        
    }
    
    bool wasInited = false;
};

class AnotherSub : public SGSubsystem
{
public:
    static const char* staticSubsystemClassId() { return "anothersub"; }

    void init() override
    {
        
    }
    
    void bind() override
    {
        auto node = get_manager()->root_node();
        if (node)
            node->setIntValue("anothersub/bar", 172);
    }
    
    void update(double dt) override
    {
        lastUpdateTime = dt;
    }
    
    double lastUpdateTime = 0.0;
};

class FakeRadioSub : public SGSubsystem
{
public:
    static const char* staticSubsystemClassId() { return "fake-radio"; }

    void init() override
    {
        wasInited = true;
    }
    
    void update(double dt) override
    {
        lastUpdateTime = dt;
    }
    
    bool wasInited = false;
    double lastUpdateTime = 0.0;
};

class InstrumentGroup : public SGSubsystemGroup
{
public:
    static const char* staticSubsystemClassId() { return "instruments"; }

    virtual ~InstrumentGroup()
    {
    }
    
    void init() override
    {
        wasInited = true;
        SGSubsystemGroup::init();
    }
    
    void update(double dt) override
    {
        lastUpdateTime = dt;
        SGSubsystemGroup::update(dt);
    }
    
    bool wasInited = false;
    double lastUpdateTime = 0.0;
};

///////////////////////////////////////////////////////////////////////////////
// sample delegate

class RecorderDelegate : public SGSubsystemMgr::Delegate
{
public:
    void willChange(SGSubsystem* sub, SGSubsystem::State newState) override
    {
        events.push_back({sub->subsystemId(), false, newState});
    }
    void didChange(SGSubsystem* sub, SGSubsystem::State newState) override
    {
        events.push_back({sub->subsystemId(), true, newState});
    }

    struct Event {
        string subsystem;
        bool didChange;
        SGSubsystem::State event;
        
        std::string nameForEvent() const
        {
            return subsystem + (didChange ? "-did-" : "-will-") + SGSubsystem::nameForState(event);
        }
    };
    
    using EventVec = std::vector<Event>;
    EventVec events;
    
    EventVec::const_iterator findEvent(const std::string& name) const
    {
        auto it = std::find_if(events.begin(), events.end(), [name](const Event& ev)
                               { return ev.nameForEvent() == name; });
        return it;
    }
    
    bool hasEvent(const std::string& name) const
    {
        return findEvent(name) != events.end();
    }
};

///////////////////////////////////////////////////////////////////////////////

SGSubsystemMgr::Registrant<MySub1> registrant(SGSubsystemMgr::GENERAL);
SGSubsystemMgr::Registrant<AnotherSub> registrant2(SGSubsystemMgr::FDM);

SGSubsystemMgr::Registrant<InstrumentGroup> registrant4(SGSubsystemMgr::FDM);

SGSubsystemMgr::InstancedRegistrant<FakeRadioSub> registrant3(SGSubsystemMgr::POST_FDM);

void testRegistrationAndCreation()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    
    auto anotherSub = manager->create<AnotherSub>();
    SG_VERIFY(anotherSub);
    SG_CHECK_EQUAL(anotherSub->subsystemId(), AnotherSub::staticSubsystemClassId());
    SG_CHECK_EQUAL(anotherSub->subsystemId(), std::string("anothersub"));
    SG_CHECK_EQUAL(anotherSub->subsystemClassId(), std::string("anothersub"));
    SG_CHECK_EQUAL(anotherSub->subsystemInstanceId(), std::string());

    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");

    
}

void testAddGetRemove()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    auto anotherSub = manager->add<AnotherSub>();
    SG_VERIFY(anotherSub);
    SG_CHECK_EQUAL(anotherSub->subsystemId(), AnotherSub::staticSubsystemClassId());
    SG_CHECK_EQUAL(anotherSub->subsystemId(), std::string("anothersub"));
    
    SG_VERIFY(d->hasEvent("anothersub-will-add"));
    SG_VERIFY(d->hasEvent("anothersub-did-add"));

    auto lookup = manager->get_subsystem<AnotherSub>();
    SG_CHECK_EQUAL(lookup, anotherSub);
    
    SG_CHECK_EQUAL(manager->get_subsystem("anothersub"), anotherSub);
    
    // manual create & add
    auto mySub = manager->create<MySub1>();
    manager->add(MySub1::staticSubsystemClassId(), mySub.ptr(), SGSubsystemMgr::DISPLAY, 0.1234);
    
    SG_VERIFY(d->hasEvent("mysub-will-add"));
    SG_VERIFY(d->hasEvent("mysub-did-add"));
    
    SG_CHECK_EQUAL(manager->get_subsystem<MySub1>(), mySub);
    
    bool ok = manager->remove(AnotherSub::staticSubsystemClassId());
    SG_VERIFY(ok);
    SG_VERIFY(d->hasEvent("anothersub-will-remove"));
    SG_VERIFY(d->hasEvent("anothersub-did-remove"));
    
    SG_VERIFY(manager->get_subsystem<AnotherSub>() == nullptr);
    
    // lookup after remove
    SG_CHECK_EQUAL(manager->get_subsystem<MySub1>(), mySub);
    
    // re-add of removed, and let's test overriding
    auto another2 = manager->add<AnotherSub>(SGSubsystemMgr::SOUND);
    SG_CHECK_EQUAL(another2->subsystemId(), AnotherSub::staticSubsystemClassId());
    
    auto soundGroup = manager->get_group(SGSubsystemMgr::SOUND);
    SG_CHECK_EQUAL(soundGroup->get_subsystem("anothersub"), another2);

}

void testSubGrouping()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    auto anotherSub = manager->add<AnotherSub>();
    auto instruments = manager->add<InstrumentGroup>();
    
    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");
    
    SG_CHECK_EQUAL(radio1->subsystemId(), std::string("fake-radio.nav1"));
    SG_CHECK_EQUAL(radio2->subsystemId(), std::string("fake-radio.nav2"));
    SG_CHECK_EQUAL(radio1->subsystemClassId(), std::string("fake-radio"));
    SG_CHECK_EQUAL(radio2->subsystemInstanceId(), std::string("nav2"));
    
    instruments->set_subsystem(radio1);
    instruments->set_subsystem(radio2);
    
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-add"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-add"));
    
    // lookup of the group should also work
    SG_CHECK_EQUAL(manager->get_subsystem<InstrumentGroup>(), instruments);
    
    manager->bind();
    manager->init();
    
    SG_VERIFY(instruments->wasInited);
    SG_VERIFY(radio1->wasInited);
    SG_VERIFY(radio2->wasInited);

    SG_VERIFY(d->hasEvent("instruments-will-init"));
    SG_VERIFY(d->hasEvent("instruments-did-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav2-did-init"));

    manager->update(0.5);
    SG_CHECK_EQUAL_EP(0.5, instruments->lastUpdateTime);
    SG_CHECK_EQUAL_EP(0.5, radio1->lastUpdateTime);
    SG_CHECK_EQUAL_EP(0.5, radio2->lastUpdateTime);

    SG_CHECK_EQUAL(0, instruments->get_subsystem("fake-radio"));

    
    SG_CHECK_EQUAL(radio1, instruments->get_subsystem("fake-radio.nav1"));
    SG_CHECK_EQUAL(radio2, instruments->get_subsystem("fake-radio.nav2"));

    // type-safe lookup of instanced
    SG_CHECK_EQUAL(radio1, manager->get_subsystem<FakeRadioSub>("nav1"));
    SG_CHECK_EQUAL(radio2, manager->get_subsystem<FakeRadioSub>("nav2"));
    
    bool ok = manager->remove("fake-radio.nav2");
    SG_VERIFY(ok);
    SG_VERIFY(instruments->get_subsystem("fake-radio.nav2") == nullptr);
    
    manager->update(1.0);
    SG_CHECK_EQUAL_EP(1.0, instruments->lastUpdateTime);
    SG_CHECK_EQUAL_EP(1.0, radio1->lastUpdateTime);
    
    // should not have been updated
    SG_CHECK_EQUAL_EP(0.5, radio2->lastUpdateTime);
    
    manager->unbind();
    SG_VERIFY(d->hasEvent("instruments-will-unbind"));
    SG_VERIFY(d->hasEvent("instruments-did-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-unbind"));
    
}

void testIncrementalInit()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    // place everything into the same group, so incremental init has
    // some work to do
    auto mySub = manager->add<MySub1>(SGSubsystemMgr::POST_FDM);
    auto anotherSub = manager->add<AnotherSub>(SGSubsystemMgr::POST_FDM);
    auto instruments = manager->add<InstrumentGroup>(SGSubsystemMgr::POST_FDM);
    
    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");
    instruments->set_subsystem(radio1);
    instruments->set_subsystem(radio2);
    
    
    manager->bind();
    for ( ; ; ) {
        auto status = manager->incrementalInit();
        if (status == SGSubsystemMgr::INIT_DONE)
            break;
    }

    SG_VERIFY(mySub->wasInited);

    SG_VERIFY(d->hasEvent("mysub-will-init"));
    SG_VERIFY(d->hasEvent("mysub-did-init"));

    SG_VERIFY(d->hasEvent("anothersub-will-init"));
    SG_VERIFY(d->hasEvent("anothersub-did-init"));
    
   // SG_VERIFY(d->hasEvent("instruments-will-init"));
   // SG_VERIFY(d->hasEvent("instruments-did-init"));

    
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-init"));
    
    SG_VERIFY(d->hasEvent("fake-radio.nav2-will-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav2-did-init"));
}

void testEmptyGroup()
{
    // testing the assert described here:
    // https://sourceforge.net/p/flightgear/codetickets/2043/
    // when an empty group is inited, we skipped setting the state
    
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    auto mySub = manager->add<MySub1>(SGSubsystemMgr::POST_FDM);
    auto anotherSub = manager->add<AnotherSub>(SGSubsystemMgr::POST_FDM);
    auto instruments = manager->add<InstrumentGroup>(SGSubsystemMgr::POST_FDM);
    
    manager->bind();
    for ( ; ; ) {
        auto status = manager->incrementalInit();
        if (status == SGSubsystemMgr::INIT_DONE)
            break;
    }
    manager->postinit();
    
    SG_VERIFY(mySub->wasInited);

    SG_VERIFY(d->hasEvent("instruments-will-init"));
    SG_VERIFY(d->hasEvent("instruments-did-init"));
}

void testSuspendResume()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    auto anotherSub = manager->add<AnotherSub>();
    auto instruments = manager->add<InstrumentGroup>();
    
    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");
    
    instruments->set_subsystem(radio1);
    instruments->set_subsystem(radio2);
    
    manager->bind();
    manager->init();
    manager->update(1.0);
    
    SG_CHECK_EQUAL_EP(1.0, anotherSub->lastUpdateTime);
    SG_CHECK_EQUAL_EP(1.0, instruments->lastUpdateTime);
    SG_CHECK_EQUAL_EP(1.0, radio1->lastUpdateTime);
    
    anotherSub->suspend();
    radio1->resume(); // should be a no-op

    SG_VERIFY(d->hasEvent("anothersub-will-suspend"));
    SG_VERIFY(d->hasEvent("anothersub-did-suspend"));
    SG_VERIFY(!d->hasEvent("fake-radio.nav1-will-suspend"));

    manager->update(0.5);
    
    SG_CHECK_EQUAL_EP(1.0, anotherSub->lastUpdateTime);
    SG_CHECK_EQUAL_EP(0.5, instruments->lastUpdateTime);
    SG_CHECK_EQUAL_EP(0.5, radio1->lastUpdateTime);
    
    // suspend the whole group
    instruments->suspend();
    anotherSub->resume();
    SG_VERIFY(d->hasEvent("anothersub-will-resume"));
    SG_VERIFY(d->hasEvent("anothersub-did-resume"));
    
    SG_VERIFY(d->hasEvent("instruments-will-suspend"));
    SG_VERIFY(d->hasEvent("instruments-did-suspend"));
    
    manager->update(2.0);
    
    SG_CHECK_EQUAL_EP(2.5, anotherSub->lastUpdateTime);
    
    // this is significant, since SGSubsystemGroup::update is still
    // called in this case
    SG_CHECK_EQUAL_EP(2.0, instruments->lastUpdateTime);
    SG_CHECK_EQUAL_EP(0.5, radio1->lastUpdateTime);
    
    // twiddle the state of a radio while its whole group is suspended
    // this should not notify!
    d->events.clear();
    radio2->suspend();
    SG_VERIFY(d->events.empty());
    
    instruments->resume();
    manager->update(3.0);
    SG_CHECK_EQUAL_EP(3.0, anotherSub->lastUpdateTime);
    SG_CHECK_EQUAL_EP(3.0, instruments->lastUpdateTime);
    
    // should see all the passed time now
    SG_CHECK_EQUAL_EP(5.0, radio1->lastUpdateTime);
    SG_CHECK_EQUAL_EP(5.0, radio2->lastUpdateTime);
}

void testPropertyRoot()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    SGPropertyNode_ptr props(new SGPropertyNode);
    manager->set_root_node(props);
    
    auto d = new RecorderDelegate;
    manager->addDelegate(d);

    manager->add<MySub1>();
    auto anotherSub = manager->add<AnotherSub>();
    auto instruments = manager->add<InstrumentGroup>();
    
    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");
    
    instruments->set_subsystem(radio1);
    instruments->set_subsystem(radio2);

    manager->bind();
    manager->init();

    SG_CHECK_EQUAL(props->getIntValue("mysub/foo"), 42);
    SG_CHECK_EQUAL(props->getIntValue("anothersub/bar"), 172);
}

void testAddRemoveAfterInit()
{
    SGSharedPtr<SGSubsystemMgr> manager = new SGSubsystemMgr();
    auto d = new RecorderDelegate;
    manager->addDelegate(d);
    
    auto group = manager->add<InstrumentGroup>();
    SG_VERIFY(group);

    auto radio1 = manager->createInstance<FakeRadioSub>("nav1");
    group->set_subsystem(radio1);

    auto com1 = manager->createInstance<FakeRadioSub>("com1");
    group->set_subsystem(com1);
    auto com2 = manager->createInstance<FakeRadioSub>("com2");
    group->set_subsystem(com2);
    
    manager->bind();
    manager->init();
    
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-bind"));

    auto radio2 = manager->createInstance<FakeRadioSub>("nav2");
    group->set_subsystem(radio2);
    
    SG_VERIFY(d->hasEvent("fake-radio.nav2-will-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav2-did-init"));
    SG_VERIFY(d->hasEvent("fake-radio.nav2-did-bind"));
    
    bool ok = manager->remove("fake-radio.nav1");
    SG_VERIFY(ok);
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-shutdown"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-shutdown"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-will-remove"));
    SG_VERIFY(d->hasEvent("fake-radio.nav1-did-remove"));
    
    manager->shutdown();
    
    SG_VERIFY(d->hasEvent("fake-radio.nav2-will-shutdown"));
    SG_VERIFY(d->hasEvent("fake-radio.nav2-did-shutdown"));
    SG_VERIFY(d->hasEvent("fake-radio.com1-will-shutdown"));
    SG_VERIFY(d->hasEvent("fake-radio.com1-did-shutdown"));
    
    d->events.clear();
    
    ok = manager->remove("fake-radio.com1");
    SG_VERIFY(d->hasEvent("fake-radio.com1-will-remove"));
    SG_VERIFY(d->hasEvent("fake-radio.com1-did-remove"));
    SG_VERIFY(d->hasEvent("fake-radio.com1-will-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.com1-did-unbind"));
    SG_VERIFY(!d->hasEvent("fake-radio.com1-will-shutdown"));
    SG_VERIFY(!d->hasEvent("fake-radio.com1-did-shutdown"));
    
    manager->unbind();
    
    SG_VERIFY(d->hasEvent("fake-radio.com2-will-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.com2-did-unbind"));
    
    d->events.clear();
    manager->remove("fake-radio.com2");
    SG_VERIFY(!d->hasEvent("fake-radio.com2-will-unbind"));
    SG_VERIFY(!d->hasEvent("fake-radio.com2-did-unbind"));
    SG_VERIFY(d->hasEvent("fake-radio.com2-will-remove"));
    SG_VERIFY(d->hasEvent("fake-radio.com2-did-remove"));
}

///////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    testRegistrationAndCreation();
    testAddGetRemove();
    testSubGrouping();
    testIncrementalInit();
    testSuspendResume();
    testPropertyRoot();
    testAddRemoveAfterInit();
    testEmptyGroup();
    
    cout << __FILE__ << ": All tests passed" << endl;
    return EXIT_SUCCESS;
}
