/**
 * \file commands.hxx
 * Interface definition for encapsulated commands.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id$
 */

#ifndef __COMMANDS_HXX
#define __COMMANDS_HXX


#include <simgear/compiler.h>

#include STL_STRING
#include <map>
#include <vector>

#include <simgear/props/props.hxx>

SG_USING_STD(string);
SG_USING_STD(map);
SG_USING_STD(vector);


/**
 * Manage commands.
 *
 * <p>This class allows the application to register and unregister
 * commands, and provides shortcuts for executing them.  Commands are
 * simple functions that take a const pointer to an SGPropertyNode:
 * the function may use the nodes children as parameters.</p>
 *
 * @author David Megginson, david@megginson.com
 */
class SGCommandMgr
{
public:

  /**
   * Type for a command function.
   */
  typedef bool (*command_t) (const SGPropertyNode * arg);


  /**
   * Default constructor.
   */
  SGCommandMgr ();


  /**
   * Destructor.
   */
  virtual ~SGCommandMgr ();


  /**
   * Register a new command with the manager.
   *
   * @param name The command name.  Any existing command with
   * the same name will silently be overwritten.
   * @param command A pointer to a one-arg function returning
   * a bool result.  The argument is always a const pointer to
   * an SGPropertyNode (which may contain multiple values).
   */
  virtual void addCommand (const string &name, command_t command);


  /**
   * Look up an existing command.
   *
   * @param name The command name.
   * @return A pointer to the command, or 0 if there is no registered
   * command with the name specified.
   */
  virtual command_t getCommand (const string &name) const;


  /**
   * Get a list of all existing command names.
   *
   * @return A (possibly empty) vector of the names of all registered
   * commands.
   */
  virtual vector<string> getCommandNames () const;


  /**
   * Execute a command.
   *
   * @param name The name of the command.
   * @param arg A const pointer to an SGPropertyNode.  The node
   * may have a value and/or children, etc., so that it is possible
   * to pass an arbitrarily complex data structure to a command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, const SGPropertyNode * arg) const;

private:

  typedef map<string,command_t> command_map;
  command_map _commands;

};

#endif // __COMMANDS_HXX

// end of commands.hxx
