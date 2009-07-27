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

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>

#include "commands.hxx"

#include <simgear/math/SGMath.hxx>



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

OpenThreads::Mutex SGCommandMgr::_instanceMutex;

SGCommandMgr*
SGCommandMgr::instance()
{
  static std::auto_ptr<SGCommandMgr> mgr;
  if (mgr.get())
    return mgr.get();

  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_instanceMutex);
  if (mgr.get())
    return mgr.get();

  mgr = std::auto_ptr<SGCommandMgr>(new SGCommandMgr);
  return mgr.get();
}

void
SGCommandMgr::addCommand (const string &name, command_t command)
{
  _commands[name] = command;
}

SGCommandMgr::command_t
SGCommandMgr::getCommand (const string &name) const
{
  const command_map::const_iterator it = _commands.find(name);
  return (it != _commands.end() ? it->second : 0);
}

vector<string>
SGCommandMgr::getCommandNames () const
{
  vector<string> names;
  command_map::const_iterator it = _commands.begin();
  command_map::const_iterator last = _commands.end();
  while (it != last) {
    names.push_back(it->first);
    it++;
  }
  return names;
}

bool
SGCommandMgr::execute (const string &name, const SGPropertyNode * arg) const
{
  command_t command = getCommand(name);
  if (command == 0)
    return false;
  else
    return (*command)(arg);
}

// end of commands.cxx
