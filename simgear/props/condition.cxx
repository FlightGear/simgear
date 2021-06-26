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

#include <simgear/structure/SGExpression.hxx>

using std::istream;
using std::ostream;
using std::string;

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
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
    { props.insert(_node.get()); }
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
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
private:
  SGConditionRef _condition;
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
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
private:
  std::vector<SGConditionRef> _conditions;
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
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
private:
  std::vector<SGConditionRef> _conditions;
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
  virtual void setPrecisionProperty( SGPropertyNode *prop_root,
                                 const char * propname );
  // will make a local copy
  virtual void setLeftValue (const SGPropertyNode * value);
  virtual void setRightValue (const SGPropertyNode * value);
  virtual void setPrecisionValue (const SGPropertyNode * value);
  
  void setLeftDExpression(SGExpressiond* dexp);
  void setRightDExpression(SGExpressiond* dexp);
  void setPrecisionDExpression(SGExpressiond* dexp);
  
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
private:
  Type _type;
  bool _reverse;
  SGPropertyNode_ptr _left_property;
  SGPropertyNode_ptr _right_property;
  SGPropertyNode_ptr _precision_property;
  
  SGSharedPtr<SGExpressiond> _left_dexp;
  SGSharedPtr<SGExpressiond> _right_dexp;
  SGSharedPtr<SGExpressiond> _precision_dexp;
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

void
SGNotCondition::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
    _condition->collectDependentProperties(props);
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
  for( size_t i = 0; i < _conditions.size(); i++ )
  {
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

void
SGAndCondition::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
  for( size_t i = 0; i < _conditions.size(); i++ )
    _conditions[i]->collectDependentProperties(props);
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
  for( size_t i = 0; i < _conditions.size(); i++ )
  {
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

void
SGOrCondition::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
  for( size_t i = 0; i < _conditions.size(); i++ )
    _conditions[i]->collectDependentProperties(props);
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGComparisonCondition.
////////////////////////////////////////////////////////////////////////

template<typename T> 
static int doComp( T v1, T v2, T e )
{
  T d = v1 - v2;
  if( d < -e )
    return SGComparisonCondition::LESS_THAN;
  else if( d > e )
    return SGComparisonCondition::GREATER_THAN;
  else
    return SGComparisonCondition::EQUALS;
}

static int
doComparison (const SGPropertyNode * left, const SGPropertyNode * right, const SGPropertyNode * precision )
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
  case props::INT:
    return doComp<int>(left->getIntValue(), right->getIntValue(), 
           precision ? std::abs(precision->getIntValue()/2) : 0 );

  case props::LONG:
    return doComp<long>(left->getLongValue(), right->getLongValue(), 
           precision ? std::abs(precision->getLongValue()/2L) : 0L );

  case props::FLOAT:
    return doComp<float>(left->getFloatValue(), right->getFloatValue(), 
           precision ? std::fabs(precision->getFloatValue()/2.0f) : 0.0f );

  case props::DOUBLE:
    return doComp<double>(left->getDoubleValue(), right->getDoubleValue(), 
           precision ? std::fabs(precision->getDoubleValue()/2.0) : 0.0 );

  case props::STRING:
  case props::NONE:
  case props::UNSPECIFIED: {
    size_t l = precision ? precision->getLongValue() : string::npos;
    string v1 = string(left->getStringValue()).substr(0,l);
    string v2 = string(right->getStringValue()).substr(0,l);
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

  if (_precision_dexp) {
    _precision_property->setDoubleValue(_precision_dexp->getValue(NULL));
  }
        
  int cmp = doComparison(_left_property, _right_property, _precision_property );
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
SGComparisonCondition::setPrecisionProperty( SGPropertyNode *prop_root,
                                         const char * propname )
{
  _precision_property = prop_root->getNode(propname, true);
}

void
SGComparisonCondition::setLeftValue (const SGPropertyNode *node)
{
  _left_property = new SGPropertyNode(*node);
}

void
SGComparisonCondition::setPrecisionValue (const SGPropertyNode *node)
{
  _precision_property = new SGPropertyNode(*node);
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

void
SGComparisonCondition::setPrecisionDExpression(SGExpressiond* dexp)
{
  _precision_property = new SGPropertyNode();
  _precision_dexp = dexp;
}

void
SGComparisonCondition::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
  if (_left_dexp)
    _left_dexp->collectDependentProperties(props);
  else
    props.insert(_left_property);
  
  if (_right_dexp)
    _right_dexp->collectDependentProperties(props);
  else
    props.insert(_right_property);
  
  if (_precision_dexp)
    _precision_dexp->collectDependentProperties(props);
  else if (_precision_property)
    props.insert(_precision_property);
  
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
  if (node->nChildren() < 2 || node->nChildren() > 3 ) {
    throw sg_exception("condition: comparison without two or three children",
                       {}, {}, false);
  }
  
  const SGPropertyNode* left = node->getChild(0), 
    *right = node->getChild(1);

  {
    string leftName(left->getName());
    if (leftName == "property") {
      condition->setLeftProperty(prop_root, left->getStringValue());
    } else if (leftName == "value") {
      condition->setLeftValue(left);
    } else if (leftName == "expression") {
      SGExpressiond* exp = SGReadDoubleExpression(prop_root, left->getChild(0));
      condition->setLeftDExpression(exp);
    } else {
      throw sg_exception("Unknown condition comparison left child:" + leftName,
                         {}, {}, false);
    }
  }
    
  {
    string rightName(right->getName());
    if (rightName == "property") {
      condition->setRightProperty(prop_root, right->getStringValue());
    } else if (rightName == "value") {
      condition->setRightValue(right);
    } else if (rightName == "expression") {
      SGExpressiond* exp = SGReadDoubleExpression(prop_root, right->getChild(0));
      condition->setRightDExpression(exp);
    } else {
      throw sg_exception("Unknown condition comparison right child:" + rightName,
                         {}, {}, false);
    }
  }
  
  if( node->nChildren() == 3 ) {
    const SGPropertyNode *n = node->getChild(2);
    string name(n->getName());
    if (name == "precision-property") {
      condition->setPrecisionProperty(prop_root, n->getStringValue());
    } else if (name == "precision-value") {
      condition->setPrecisionValue(n);
    } else if (name == "precision-expression") {
      SGExpressiond* exp = SGReadDoubleExpression(prop_root, n->getChild(0));
      condition->setPrecisionDExpression(exp);
    } else {
      throw sg_exception("Unknown condition comparison precision child:" + name,
                         {}, {}, false);
    }
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
