// condition.cxx - Declarations and inline methods for property conditions.
//
// Written by David Megginson, started 2000.
// CLO May 2003 - Split out condition specific code.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

// #include <iostream>

#include <simgear/structure/exception.hxx>

#include "props.hxx"
#include "condition.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGExpression.hxx>

using std::istream;
using std::ostream;

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
 * Condition with constant value
 *
 */
class SGConstantCondition : public SGCondition
{
public:
  SGConstantCondition (bool v) : _value(v) { ; }
  virtual bool test () const { return _value; }
private:
  bool _value;
};

/**
 * Condition for a 'not' operator.
 *
 * This condition is true only if the child condition is false.
 */
class SGNotCondition : public SGCondition
{
public:
  SGNotCondition (SGCondition * condition);
  virtual ~SGNotCondition ();
  virtual bool test () const;
private:
  SGSharedPtr<SGCondition> _condition;
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
  std::vector<SGSharedPtr<SGCondition> > _conditions;
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
  std::vector<SGSharedPtr<SGCondition> > _conditions;
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
  virtual void setLeftValue (const SGPropertyNode * value);
  virtual void setRightValue (const SGPropertyNode * value);
  
  void setLeftDExpression(SGExpressiond* dexp);
  void setRightDExpression(SGExpressiond* dexp);
  
private:
  Type _type;
  bool _reverse;
  SGPropertyNode_ptr _left_property;
  SGPropertyNode_ptr _right_property;
  
  SGSharedPtr<SGExpressiond> _left_dexp;
  SGSharedPtr<SGExpressiond> _right_dexp;
};


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
  using namespace simgear;
  switch (left->getType()) {
  case props::BOOL: {
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
  case props::INT: {
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
  case props::LONG: {
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
  case props::FLOAT: {
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
  case props::DOUBLE: {
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
  case props::STRING:
  case props::NONE:
  case props::UNSPECIFIED: {
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
  default:
    throw sg_exception("condition: unrecognized node type in comparison");
  }
  
  return 0;
}


SGComparisonCondition::SGComparisonCondition (Type type, bool reverse)
  : _type(type),
    _reverse(reverse)
{
}

SGComparisonCondition::~SGComparisonCondition ()
{
}

bool
SGComparisonCondition::test () const
{
				// Always fail if incompletely specified
  if (!_left_property || !_right_property)
    return false;

  // Get LESS_THAN, EQUALS, or GREATER_THAN
  if (_left_dexp) {
    _left_property->setDoubleValue(_left_dexp->getValue(NULL));
  }
  
  if (_right_dexp) {
    _right_property->setDoubleValue(_right_dexp->getValue(NULL));
  }
        
  int cmp = doComparison(_left_property, _right_property);
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
  _right_property = prop_root->getNode(propname, true);
}

void
SGComparisonCondition::setLeftValue (const SGPropertyNode *node)
{
  _left_property = new SGPropertyNode(*node);
}


void
SGComparisonCondition::setRightValue (const SGPropertyNode *node)
{
  _right_property = new SGPropertyNode(*node);
}

void
SGComparisonCondition::setLeftDExpression(SGExpressiond* dexp)
{
  _left_property = new SGPropertyNode();
  _left_dexp = dexp;
}

void
SGComparisonCondition::setRightDExpression(SGExpressiond* dexp)
{
  _right_property = new SGPropertyNode();
  _right_dexp = dexp;
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
  if (node->nChildren() != 2) {
    throw sg_exception("condition: comparison without two children");
  }
  
  const SGPropertyNode* left = node->getChild(0), 
    *right = node->getChild(1);
  string leftName(left->getName());
  if (leftName == "property") {
    condition->setLeftProperty(prop_root, left->getStringValue());
  } else if (leftName == "value") {
    condition->setLeftValue(left);
  } else if (leftName == "expression") {
    SGExpressiond* exp = SGReadDoubleExpression(prop_root, left->getChild(0));
    condition->setLeftDExpression(exp);
  } else {
    throw sg_exception("Unknown condition comparison left child:" + leftName);
  }
    
  string rightName(right->getName());
  if (rightName == "property") {
    condition->setRightProperty(prop_root, right->getStringValue());
  } else if (rightName == "value") {
    condition->setRightValue(right);
  } else if (rightName == "expression") {
    SGExpressiond* exp = SGReadDoubleExpression(prop_root, right->getChild(0));
    condition->setRightDExpression(exp);
  } else {
    throw sg_exception("Unknown condition comparison right child:" + rightName);
  }
  
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
  else if (name == "false") 
    return new SGConstantCondition(false);
  else if (name == "true")
    return new SGConstantCondition(true);
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
