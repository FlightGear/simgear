// condition.hxx - Declarations and inline methods for property conditions.
// Written by David Megginson, started 2000.
// CLO May 2003 - Split out condition specific code.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear/compiler.h>
#endif

// #include STL_IOSTREAM

#include <simgear/misc/exception.hxx>

#include "props.hxx"

#include "condition.hxx"

SG_USING_STD(istream);
SG_USING_STD(ostream);




////////////////////////////////////////////////////////////////////////
// Implementation of FGCondition.
////////////////////////////////////////////////////////////////////////

FGCondition::FGCondition ()
{
}

FGCondition::~FGCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPropertyCondition.
////////////////////////////////////////////////////////////////////////

FGPropertyCondition::FGPropertyCondition ( SGPropertyNode *prop_root,
                                           const char *propname )
    : _node( prop_root->getNode(propname, true) )
{
    cout << "FGPropertyCondition::FGPropertyCondition()" << endl;
    cout << "  prop_root = " << prop_root << endl;
    cout << "  propname = " << propname << endl;
    _node = prop_root->getNode(propname, true);
    cout << "  _node = " << _node << endl;
}

FGPropertyCondition::~FGPropertyCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGNotCondition.
////////////////////////////////////////////////////////////////////////

FGNotCondition::FGNotCondition (FGCondition * condition)
  : _condition(condition)
{
}

FGNotCondition::~FGNotCondition ()
{
  delete _condition;
}

bool
FGNotCondition::test () const
{
  return !(_condition->test());
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAndCondition.
////////////////////////////////////////////////////////////////////////

FGAndCondition::FGAndCondition ()
{
}

FGAndCondition::~FGAndCondition ()
{
  for (unsigned int i = 0; i < _conditions.size(); i++)
    delete _conditions[i];
}

bool
FGAndCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (!_conditions[i]->test())
      return false;
  }
  return true;
}

void
FGAndCondition::addCondition (FGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGOrCondition.
////////////////////////////////////////////////////////////////////////

FGOrCondition::FGOrCondition ()
{
}

FGOrCondition::~FGOrCondition ()
{
  for (unsigned int i = 0; i < _conditions.size(); i++)
    delete _conditions[i];
}

bool
FGOrCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (_conditions[i]->test())
      return true;
  }
  return false;
}

void
FGOrCondition::addCondition (FGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGComparisonCondition.
////////////////////////////////////////////////////////////////////////

static int
doComparison (const SGPropertyNode * left, const SGPropertyNode *right)
{
  switch (left->getType()) {
  case SGPropertyNode::BOOL: {
    bool v1 = left->getBoolValue();
    bool v2 = right->getBoolValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::INT: {
    int v1 = left->getIntValue();
    int v2 = right->getIntValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::LONG: {
    long v1 = left->getLongValue();
    long v2 = right->getLongValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::FLOAT: {
    float v1 = left->getFloatValue();
    float v2 = right->getFloatValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::DOUBLE: {
    double v1 = left->getDoubleValue();
    double v2 = right->getDoubleValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::STRING: 
  case SGPropertyNode::NONE:
  case SGPropertyNode::UNSPECIFIED: {
    string v1 = left->getStringValue();
    string v2 = right->getStringValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  }
  throw sg_exception("Unrecognized node type");
  return 0;
}


FGComparisonCondition::FGComparisonCondition (Type type, bool reverse)
  : _type(type),
    _reverse(reverse),
    _left_property(0),
    _right_property(0),
    _right_value(0)
{
}

FGComparisonCondition::~FGComparisonCondition ()
{
  delete _right_value;
}

bool
FGComparisonCondition::test () const
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
FGComparisonCondition::setLeftProperty( SGPropertyNode *prop_root,
                                        const char * propname )
{
  _left_property = prop_root->getNode(propname, true);
}

void
FGComparisonCondition::setRightProperty( SGPropertyNode *prop_root,
                                         const char * propname )
{
  delete _right_value;
  _right_value = 0;
  _right_property = prop_root->getNode(propname, true);
}

void
FGComparisonCondition::setRightValue (const SGPropertyNode *node)
{
  _right_property = 0;
  delete _right_value;
  _right_value = new SGPropertyNode(*node);
}



////////////////////////////////////////////////////////////////////////
// Read a condition and use it if necessary.
////////////////////////////////////////////////////////////////////////

                                // Forward declaration
static FGCondition * readCondition( SGPropertyNode *prop_root,
                                    const SGPropertyNode *node );

static FGCondition *
readPropertyCondition( SGPropertyNode *prop_root,
                       const SGPropertyNode *node )
{
  return new FGPropertyCondition( prop_root, node->getStringValue() );
}

static FGCondition *
readNotCondition( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      return new FGNotCondition(condition);
  }
  SG_LOG(SG_COCKPIT, SG_ALERT, "Panel: empty 'not' condition");
  return 0;
}

static FGCondition *
readAndConditions( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  FGAndCondition * andCondition = new FGAndCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      andCondition->addCondition(condition);
  }
  return andCondition;
}

static FGCondition *
readOrConditions( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  FGOrCondition * orCondition = new FGOrCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(prop_root, child);
    if (condition != 0)
      orCondition->addCondition(condition);
  }
  return orCondition;
}

static FGCondition *
readComparison( SGPropertyNode *prop_root,
                const SGPropertyNode *node,
                FGComparisonCondition::Type type,
		bool reverse)
{
  FGComparisonCondition * condition = new FGComparisonCondition(type, reverse);
  condition->setLeftProperty(prop_root, node->getStringValue("property[0]"));
  if (node->hasValue("property[1]"))
    condition->setRightProperty(prop_root, node->getStringValue("property[1]"));
  else
    condition->setRightValue(node->getChild("value", 0));

  return condition;
}

static FGCondition *
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
    return readComparison(prop_root, node, FGComparisonCondition::LESS_THAN,
                          false);
  else if (name == "less-than-equals")
    return readComparison(prop_root, node, FGComparisonCondition::GREATER_THAN,
                          true);
  else if (name == "greater-than")
    return readComparison(prop_root, node, FGComparisonCondition::GREATER_THAN,
                          false);
  else if (name == "greater-than-equals")
    return readComparison(prop_root, node, FGComparisonCondition::LESS_THAN,
                          true);
  else if (name == "equals")
    return readComparison(prop_root, node, FGComparisonCondition::EQUALS,
                          false);
  else if (name == "not-equals")
    return readComparison(prop_root, node, FGComparisonCondition::EQUALS, true);
  else
    return 0;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGConditional.
////////////////////////////////////////////////////////////////////////

FGConditional::FGConditional ()
  : _condition (0)
{
}

FGConditional::~FGConditional ()
{
  delete _condition;
}

void
FGConditional::setCondition (FGCondition * condition)
{
  delete _condition;
  _condition = condition;
}

bool
FGConditional::test () const
{
  return ((_condition == 0) || _condition->test());
}



// The top-level is always an implicit 'and' group
FGCondition *
fgReadCondition( SGPropertyNode *prop_root, const SGPropertyNode *node )
{
  return readAndConditions(prop_root, node);
}


// end of fg_props.cxx
