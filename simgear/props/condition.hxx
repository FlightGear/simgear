/**
 * \file condition.hxx
 * Declarations and inline methods for property conditions.
 * Written by David Megginson, started 2000.
 * CLO May 2003 - Split out condition specific code.
 *
 * This file is in the Public Domain, and comes with no warranty.
 */

#ifndef __SG_CONDITION_HXX
#define __SG_CONDITION_HXX

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>


////////////////////////////////////////////////////////////////////////
// Conditions.
////////////////////////////////////////////////////////////////////////


/**
 * An encoded condition.
 *
 * This class encodes a single condition of some sort, possibly
 * connected with properties.
 *
 * This class should migrate to somewhere more general.
 */
class SGCondition
{
public:
  SGCondition ();
  virtual ~SGCondition ();
  virtual bool test () const = 0;
};


/**
 * Condition for a single property.
 *
 * This condition is true only if the property returns a boolean
 * true value.
 */
class SGPropertyCondition : public SGCondition
{
public:
  SGPropertyCondition ( SGPropertyNode *prop_root,
                        const char * propname  );
  virtual ~SGPropertyCondition ();
  virtual bool test () const { return _node->getBoolValue(); }
private:
  SGConstPropertyNode_ptr _node;
};


/**
 * Condition for a 'not' operator.
 *
 * This condition is true only if the child condition is false.
 */
class SGNotCondition : public SGCondition
{
public:
				// transfer pointer ownership
  SGNotCondition (SGCondition * condition);
  virtual ~SGNotCondition ();
  virtual bool test () const;
private:
  SGCondition * _condition;
};


/**
 * Condition for an 'and' group.
 *
 * This condition is true only if all of the conditions
 * in the group are true.
 */
class SGAndCondition : public SGCondition
{
public:
  SGAndCondition ();
  virtual ~SGAndCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (SGCondition * condition);
private:
  vector<SGCondition *> _conditions;
};


/**
 * Condition for an 'or' group.
 *
 * This condition is true if at least one of the conditions in the
 * group is true.
 */
class SGOrCondition : public SGCondition
{
public:
  SGOrCondition ();
  virtual ~SGOrCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (SGCondition * condition);
private:
  vector<SGCondition *> _conditions;
};


/**
 * Abstract base class for property comparison conditions.
 */
class SGComparisonCondition : public SGCondition
{
public:
  enum Type {
    LESS_THAN,
    GREATER_THAN,
    EQUALS
  };
  SGComparisonCondition (Type type, bool reverse = false);
  virtual ~SGComparisonCondition ();
  virtual bool test () const;
  virtual void setLeftProperty( SGPropertyNode *prop_root,
                                const char * propname );
  virtual void setRightProperty( SGPropertyNode *prop_root,
                                 const char * propname );
				// will make a local copy
  virtual void setRightValue (const SGPropertyNode * value);
private:
  Type _type;
  bool _reverse;
  SGConstPropertyNode_ptr _left_property;
  SGConstPropertyNode_ptr _right_property;
  SGConstPropertyNode_ptr _right_value;
};


/**
 * Base class for a conditional components.
 *
 * This class manages the conditions and tests; the component should
 * invoke the test() method whenever it needs to decide whether to
 * active itself, draw itself, and so on.
 */
class SGConditional
{
public:
  SGConditional ();
  virtual ~SGConditional ();
				// transfer pointer ownership
  virtual void setCondition (SGCondition * condition);
  virtual const SGCondition * getCondition () const { return _condition; }
  virtual bool test () const;
private:
  SGCondition * _condition;
};


/**
 * Global function to make a condition out of properties.
 *
 * The top-level is always an implicit 'and' group, whatever the
 * node's name (it should usually be "condition").
 *
 * @param node The top-level condition node (usually named "condition").
 * @return A pointer to a newly-allocated condition; it is the
 *         responsibility of the caller to delete the condition when
 *         it is no longer needed.
 */
SGCondition *sgReadCondition( SGPropertyNode *prop_root,
                              const SGPropertyNode *node );


#endif // __SG_CONDITION_HXX

