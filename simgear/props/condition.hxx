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

#include <set>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

class SGPropertyNode;

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
class SGCondition : public SGReferenced
{
public:
  SGCondition ();
  virtual ~SGCondition ();
  virtual bool test () const = 0;
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const { }
};

typedef SGSharedPtr<SGCondition> SGConditionRef;


/**
 * Base class for a conditional components.
 *
 * This class manages the conditions and tests; the component should
 * invoke the test() method whenever it needs to decide whether to
 * active itself, draw itself, and so on.
 */
class SGConditional : public SGReferenced
{
public:
  SGConditional ();
  virtual ~SGConditional ();
				// transfer pointer ownership
  virtual void setCondition (SGCondition * condition);
  virtual const SGCondition * getCondition () const { return _condition; }
  virtual bool test () const;
private:
  SGConditionRef _condition;
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

