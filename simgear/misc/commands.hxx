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


#include <string>
#include <map>
#include <vector>

#include "props.hxx"

using std::string;
using std::map;
using std::vector;


/**
 * Manage commands.
 *
 * <p>This class allows the application to register and unregister
 * commands, and provides shortcuts for executing them.  Commands
 * are simple functions that take a const pointer to an SGPropertyNode
 * as an argument and return a bool value indicating success or failure.
 * The property node may be ignored, or it may contain values that
 * the command uses as parameters.</p>
 *
 * <p>There are convenience methods for invoking a command function
 * with no arguments or with a single, primitive argument.</p>
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
   * This is the primary method for invoking a command; the others
   * are convenience methods that invoke this one indirectly.
   * 
   * @param name The name of the command.
   * @param arg A const pointer to an SGPropertyNode.  The node
   * may have a value and/or children, etc., so that it is possible
   * to pass an arbitrarily complex data structure to a command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, const SGPropertyNode * arg) const;


  /**
   * Execute a command with no argument.
   *
   * The command function will receive a pointer to a property node
   * with no value and no children.
   *
   * @param name The name of the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name) const;


  /**
   * Execute a command with a single bool argument.
   *
   * The command function will receive a pointer to a property node
   * with a bool value and no children.
   *
   * @param name The name of the command.
   * @param arg The bool argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, bool arg) const;


  /**
   * Execute a command with a single int argument.
   *
   * The command function will receive a pointer to a property node
   * with a int value and no children.
   *
   * @param name The name of the command.
   * @param arg The int argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, int arg) const;


  /**
   * Execute a command with a single long argument.
   *
   * The command function will receive a pointer to a property node
   * with a long value and no children.
   *
   * @param name The name of the command.
   * @param arg The long argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, long arg) const;


  /**
   * Execute a command with a single float argument.
   *
   * The command function will receive a pointer to a property node
   * with a float value and no children.
   *
   * @param name The name of the command.
   * @param arg The float argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, float arg) const;


  /**
   * Execute a command with a single double argument.
   *
   * The command function will receive a pointer to a property node
   * with a double value and no children.
   *
   * @param name The name of the command.
   * @param arg The double argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, double arg) const;


  /**
   * Execute a command with a single string argument.
   *
   * The command function will receive a pointer to a property node
   * with a string value and no children.
   *
   * @param name The name of the command.
   * @param arg The string argument to the command.
   * @return true if the command is present and executes successfully,
   * false otherwise.
   */
  virtual bool execute (const string &name, string arg) const;


private:

  typedef map<string,command_t> command_map;
  command_map _commands;

};

#endif __COMMANDS_HXX

// end of commands.hxx
