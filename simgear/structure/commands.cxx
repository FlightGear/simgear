// commands.cxx - encapsulated commands.
// Started Spring 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <memory>
#include <simgear/props/props_io.hxx>

#include "commands.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>
#include <simgear/debug/logstream.hxx>


////////////////////////////////////////////////////////////////////////
// Implementation of SGCommandMgr class.
////////////////////////////////////////////////////////////////////////


SGCommandMgr::SGCommandMgr ()
{
  // no-op
}

SGCommandMgr::~SGCommandMgr ()
{
  // no-op
}

SGMutex SGCommandMgr::_instanceMutex;

SGCommandMgr*
SGCommandMgr::instance()
{
  static std::auto_ptr<SGCommandMgr> mgr;
  if (mgr.get())
    return mgr.get();

  SGGuard<SGMutex> lock(_instanceMutex);
  if (mgr.get())
    return mgr.get();

  mgr = std::auto_ptr<SGCommandMgr>(new SGCommandMgr);
  return mgr.get();
}

void
SGCommandMgr::addCommand (const std::string &name, command_t command)
{
  _commands[name] = command;
}

SGCommandMgr::command_t
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
  command_t command = getCommand(name);
  if (command == 0)
    return false;


  try {
    return (*command)(arg);
  } catch (sg_exception& e) {
    SG_LOG(SG_GENERAL, SG_ALERT, "command '" << name << "' failed with exception\n"
      << "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")");
    return false;
  }
}

// end of commands.cxx
