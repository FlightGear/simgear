// commands.cxx - encapsulated commands.
// Started Spring 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#include "commands.hxx"
#include "props_io.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of SGCommandState class.
////////////////////////////////////////////////////////////////////////

SGCommandState::SGCommandState ()
  : _args(0)
{
}

SGCommandState::SGCommandState (const SGPropertyNode * args)
  : _args(0)
{
  setArgs(args);
}

SGCommandState::~SGCommandState ()
{
  delete _args;
}

void
SGCommandState::setArgs (const SGPropertyNode * args)
{
  delete _args;
  _args = new SGPropertyNode();
  copyProperties(args, _args);
}



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
SGCommandMgr::execute (const string &name, const SGPropertyNode * arg,
		       SGCommandState ** state) const
{
  command_t command = getCommand(name);
  if (command == 0)
    return false;
  else
    return (*command)(arg, state);
}


// bool
// SGCommandMgr::execute (const string &name) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, bool value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setBoolValue(value);
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, int value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setIntValue(value);
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, long value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setLongValue(value);
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, float value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setFloatValue(value);
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, double value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setDoubleValue(value);
//   return execute(name, &node);
// }


// bool
// SGCommandMgr::execute (const string &name, string value) const
// {
// 				// FIXME
//   SGPropertyNode node;
//   node.setStringValue(value);
//   return execute(name, &node);
// }


// end of commands.cxx
