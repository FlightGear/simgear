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
#include <memory>

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
        virtual bool operator()(const SGPropertyNode * arg, SGPropertyNode *root) = 0;
    };


	  typedef bool (*command_t) (const SGPropertyNode * arg, SGPropertyNode *root);

private:
    class FunctionCommand : public Command
    {
    public:
        FunctionCommand( command_t fun )
    	: f_(fun) {}

        virtual bool operator()(const SGPropertyNode * arg, SGPropertyNode *root) { return (*f_)(arg, root); }
    private:
        command_t f_;
    };

    template< class ObjPtr, typename MemFn >
    class MethodCommand : public Command
    {
    public:
        MethodCommand( const ObjPtr& pObj, MemFn pMemFn ) :
    	  pObj_(pObj), pMemFn_(pMemFn) {}

        virtual bool operator()(const SGPropertyNode * arg, SGPropertyNode *root)
        {
    	     return ((*pObj_).*pMemFn_)(arg,root);
        }
    private:
        ObjPtr pObj_;
        MemFn pMemFn_;
    };

   /**
    * Helper template functions.
    */

   template< class ObjPtr, typename MemFn >
   Command* make_functor( const ObjPtr& pObj, MemFn pMemFn )
   {
       return new MethodCommand<ObjPtr,MemFn>(pObj, pMemFn );
   }

public:
   /**
    * Default constructor (sets instance to created item)
    */
   SGCommandMgr ();

  /**
   * Destructor. (sets instance to NULL)
   */
  virtual ~SGCommandMgr ();

  static SGCommandMgr* instance();

  /**
   * specify the root node to use if one is not explicitly provided when
   * executing a command.
   */
  void setImplicitRoot(SGPropertyNode* root);
    
  /**
   * Register a new command with the manager.
   *
   * @param name    The command name. Any existing command with the same name
   *                will silently be overwritten.
   * @param f       A pointer to a one-arg function returning a bool result. The
   *                argument is always a const pointer to an SGPropertyNode
   *                (which may contain multiple values).
   */
  void addCommand(const std::string& name, command_t f)
  { addCommandObject(name, new FunctionCommand(f)); }

  void addCommandObject (const std::string &name, Command* command);

  template<class OBJ, typename METHOD>
  void addCommand(const std::string& name, const OBJ& o, METHOD m)
  {
    addCommandObject(name, make_functor(o,m));
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
  virtual bool execute (const std::string &name, const SGPropertyNode * arg, SGPropertyNode *root = nullptr) const;

  /**
   * Queue a command for execution on the main thread / thread owning
   * this command manager. (In practice in FlightGear, this means the same thing)
   * The argument node will be copied immediately, so changes made after this call
   * will not be reflected when the command is executed.
   *
   * Note there is no way to unqueue a command, or find out when the execution
   * actually occurs, at present. Queued ommands are executed in the order submitted,
   * however, which can be used to infer completion.
   *
   * Queued execution always uses the implicit root node.
   */
  void queuedExecute(const std::string &name, const SGPropertyNode* arg);

  /**
   * Dispatch queued command. FlightGear calls this once per frame
   * on the main thread.
   *
   */
  void executedQueuedCommands();

  /**
   * Remove a command registration
   */
  bool removeCommand(const std::string& name);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // __COMMANDS_HXX

// end of commands.hxx
