// commands.cxx - encapsulated commands.
// Started Spring 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#include "commands.hxx"
#include "props_io.hxx"



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
