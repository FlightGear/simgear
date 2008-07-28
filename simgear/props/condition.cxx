// condition.cxx - Declarations and inline methods for property conditions.
//
// Written by David Megginson, started 2000.
// CLO May 2003 - Split out condition specific code.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear/compiler.h>
#endif

// #include <iostream>

#include <simgear/structure/exception.hxx>

#include "props.hxx"
#include "condition.hxx"

using std::istream;
using std::ostream;




////////////////////////////////////////////////////////////////////////
// Implementation of SGCondition.
////////////////////////////////////////////////////////////////////////

SGCondition::SGCondition ()
{
}

SGCondition::~SGCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyCondition.
////////////////////////////////////////////////////////////////////////

SGPropertyCondition::SGPropertyCondition ( SGPropertyNode *prop_root,
                                           const char *propname )
    : _node( prop_root->getNode(propname, true) )
{
}

SGPropertyCondition::~SGPropertyCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGNotCondition.
////////////////////////////////////////////////////////////////////////

SGNotCondition::SGNotCondition (SGCondition * condition)
  : _condition(condition)
{
}

SGNotCondition::~SGNotCondition ()
{
}

bool
SGNotCondition::test () const
{
  return !(_condition->test());
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGAndCondition.
////////////////////////////////////////////////////////////////////////

SGAndCondition::SGAndCondition ()
{
}

SGAndCondition::~SGAndCondition ()
{
}

bool
SGAndCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (!_conditions[i]->test())
      return false;
  }
  return true;
}

void
SGAndCondition::addCondition (SGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGOrCondition.
////////////////////////////////////////////////////////////////////////

SGOrCondition::SGOrCondition ()
{
}

SGOrCondition::~SGOrCondition ()
{
}

bool
SGOrCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (_conditions[i]->test())
      return true;
  }
  return false;
}

void
SGOrCondition::addCondition (SGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGComparisonCondition.
////////////////////////////////////////////////////////////////////////

static int
doComparison (const SGPropertyNode * left, const SGPropertyNode *right)
{
  switch (left->getType()) {
  case SGPropertyNode::BOOL: {
    bool v1 = left->getBoolValue();
    bool v2 = right->getBoolValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::INT: {
    int v1 = left->getIntValue();
    int v2 = right->getIntValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::LONG: {
    long v1 = left->getLongValue();
    long v2 = right->getLongValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::FLOAT: {
    float v1 = left->getFloatValue();
    float v2 = right->getFloatValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::DOUBLE: {
    double v1 = left->getDoubleValue();
    double v2 = right->getDoubleValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::STRING: 
  case SGPropertyNode::NONE:
  case SGPropertyNode::UNSPECIFIED: {
    string v1 = left->getStringValue();
    string v2 = right->getStringValue();
    if (v1 < v2)
      return SGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return SGComparisonCondition::GREATER_THAN;
    else
      return SGComparisonCondition::EQUALS;
    break;
  }
  }
  throw sg_exception("condition: unrecognized node type in comparison");
  return 0;
}


SGComparisonCondition::SGComparisonCondition (Type type, bool reverse)
  : _type(type),
    _reverse(reverse),
    _left_property(0),
    _right_property(0),
    _right_value(0)
{
}

SGComparisonCondition::~SGComparisonCondition ()
{
}

bool
SGComparisonCondition::test () const
{
				// Always fail if incompletely specified
  if (_left_property == 0 ||
      (_right_property == 0 && _right_value == 0))
    return false;

				// Get LESS_THAN, EQUALS, or GREATER_THAN
  int cmp =
    doComparison(_left_property,
		 (_right_property != 0 ? _right_property : _right_value));
  if (!_reverse)
    return (cmp == _type);
  else
    return (cmp != _type);
}

void
SGComparisonCondition::setLeftProperty( SGPropertyNode *prop_root,
                                        const char * propname )
{
  _left_property = prop_root->getNode(propname, true);
}

void
SGComparisonCondition::setRightProperty( SGPropertyNode *prop_root,
                                         const char * propname )
{
  _right_value = 0;
  _right_property = prop_root->getNode(propname, true);
}

void
SGComparisonCondition::setRightValue (const SGPropertyNode *node)
{
  _right_property = 0;
  _right_value = new SGPropertyNode(*node);
}



////////////////////////////////////////////////////////////////////////
// Read a condition and use it if necessary.
////////////////////////////////////////////////////////////////////////

                                // Forward declaration
static SGCondition * readCondition( SGPropertyNode *prop_root,
                                    const SGPropertyNode *node );

static SGCondition *
readPropertyCondition( SGPropertyNode *prop_root,
                       const SGPropertyNode *node )
{
  return new SGPropertyCondition( prop_root, node->getStringValue() );
}

static SGCondition *
readNotCondition( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    SGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      return new SGNotCondition(condition);
  }
  SG_LOG(SG_COCKPIT, SG_ALERT, "empty 'not' condition");
  return 0;
}

static SGCondition *
readAndConditions( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  SGAndCondition * andCondition = new SGAndCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    SGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      andCondition->addCondition(condition);
  }
  return andCondition;
}

static SGCondition *
readOrConditions( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  SGOrCondition * orCondition = new SGOrCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    SGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      orCondition->addCondition(condition);
  }
  return orCondition;
}

static SGCondition *
readComparison( SGPropertyNode *prop_root,
                const SGPropertyNode *node,
                SGComparisonCondition::Type type,
		bool reverse)
{
  SGComparisonCondition * condition = new SGComparisonCondition(type, reverse);
  condition->setLeftProperty(prop_root, node->getStringValue("property[0]"));
  if (node->hasValue("property[1]"))
    condition->setRightProperty(prop_root, node->getStringValue("property[1]"));
  else if (node->hasValue("value"))
    condition->setRightValue(node->getChild("value", 0));
  else
    throw sg_exception("condition: comparison without property[1] or value");

  return condition;
}

static SGCondition *
readCondition( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  const string &name = node->getName();
  if (name == "property")
    return readPropertyCondition(prop_root, node);
  else if (name == "not")
    return readNotCondition(prop_root, node);
  else if (name == "and")
    return readAndConditions(prop_root, node);
  else if (name == "or")
    return readOrConditions(prop_root, node);
  else if (name == "less-than")
    return readComparison(prop_root, node, SGComparisonCondition::LESS_THAN,
                          false);
  else if (name == "less-than-equals")
    return readComparison(prop_root, node, SGComparisonCondition::GREATER_THAN,
                          true);
  else if (name == "greater-than")
    return readComparison(prop_root, node, SGComparisonCondition::GREATER_THAN,
                          false);
  else if (name == "greater-than-equals")
    return readComparison(prop_root, node, SGComparisonCondition::LESS_THAN,
                          true);
  else if (name == "equals")
    return readComparison(prop_root, node, SGComparisonCondition::EQUALS,
                          false);
  else if (name == "not-equals")
    return readComparison(prop_root, node, SGComparisonCondition::EQUALS, true);
  else
    return 0;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGConditional.
////////////////////////////////////////////////////////////////////////

SGConditional::SGConditional ()
  : _condition (0)
{
}

SGConditional::~SGConditional ()
{
}

void
SGConditional::setCondition (SGCondition * condition)
{
  _condition = condition;
}

bool
SGConditional::test () const
{
  return ((_condition == 0) || _condition->test());
}



// The top-level is always an implicit 'and' group
SGCondition *
sgReadCondition( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  return readAndConditions(prop_root, node);
}


// end of condition.cxx
