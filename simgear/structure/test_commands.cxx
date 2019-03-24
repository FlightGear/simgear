#include <simgear_config.h>

#include <cstdio>
#include <algorithm>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/commands.hxx>
#include <simgear/misc/test_macros.hxx>
#include <simgear/props/props.hxx>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

SGPropertyNode_ptr test_rootNode;

bool commandAFunc(const SGPropertyNode* args, SGPropertyNode* root)
{
    SG_VERIFY(root == test_rootNode.get());
    root->setIntValue("propA", args->getIntValue("argA"));
    return true;
}

bool commandBFunc(const SGPropertyNode* args, SGPropertyNode* root)
{
    SG_VERIFY(root == test_rootNode.get());
    root->setIntValue("propB", args->getIntValue("argB"));
    return args->getBoolValue("result");
}

void testBasicCommands()
{
    test_rootNode.reset(new SGPropertyNode);
    SGCommandMgr::instance()->setImplicitRoot(test_rootNode.get());
    
    SGCommandMgr::instance()->addCommand("cmd-a", commandAFunc);
    SGCommandMgr::instance()->addCommand("cmd-b", commandBFunc);

    auto names = SGCommandMgr::instance()->getCommandNames();
    SG_CHECK_EQUAL(names.size(), 2);
    SG_VERIFY(std::count(names.begin(), names.end(), "cmd-a"));
    SG_VERIFY(std::count(names.begin(), names.end(), "cmd-b"));

    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("argA", 42);
        bool ok = SGCommandMgr::instance()->execute("cmd-a", args);
        SG_VERIFY(ok);
        SG_CHECK_EQUAL(test_rootNode->getIntValue("propA"), 42);
    }
                       
    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("argB", 99);
        args->setBoolValue("result", true);
        bool ok = SGCommandMgr::instance()->execute("cmd-b", args);
        SG_VERIFY(ok);
        SG_CHECK_EQUAL(test_rootNode->getIntValue("propB"), 99);
    }
    
    SGCommandMgr::instance()->removeCommand("cmd-a");
    names = SGCommandMgr::instance()->getCommandNames();
    SG_CHECK_EQUAL(names.size(), 1);
    SG_VERIFY(std::count(names.begin(), names.end(), "cmd-b"));
    
    // check we can't execute a removed command
    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("argA", 66);
        bool ok = SGCommandMgr::instance()->execute("cmd-a", args);
        SG_VERIFY(!ok);
        SG_CHECK_EQUAL(test_rootNode->getIntValue("propA"), 42);
    }
}

///////////////////////////////////////////////////////////////////////////////

void testQueuedExec()
{
    // delete the previous instance, so next call re-creates
    delete SGCommandMgr::instance();
    
    test_rootNode.reset(new SGPropertyNode);
    test_rootNode->setIntValue("propA", 71);
    
    SGCommandMgr::instance()->setImplicitRoot(test_rootNode.get());
    
    SGCommandMgr::instance()->addCommand("cmd-a", commandAFunc);
    SGCommandMgr::instance()->addCommand("cmd-b", commandBFunc);
    
    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("argA", 99);
        SGCommandMgr::instance()->queuedExecute("cmd-a", args);
        
        // ensure args are cpatured during enqueue call
        args->setIntValue("argA", 101);
    }
    
    // not executed yet
    SG_CHECK_EQUAL(test_rootNode->getIntValue("propA"), 71);
    
    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("argB", 1234);
        args->setBoolValue("result", true);
        SGCommandMgr::instance()->queuedExecute("cmd-b", args);
    }
    
    SGCommandMgr::instance()->executedQueuedCommands();
    
    SG_CHECK_EQUAL(test_rootNode->getIntValue("propA"), 99);
    SG_CHECK_EQUAL(test_rootNode->getIntValue("propB"), 1234);
}

///////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    testBasicCommands();
    testQueuedExec();
    
    cout << __FILE__ << ": All tests passed" << endl;
    return EXIT_SUCCESS;
}
