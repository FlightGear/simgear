// commands.cxx - encapsulated commands.
// Started Spring 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <memory>
#include <cassert>

#include <simgear/props/props_io.hxx>

#include "commands.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>
#include <simgear/debug/logstream.hxx>


////////////////////////////////////////////////////////////////////////
// Implementation of SGCommandMgr class.
////////////////////////////////////////////////////////////////////////

static SGCommandMgr* static_instance = NULL;

SGCommandMgr::SGCommandMgr ()
{
    assert(static_instance == NULL);
    static_instance = this;
}

SGCommandMgr::~SGCommandMgr ()
{
    assert(static_instance == this);
    static_instance = NULL;
}

SGCommandMgr*
SGCommandMgr::instance()
{
    return static_instance;
}

void
SGCommandMgr::addCommandObject (const std::string &name, Command* command)
{
    if (_commands.find(name) != _commands.end())
        throw sg_exception("duplicate command name:" + name);
    
  _commands[name] = command;
}

SGCommandMgr::Command*
SGCommandMgr::getCommand (const std::string &name) const
{
  const command_map::const_iterator it = _commands.find(name);
  return (it != _commands.end() ? it->second : 0);
}

string_list
SGCommandMgr::getCommandNames () const
{
  string_list names;
  command_map::const_iterator it = _commands.begin();
  command_map::const_iterator last = _commands.end();
  while (it != last) {
    names.push_back(it->first);
    ++it;
  }
  return names;
}

bool
SGCommandMgr::execute (const std::string &name, const SGPropertyNode * arg) const
{
  Command* command = getCommand(name);
  if (command == 0)
  {
    SG_LOG(SG_GENERAL, SG_WARN, "command not found: '" << name << "'");
    return false;
  }

  try
  {
    return (*command)(arg);
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
    command_map::iterator it = _commands.find(name);
    if (it == _commands.end())
        return false;
    
    delete it->second;
    _commands.erase(it);
    return true;
}

// end of commands.cxx
