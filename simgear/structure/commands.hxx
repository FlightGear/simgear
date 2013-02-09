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

#include <string>
#include <map>

#include <simgear/threads/SGThread.hxx>
#include <simgear/math/sg_types.hxx>

// forward decls
class SGPropertyNode;
     
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
     * Command functor object
     */
    class Command
    {
    public:
        virtual ~Command() { }
        virtual bool operator()(const SGPropertyNode * arg) = 0;
    };

private:
    template< typename Fun >
    class FunctionCommand : public Command
    {
    public:
        FunctionCommand( const Fun* fun )
    	: f_(fun) {}

        virtual bool operator()(const SGPropertyNode * arg) { return (*f_)(arg); }
    private:
        Fun* f_;
    };

    template< class ObjPtr, typename MemFn >
    class MethodCommand : public Command
    {
    public:
        MethodCommand( const ObjPtr& pObj, MemFn pMemFn ) :
    	  pObj_(pObj), pMemFn_(pMemFn) {}

        virtual bool operator()(const SGPropertyNode * arg)
        {
    	     return ((*pObj_).*pMemFn_)(arg);
        }
    private:
        ObjPtr pObj_;
        MemFn pMemFn_;
    };
    
   /**
    * Helper template functions.
    */

   template< typename Fun >
   Command* make_functor( const Fun* fun )
   {
       return new FunctionCommand<Fun>(fun);
   }

   template< class ObjPtr, typename MemFn >
   Command* make_functor( const ObjPtr& pObj, MemFn pMemFn )
   {
       return new MethodCommand<ObjPtr,MemFn>(pObj, pMemFn );
   }
   
public:
    
  typedef bool (*command_t) (const SGPropertyNode * arg);

  /**
   * Destructor.
   */
  virtual ~SGCommandMgr ();

  /**
   * Implement the classical singleton.
   */
  static SGCommandMgr* instance();

  /**
   * Register a new command with the manager.
   *
   * @param name The command name.  Any existing command with
   * the same name will silently be overwritten.
   * @param command A pointer to a one-arg function returning
   * a bool result.  The argument is always a const pointer to
   * an SGPropertyNode (which may contain multiple values).
   */
  template<typename FUNC>
  void addCommand(const std::string& name, const FUNC* f)
  { addCommand(name, make_functor(f)); }
       
  void addCommand (const std::string &name, Command* command);

  template<class OBJ, typename METHOD>
  void addCommand(const std::string& name, const OBJ& o, METHOD m)
  { 
    addCommand(name, make_functor(o,m));
  }
  
  /**
   * Look up an existing command.
   *
   * @param name The command name.
   * @return A pointer to the command, or 0 if there is no registered
   * command with the name specified.
   */
  virtual Command* getCommand (const std::string &name) const;


  /**
   * Get a list of all existing command names.
   *
   * @return A (possibly empty) vector of the names of all registered
   * commands.
   */
  virtual string_list getCommandNames () const;


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
  virtual bool execute (const std::string &name, const SGPropertyNode * arg) const;

protected:
  /**
   * Default constructor.
   */
  SGCommandMgr ();


private:

  typedef std::map<std::string,Command*> command_map;
  command_map _commands;

  static SGMutex _instanceMutex;

};

#endif // __COMMANDS_HXX

// end of commands.hxx
