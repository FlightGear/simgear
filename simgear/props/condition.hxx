// condition.hxx - Declarations and inline methods for property conditions.
// Written by David Megginson, started 2000.
// CLO May 2003 - Split out condition specific code.
//
// This file is in the Public Domain, and comes with no warranty.

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
class FGCondition
{
public:
  FGCondition ();
  virtual ~FGCondition ();
  virtual bool test () const = 0;
};


/**
 * Condition for a single property.
 *
 * This condition is true only if the property returns a boolean
 * true value.
 */
class FGPropertyCondition : public FGCondition
{
public:
  FGPropertyCondition ( SGPropertyNode *prop_root,
                        const char * propname  );
  virtual ~FGPropertyCondition ();
  virtual bool test () const { return _node->getBoolValue(); }
private:
  const SGPropertyNode * _node;
};


/**
 * Condition for a 'not' operator.
 *
 * This condition is true only if the child condition is false.
 */
class FGNotCondition : public FGCondition
{
public:
				// transfer pointer ownership
  FGNotCondition (FGCondition * condition);
  virtual ~FGNotCondition ();
  virtual bool test () const;
private:
  FGCondition * _condition;
};


/**
 * Condition for an 'and' group.
 *
 * This condition is true only if all of the conditions
 * in the group are true.
 */
class FGAndCondition : public FGCondition
{
public:
  FGAndCondition ();
  virtual ~FGAndCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (FGCondition * condition);
private:
  vector<FGCondition *> _conditions;
};


/**
 * Condition for an 'or' group.
 *
 * This condition is true if at least one of the conditions in the
 * group is true.
 */
class FGOrCondition : public FGCondition
{
public:
  FGOrCondition ();
  virtual ~FGOrCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (FGCondition * condition);
private:
  vector<FGCondition *> _conditions;
};


/**
 * Abstract base class for property comparison conditions.
 */
class FGComparisonCondition : public FGCondition
{
public:
  enum Type {
    LESS_THAN,
    GREATER_THAN,
    EQUALS
  };
  FGComparisonCondition (Type type, bool reverse = false);
  virtual ~FGComparisonCondition ();
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
  const SGPropertyNode * _left_property;
  const SGPropertyNode * _right_property;
  const SGPropertyNode * _right_value;
};


/**
 * Base class for a conditional components.
 *
 * This class manages the conditions and tests; the component should
 * invoke the test() method whenever it needs to decide whether to
 * active itself, draw itself, and so on.
 */
class FGConditional
{
public:
  FGConditional ();
  virtual ~FGConditional ();
				// transfer pointer ownership
  virtual void setCondition (FGCondition * condition);
  virtual const FGCondition * getCondition () const { return _condition; }
  virtual bool test () const;
private:
  FGCondition * _condition;
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
FGCondition * fgReadCondition( SGPropertyNode *prop_root,
                               const SGPropertyNode *node );


#endif // __SG_CONDITION_HXX

