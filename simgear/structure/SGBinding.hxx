/**
 * \file commands.hxx
 * Interface definition for encapsulated commands.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id$
 */

#ifndef __SGBINDING_HXX
#define __SGBINDING_HXX


#include <simgear/compiler.h>

#include <string>
#include <map>
#include <vector>

#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>

#include "commands.hxx"

/**
 * An input binding of some sort.
 *
 * <p>This class represents a binding that can be assigned to a
 * keyboard key, a joystick button or axis, or even a panel
 * instrument.</p>
 */
class SGBinding : public SGConditional
{
public:

  /**
   * Default constructor.
   */
  SGBinding ();


  /**
   * Convenience constructor.
   *
   * @param node The binding will be built from this node.
   */
  SGBinding (const SGPropertyNode * node, SGPropertyNode* root);


  /**
   * Destructor.
   */
  virtual ~SGBinding ();


  /**
   * Get the command name.
   *
   * @return The string name of the command for this binding.
   */
  const string &getCommandName () const { return _command_name; }


  /**
   * Get the command itself.
   *
   * @return The command associated with this binding, or 0 if none
   * is present.
   */
  SGCommandMgr::command_t getCommand () const { return _command; }


  /**
   * Get the argument that will be passed to the command.
   *
   * @return A property node that will be passed to the command as its
   * argument, or 0 if none was supplied.
   */
  const SGPropertyNode * getArg () { return _arg; }
  

  /**
   * Read a binding from a property node.
   *
   * @param node The property node containing the binding.
   */
  void read (const SGPropertyNode * node, SGPropertyNode* root);


  /**
   * Fire a binding.
   */
  void fire () const;


  /**
   * Fire a binding with a scaled movement (rather than absolute position).
   */
  void fire (double offset, double max) const;


  /**
   * Fire a binding with a setting (i.e. joystick axis).
   *
   * A double 'setting' property will be added to the arguments.
   *
   * @param setting The input setting, usually between -1.0 and 1.0.
   */
  void fire (double setting) const;


private:
                                // just to be safe.
  SGBinding (const SGBinding &binding);

  std::string _command_name;
  mutable SGCommandMgr::command_t _command;
  mutable SGPropertyNode_ptr _arg;
  mutable SGPropertyNode_ptr _setting;
};

typedef std::vector<SGSharedPtr<SGBinding> > SGBindingList;
typedef std::map<unsigned,SGBindingList> SGBindingMap;

#endif
