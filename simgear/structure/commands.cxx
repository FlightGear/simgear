// commands.cxx - encapsulated commands.
// Started Spring 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#include <simgear_config.h>

#include <memory>
#include <cassert>
#include <mutex>

#include <simgear/props/props_io.hxx>

#include "commands.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/debug/logstream.hxx>

struct Invocation
{
  std::string command;
  SGPropertyNode_ptr args;
};

class SGCommandMgr::Private
{
public:
  SGPropertyNode_ptr _rootNode;
  
  using InvocactionVec = std::vector<Invocation>;
  InvocactionVec _queue;

  std::mutex _mutex;

  using command_map = std::map<std::string,Command*> ;
  command_map _commands;
  
  long _mainThreadId = 0;
};

////////////////////////////////////////////////////////////////////////
// Implementation of SGCommandMgr class.
////////////////////////////////////////////////////////////////////////

static SGCommandMgr* static_instance = nullptr;

SGCommandMgr::SGCommandMgr () :
  d(new Private)
{
    d->_mainThreadId = SGThread::current();
    assert(static_instance == nullptr);
    static_instance = this;
}

SGCommandMgr::~SGCommandMgr ()
{
    assert(static_instance == this);
    static_instance = nullptr;
}

SGCommandMgr*
SGCommandMgr::instance()
{
    return static_instance ? static_instance : new SGCommandMgr;
}

void SGCommandMgr::setImplicitRoot(SGPropertyNode *root)
{
  assert(root);
  d->_rootNode = root;
}

void
SGCommandMgr::addCommandObject (const std::string &name, Command* command)
{
#if !defined(NDEBUG)
  assert(SGThread::current() == d->_mainThreadId);
#endif
  if (d->_commands.find(name) != d->_commands.end())
    throw sg_exception("duplicate command name:" + name);

  d->_commands[name] = command;
}

SGCommandMgr::Command*
SGCommandMgr::getCommand (const std::string &name) const
{
  const auto it = d->_commands.find(name);
  return (it != d->_commands.end() ? it->second : nullptr);
}

string_list
SGCommandMgr::getCommandNames () const
{
  string_list names;
  auto it = d->_commands.begin();
  auto last = d->_commands.end();
  while (it != last) {
    names.push_back(it->first);
    ++it;
  }
  return names;
}

bool
SGCommandMgr::execute (const std::string &name, const SGPropertyNode * arg, SGPropertyNode *root) const
{
#if !defined(NDEBUG)
    if (SGThread::current() != d->_mainThreadId) {
        SG_LOG(SG_GENERAL, SG_WARN, "calling SGCommandMgr::execute from a different thread than expected. Command is:" << name);
    }
  // assert(SGThread::current() == d->_mainThreadId);
#endif
  Command* command = getCommand(name);
  if (command == nullptr)
  {
    SG_LOG(SG_GENERAL, SG_WARN, "command not found: '" << name << "'");
    return false;
  }

  // use implicit root node if caller did not define one explicitly
  if (root == nullptr) {
    root = d->_rootNode.get();
  }
  
  try
  {
    return (*command)(arg, root);
  }
  catch(sg_exception& e)
  {
    SG_LOG
    (
      SG_GENERAL,
      SG_ALERT,
      "command '" << name << "' failed with exception\n"
      "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")"
    );
  }
  catch(std::exception& ex)
  {
    SG_LOG
    (
      SG_GENERAL,
      SG_ALERT,
      "command '" << name << "' failed with exception: " << ex.what()
    );
  }
  catch(...)
  {
    SG_LOG
    (
      SG_GENERAL,
      SG_ALERT,
      "command '" << name << "' failed with unknown exception."
    );
  }

  return false;
}

bool SGCommandMgr::removeCommand(const std::string& name)
{
#if !defined(NDEBUG)
  assert(SGThread::current() == d->_mainThreadId);
#endif
    auto it = d->_commands.find(name);
    if (it == d->_commands.end())
        return false;

    delete it->second;
    d->_commands.erase(it);
    return true;
}

void SGCommandMgr::queuedExecute(const std::string &name, const SGPropertyNode* arg)
{
  Invocation invoke = {name, new SGPropertyNode};
  copyProperties(arg, invoke.args);
  std::lock_guard<std::mutex> g(d->_mutex);
  d->_queue.push_back(invoke);
}

void SGCommandMgr::executedQueuedCommands()
{
#if !defined(NDEBUG)
  assert(SGThread::current() == d->_mainThreadId);
#endif
  
  // locked swap with the shared queue
  Private::InvocactionVec q;
  {
    std::lock_guard<std::mutex> g(d->_mutex);
    d->_queue.swap(q);
  }
  
  for (auto i : q) {
    bool ok = execute(i.command, i.args);
    if (!ok) {
      SG_LOG(SG_GENERAL, SG_WARN, "queued execute of command " << i.command
             << "failed");
    }
  }
}


// end of commands.cxx
