// props.cxx - implementation of a property list.
// Started Fall 2000 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// See props.html for documentation [replace with URL when available].
//
// $Id$

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>
#include <stdlib.h>
#include STL_IOSTREAM
#include <algorithm>
#include "props.hxx"

SG_USING_STD(sort);



////////////////////////////////////////////////////////////////////////
// Local classes.
////////////////////////////////////////////////////////////////////////

/**
 * Comparator class for sorting by index.
 */
class CompareIndices
{
public:
  int operator() (const SGPropertyNode * n1, const SGPropertyNode *n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};



////////////////////////////////////////////////////////////////////////
// Convenience macros for value access.
////////////////////////////////////////////////////////////////////////

#define GET_BOOL (_value.bool_val->getValue())
#define GET_INT (_value.int_val->getValue())
#define GET_LONG (_value.long_val->getValue())
#define GET_FLOAT (_value.float_val->getValue())
#define GET_DOUBLE (_value.double_val->getValue())
#define GET_STRING (_value.string_val->getValue())

#define SET_BOOL(val) (_value.bool_val->setValue(val))
#define SET_INT(val) (_value.int_val->setValue(val))
#define SET_LONG(val) (_value.long_val->setValue(val))
#define SET_FLOAT(val) (_value.float_val->setValue(val))
#define SET_DOUBLE(val) (_value.double_val->setValue(val))
#define SET_STRING(val) (_value.string_val->setValue(val))



////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

const bool SGRawValue<bool>::DefaultValue = false;
const int SGRawValue<int>::DefaultValue = 0;
const long SGRawValue<long>::DefaultValue = 0L;
const float SGRawValue<float>::DefaultValue = 0.0;
const double SGRawValue<double>::DefaultValue = 0.0L;
const string SGRawValue<string>::DefaultValue = "";



////////////////////////////////////////////////////////////////////////
// Local path normalization code.
////////////////////////////////////////////////////////////////////////

/**
 * A component in a path.
 */
struct PathComponent
{
  string name;
  int index;
};

/**
 * Parse the name for a path component.
 *
 * Name: [_a-zA-Z][-._a-zA-Z0-9]*
 */
static inline string
parse_name (const string &path, int &i)
{
  string name = "";
  int max = path.size();

  if (path[i] == '.') {
    i++;
    if (i < max && path[i] == '.') {
      i++;
      name = "..";
    } else {
      name = ".";
    }
    if (i < max && path[i] != '/')
      throw string(string("Illegal character after ") + name);
  }

  else if (isalpha(path[i]) || path[i] == '_') {
    name += path[i];
    i++;

				// The rules inside a name are a little
				// less restrictive.
    while (i < max) {
      if (isalpha(path[i]) || isdigit(path[i]) || path[i] == '_' ||
	  path[i] == '-' || path[i] == '.') {
	name += path[i];
      } else if (path[i] == '[' || path[i] == '/') {
	break;
      } else {
	throw string("name may contain only ._- and alphanumeric characters");
      }
      i++;
    }
  }

  else {
    if (name.size() == 0)
      throw string("name must begin with alpha or '_'");
  }

  return name;
}


/**
 * Parse the optional integer index for a path component.
 *
 * Index: "[" [0-9]+ "]"
 */
static inline int
parse_index (const string &path, int &i)
{
  int index = 0;

  if (path[i] != '[')
    return 0;
  else
    i++;

  for (int max = path.size(); i < max; i++) {
    if (isdigit(path[i])) {
      index = (index * 10) + (path[i] - '0');
    } else if (path[i] == ']') {
      i++;
      return index;
    } else {
      break;
    }
  }

  throw string("unterminated index (looking for ']')");
}


/**
 * Parse a single path component.
 *
 * Component: Name Index?
 */
static inline PathComponent
parse_component (const string &path, int &i)
{
  PathComponent component;
  component.name = parse_name(path, i);
  if (component.name[0] != '.')
    component.index = parse_index(path, i);
  else
    component.index = -1;
  return component;
}


/**
 * Parse a path into its components.
 */
static void
parse_path (const string &path, vector<PathComponent> &components)
{
  int pos = 0;
  int max = path.size();

				// Check for initial '/'
  if (path[pos] == '/') {
    PathComponent root;
    root.name = "";
    root.index = -1;
    components.push_back(root);
    pos++;
    while (pos < max && path[pos] == '/')
      pos++;
  }

  while (pos < max) {
    components.push_back(parse_component(path, pos));
    while (pos < max && path[pos] == '/')
      pos++;
  }
}



////////////////////////////////////////////////////////////////////////
// Other static utility functions.
////////////////////////////////////////////////////////////////////////


/**
 * Locate a child node by name and index.
 */
static int
find_child (const string &name, int index, vector<SGPropertyNode *> nodes)
{
  int nNodes = nodes.size();
  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (node->getName() == name && node->getIndex() == index)
      return i;
  }
  return -1;
}


/**
 * Locate another node, given a relative path.
 */
static SGPropertyNode *
find_node (SGPropertyNode * current,
	   const vector<PathComponent> &components,
	   int position,
	   bool create)
{
				// Run off the end of the list
  if (current == 0) {
    return 0;
  }

				// Success! This is the one we want.
  else if (position >= (int)components.size()) {
    return current;
  }

				// Empty component means root.
  else if (components[position].name == "") {
    return find_node(current->getRootNode(), components, position + 1, create);
  }

				// . means current directory
  else if (components[position].name == ".") {
    return find_node(current, components, position + 1, create);
  }

				// .. means parent directory
  else if (components[position].name == "..") {
    SGPropertyNode * parent = current->getParent();
    if (parent == 0)
      throw string("Attempt to move past root with '..'");
    else
      return find_node(parent, components, position + 1, create);
  }

				// Otherwise, a child name
  else {
    SGPropertyNode * child =
      current->getChild(components[position].name,
			components[position].index,
			create);
    return find_node(child, components, position + 1, create);
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGValue.
////////////////////////////////////////////////////////////////////////


/**
 * Default constructor.
 *
 * The type will be UNKNOWN and the raw value will be "".
 */
SGValue::SGValue ()
  : _type(UNKNOWN), _tied(false)
{
  _value.string_val = new SGRawValueInternal<string>;
}


/**
 * Copy constructor.
 */
SGValue::SGValue (const SGValue &source)
{
  _type = source._type;
  _tied = source._tied;
  switch (source._type) {
  case ALIAS:
				// FIXME!!!
    _value.alias = (SGValue *)(source.getAlias());
    break;
  case BOOL:
    _value.bool_val = source._value.bool_val->clone();
    break;
  case INT:
    _value.int_val = source._value.int_val->clone();
    break;
  case LONG:
    _value.long_val = source._value.long_val->clone();
    break;
  case FLOAT:
    _value.float_val = source._value.float_val->clone();
    break;
  case DOUBLE:
    _value.double_val = source._value.double_val->clone();
    break;
  case STRING:
  case UNKNOWN:
    _value.string_val = source._value.string_val->clone();
    break;
  }
}


/**
 * Destructor.
 */
SGValue::~SGValue ()
{
  if (_type != ALIAS)
    clear_value();
}


/**
 * Delete and clear the current value.
 */
void
SGValue::clear_value ()
{
  switch (_type) {
  case ALIAS:
    _value.alias->clear_value();
  case BOOL:
    delete _value.bool_val;
    _value.bool_val = 0;
    break;
  case INT:
    delete _value.int_val;
    _value.int_val = 0;
    break;
  case LONG:
    delete _value.long_val;
    _value.long_val = 0L;
    break;
  case FLOAT:
    delete _value.float_val;
    _value.float_val = 0;
    break;
  case DOUBLE:
    delete _value.double_val;
    _value.double_val = 0;
    break;
  case STRING:
  case UNKNOWN:
    delete _value.string_val;
    _value.string_val = 0;
    break;
  }
}


/**
 * Get the current type.
 *
 * Does not return a type of ALIAS.
 */
SGValue::Type
SGValue::getType () const
{
  if (_type == ALIAS)
    return _value.alias->getType();
  else
    return (Type)_type;
}


/**
 * Get the current aliased value.
 */
SGValue *
SGValue::getAlias ()
{
  return (_type == ALIAS ? _value.alias : 0);
}


/**
 * Get the current aliased value.
 */
const SGValue *
SGValue::getAlias () const
{
  return (_type == ALIAS ? _value.alias : 0);
}


/**
 * Alias to another value.
 */
bool
SGValue::alias (SGValue * alias)
{
  if (alias == 0 || _type == ALIAS || _tied)
    return false;
  clear_value();
  _value.alias = alias;
  _type = ALIAS;
  return true;
}


/**
 * Unalias from another value.
 */
bool
SGValue::unalias ()
{
				// FIXME: keep copy of previous value,
				// as with untie()
  if (_type != ALIAS)
    return false;
  _value.string_val = new SGRawValueInternal<string>;
  _type = UNKNOWN;
  return true;
}


/**
 * Get a boolean value.
 */
bool
SGValue::getBoolValue () const
{
  switch (_type) {
  case ALIAS:
    return _value.alias->getBoolValue();
  case BOOL:
    return GET_BOOL;
  case INT:
    return GET_INT == 0 ? false : true;
  case LONG:
    return GET_LONG == 0L ? false : true;
  case FLOAT:
    return GET_FLOAT == 0.0 ? false : true;
  case DOUBLE:
    return GET_DOUBLE == 0.0L ? false : true;
  case STRING:
  case UNKNOWN:
    return (GET_STRING == "true" || getDoubleValue() != 0.0L);
  }

  return false;
}


/**
 * Get an integer value.
 */
int
SGValue::getIntValue () const
{
  switch (_type) {
  case ALIAS:
    return _value.alias->getIntValue();
  case BOOL:
    return int(GET_BOOL);
  case INT:
    return GET_INT;
  case LONG:
    return int(GET_LONG);
  case FLOAT:
    return int(GET_FLOAT);
  case DOUBLE:
    return int(GET_DOUBLE);
  case STRING:
  case UNKNOWN:
    return atoi(GET_STRING.c_str());
  }

  return 0;
}


/**
 * Get a long integer value.
 */
long
SGValue::getLongValue () const
{
  switch (_type) {
  case ALIAS:
    return _value.alias->getLongValue();
  case BOOL:
    return long(GET_BOOL);
  case INT:
    return long(GET_INT);
  case LONG:
    return GET_LONG;
  case FLOAT:
    return long(GET_FLOAT);
  case DOUBLE:
    return long(GET_DOUBLE);
  case STRING:
  case UNKNOWN:
    return strtol(GET_STRING.c_str(), 0, 0);
  }
}


/**
 * Get a float value.
 */
float
SGValue::getFloatValue () const
{
  switch (_type) {
  case ALIAS:
    return _value.alias->getFloatValue();
  case BOOL:
    return float(GET_BOOL);
  case INT:
    return float(GET_INT);
  case LONG:
    return float(GET_LONG);
  case FLOAT:
    return GET_FLOAT;
  case DOUBLE:
    return float(GET_DOUBLE);
  case STRING:
  case UNKNOWN:
    return atof(GET_STRING.c_str());
  }

  return 0.0;
}


/**
 * Get a double value.
 */
double
SGValue::getDoubleValue () const
{
  switch (_type) {
  case ALIAS:
    return _value.alias->getDoubleValue();
  case BOOL:
    return double(GET_BOOL);
  case INT:
    return double(GET_INT);
  case LONG:
    return double(GET_LONG);
  case FLOAT:
    return double(GET_FLOAT);
  case DOUBLE:
    return GET_DOUBLE;
  case STRING:
  case UNKNOWN:
    return strtod(GET_STRING.c_str(), 0);
  }

  return 0.0;
}


/**
 * Get a string value.
 */
string
SGValue::getStringValue () const
{
  char buf[128];

  switch (_type) {
  case ALIAS:
    return _value.alias->getStringValue();
  case BOOL:
    if (GET_BOOL)
      return "true";
    else
      return "false";
  case INT:
    sprintf(buf, "%d", GET_INT);
    return buf;
  case LONG:
    sprintf(buf, "%ld", GET_LONG);
    return buf;
  case FLOAT:
    sprintf(buf, "%f", GET_FLOAT);
    return buf;
  case DOUBLE:
    sprintf(buf, "%f", GET_DOUBLE);
    return buf;
  case STRING:
  case UNKNOWN:
    return GET_STRING;
  }

  return "";
}


/**
 * Set a bool value.
 */
bool
SGValue::setBoolValue (bool value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.bool_val = new SGRawValueInternal<bool>;
    _type = BOOL;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setBoolValue(value);
  case BOOL:
    return SET_BOOL(value);
  case INT:
    return SET_INT(int(value));
  case LONG:
    return SET_LONG(long(value));
  case FLOAT:
    return SET_FLOAT(float(value));
  case DOUBLE:
    return SET_DOUBLE(double(value));
  case STRING:
    return SET_STRING(value ? "true" : "false");
  }

  return false;
}


/**
 * Set an int value.
 */
bool
SGValue::setIntValue (int value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.int_val = new SGRawValueInternal<int>;
    _type = INT;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setIntValue(value);
  case BOOL:
    return SET_BOOL(value == 0 ? false : true);
  case INT:
    return SET_INT(value);
  case LONG:
    return SET_LONG(long(value));
  case FLOAT:
    return SET_FLOAT(float(value));
  case DOUBLE:
    return SET_DOUBLE(double(value));
  case STRING: {
    char buf[128];
    sprintf(buf, "%d", value);
    return SET_STRING(buf);
  }
  }

  return false;
}


/**
 * Set a long value.
 */
bool
SGValue::setLongValue (long value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.long_val = new SGRawValueInternal<long>;
    _type = LONG;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setLongValue(value);
  case BOOL:
    return SET_BOOL(value == 0L ? false : true);
  case INT:
    return SET_INT(int(value));
  case LONG:
    return SET_LONG(value);
  case FLOAT:
    return SET_FLOAT(float(value));
  case DOUBLE:
    return SET_DOUBLE(double(value));
  case STRING: {
    char buf[128];
    sprintf(buf, "%d", value);
    return SET_STRING(buf);
  }
  }

  return false;
}


/**
 * Set a float value.
 */
bool
SGValue::setFloatValue (float value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.float_val = new SGRawValueInternal<float>;
    _type = FLOAT;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setFloatValue(value);
  case BOOL:
    return SET_BOOL(value == 0.0 ? false : true);
  case INT:
    return SET_INT(int(value));
  case LONG:
    return SET_LONG(long(value));
  case FLOAT:
    return SET_FLOAT(value);
  case DOUBLE:
    return SET_DOUBLE(double(value));
  case STRING: {
    char buf[128];
    sprintf(buf, "%f", value);
    return SET_STRING(buf);
  }
  }

  return false;
}


/**
 * Set a double value.
 */
bool
SGValue::setDoubleValue (double value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.double_val = new SGRawValueInternal<double>;
    _type = DOUBLE;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setDoubleValue(value);
  case BOOL:
    return SET_BOOL(value == 0.0L ? false : true);
  case INT:
    return SET_INT(int(value));
  case LONG:
    return SET_LONG(long(value));
  case FLOAT:
    return SET_FLOAT(float(value));
  case DOUBLE:
    return SET_DOUBLE(value);
  case STRING: {
    char buf[128];
    sprintf(buf, "%lf", value);
    return SET_STRING(buf);
  }
  }

  return false;
}


/**
 * Set a string value.
 */
bool
SGValue::setStringValue (string value)
{
  if (_type == UNKNOWN) {
    clear_value();
    _value.string_val = new SGRawValueInternal<string>;
    _type = STRING;
  }

  switch (_type) {
  case ALIAS:
    return _value.alias->setStringValue(value);
  case BOOL:
    return SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
  case INT:
    return SET_INT(atoi(value.c_str()));
  case LONG:
    return SET_LONG(strtol(value.c_str(), 0, 0));
  case FLOAT:
    return SET_FLOAT(atof(value.c_str()));
  case DOUBLE:
    return SET_DOUBLE(strtod(value.c_str(), 0));
  case STRING:
    return SET_STRING(value);
  }

  return false;
}


/**
 * Set a value of unknown type (stored as a string).
 */
bool
SGValue::setUnknownValue (string value)
{
  switch (_type) {
  case ALIAS:
    return _value.alias->setUnknownValue(value);
  case BOOL:
    return SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
  case INT:
    return SET_INT(atoi(value.c_str()));
  case LONG:
    return SET_LONG(strtol(value.c_str(), 0, 0));
  case FLOAT:
    return SET_FLOAT(atof(value.c_str()));
  case DOUBLE:
    return SET_DOUBLE(strtod(value.c_str(), 0));
  case STRING:
  case UNKNOWN:
    return SET_STRING(value);
  }

  return false;
}


/**
 * Tie a bool value.
 */
bool
SGValue::tie (const SGRawValue<bool> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  bool old_val = false;
  if (use_default)
    old_val = getBoolValue();

  clear_value();
  _type = BOOL;
  _tied = true;
  _value.bool_val = value.clone();

  if (use_default)
    setBoolValue(old_val);

  return true;
}


/**
 * Tie an int value.
 */
bool
SGValue::tie (const SGRawValue<int> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  int old_val = 0;
  if (use_default)
    old_val = getIntValue();

  clear_value();
  _type = INT;
  _tied = true;
  _value.int_val = value.clone();

  if (use_default)
    setIntValue(old_val);

  return true;
}


/**
 * Tie a long value.
 */
bool
SGValue::tie (const SGRawValue<long> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  long old_val;
  if (use_default)
    old_val = getLongValue();

  clear_value();
  _type = LONG;
  _tied = true;
  _value.long_val = value.clone();

  if (use_default)
    setLongValue(old_val);

  return true;
}


/**
 * Tie a float value.
 */
bool
SGValue::tie (const SGRawValue<float> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  float old_val = 0.0;
  if (use_default)
    old_val = getFloatValue();

  clear_value();
  _type = FLOAT;
  _tied = true;
  _value.float_val = value.clone();

  if (use_default)
    setFloatValue(old_val);

  return true;
}


/**
 * Tie a double value.
 */
bool
SGValue::tie (const SGRawValue<double> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  double old_val = 0.0;
  if (use_default)
    old_val = getDoubleValue();

  clear_value();
  _type = DOUBLE;
  _tied = true;
  _value.double_val = value.clone();

  if (use_default)
    setDoubleValue(old_val);

  return true;
}


/**
 * Tie a string value.
 */
bool
SGValue::tie (const SGRawValue<string> &value, bool use_default)
{
  if (_type == ALIAS)
    return _value.alias->tie(value, use_default);
  else if (_tied)
    return false;

  string old_val;
  if (use_default)
    old_val = getStringValue();

  clear_value();
  _type = STRING;
  _tied = true;
  _value.string_val = value.clone();

  if (use_default)
    setStringValue(old_val);

  return true;
}


/**
 * Untie a value.
 */
bool
SGValue::untie ()
{
  if (!_tied)
    return false;

  switch (_type) {
  case ALIAS: {
    return _value.alias->untie();
  }
  case BOOL: {
    bool val = getBoolValue();
    clear_value();
    _value.bool_val = new SGRawValueInternal<bool>;
    SET_BOOL(val);
    break;
  }
  case INT: {
    int val = getIntValue();
    clear_value();
    _value.int_val = new SGRawValueInternal<int>;
    SET_INT(val);
    break;
  }
  case LONG: {
    long val = getLongValue();
    clear_value();
    _value.long_val = new SGRawValueInternal<long>;
    SET_LONG(val);
    break;
  }
  case FLOAT: {
    float val = getFloatValue();
    clear_value();
    _value.float_val = new SGRawValueInternal<float>;
    SET_FLOAT(val);
    break;
  }
  case DOUBLE: {
    double val = getDoubleValue();
    clear_value();
    _value.double_val = new SGRawValueInternal<double>;
    SET_DOUBLE(val);
    break;
  }
  case STRING: {
    string val = getStringValue();
    clear_value();
    _value.string_val = new SGRawValueInternal<string>;
    SET_STRING(val);
    break;
  }
  }

  _tied = false;
  return true;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyNode.
////////////////////////////////////////////////////////////////////////


/**
 * Utility function: given a value, find the property node.
 */
static SGPropertyNode *
find_node_by_value (SGPropertyNode * start_node, const SGValue * value)
{
  if (start_node->getValue() == value) {
    return start_node;
  } else for (int i = 0; i < start_node->nChildren(); i++) {
    SGPropertyNode * child =
      find_node_by_value(start_node->getChild(i), value);
    if (child != 0)
      return child;
  }
  return 0;
}


/**
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _value(0), _name(""), _index(0), _parent(0), _target(0)
{
}


/**
 * Convenience constructor.
 */
SGPropertyNode::SGPropertyNode (const string &name,
				int index, SGPropertyNode * parent)
  : _value(0), _name(name), _index(index), _parent(parent), _target(0)
{
}


/**
 * Destructor.
 */
SGPropertyNode::~SGPropertyNode ()
{
  delete _value;
  for (int i = 0; i < (int)_children.size(); i++)
    delete _children[i];
}


/**
 * Get a value, optionally creating it if not present.
 */
SGValue *
SGPropertyNode::getValue (bool create)
{
  if (_value == 0 && create)
    _value = new SGValue();
  return _value;
}


/**
 * Alias to another node.
 */
bool
SGPropertyNode::alias (SGPropertyNode * target)
{
  if (_value == 0)
    _value = new SGValue();
  _target = target;
  return _value->alias(target->getValue(true));
}


/**
 * Alias to another node by path.
 */
bool
SGPropertyNode::alias (const string &path)
{
  return alias(getNode(path, true));
}


/**
 * Remove an alias.
 */
bool
SGPropertyNode::unalias ()
{
  _target = 0;
  return (_value != 0 ? _value->unalias() : false);
}


/**
 * Test whether this node is aliased.
 */
bool
SGPropertyNode::isAlias () const
{
  return (_value != 0 ? _value->isAlias() : false);
}


/**
 * Get the target of an alias.
 *
 * This is tricky, because it is actually the value that is aliased,
 * and someone could realias or unalias the value directly without
 * going through the property node.  The node caches its best guess,
 * but it may have to search the whole property tree.
 *
 * @return The target node for the alias, or 0 if the node is not aliased.
 */
SGPropertyNode *
SGPropertyNode::getAliasTarget ()
{
  if (_value == 0 || !_value->isAlias()) {
    return 0;
  } else if (_target != 0 && _target->getValue() == _value->getAlias()) {
    return _target;
  } else {
    _target = find_node_by_value(getRootNode(), _value->getAlias());
  }
}


const SGPropertyNode *
SGPropertyNode::getAliasTarget () const
{
  if (_value == 0 || !_value->isAlias()) {
    return 0;
  } else if (_target != 0 && _target->getValue() == _value->getAlias()) {
    return _target;
  } else {
				// FIXME: const cast
    _target =
      find_node_by_value((SGPropertyNode *)getRootNode(), _value->getAlias());
  }
}


/**
 * Get a non-const child by index.
 */
SGPropertyNode *
SGPropertyNode::getChild (int position)
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}


/**
 * Get a const child by index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (int position) const
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}


/**
 * Get a non-const child by name and index, creating if necessary.
 */
SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index, bool create)
{
  int pos = find_child(name, index, _children);
  if (pos >= 0) {
    return _children[pos];
  } else if (create) {
    _children.push_back(new SGPropertyNode(name, index, this));
    return _children[_children.size()-1];
  } else {
    return 0;
  }
}


/**
 * Get a const child by name and index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index) const
{
  int pos = find_child(name, index, _children);
  if (pos >= 0)
    return _children[pos];
  else
    return 0;
}


/**
 * Get all children with the same name (but different indices).
 */
vector<SGPropertyNode *>
SGPropertyNode::getChildren (const string &name)
{
  vector<SGPropertyNode *> children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (_children[i]->getName() == name)
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


/**
 * Get all children const with the same name (but different indices).
 */
vector<const SGPropertyNode *>
SGPropertyNode::getChildren (const string &name) const
{
  vector<const SGPropertyNode *> children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (_children[i]->getName() == name)
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


string
SGPropertyNode::getPath (bool simplify) const
{
  if (_parent == 0)
    return "";

  string path = _parent->getPath(simplify);
  path += '/';
  path += _name;
  if (_index != 0 || !simplify) {
    char buffer[128];
    sprintf(buffer, "[%d]", _index);
    path += buffer;
  }
  return path;
}

SGValue::Type
SGPropertyNode::getType () const
{
  if (_value != 0)
    return _value->getType();
  else
    return SGValue::UNKNOWN;
}


bool 
SGPropertyNode::getBoolValue () const
{
  return (_value == 0 ? SGRawValue<bool>::DefaultValue
	  : _value->getBoolValue());
}

int 
SGPropertyNode::getIntValue () const
{
  return (_value == 0 ? SGRawValue<int>::DefaultValue
	  : _value->getIntValue());
}

long 
SGPropertyNode::getLongValue () const
{
  return (_value == 0 ? SGRawValue<long>::DefaultValue
	  : _value->getLongValue());
}

float 
SGPropertyNode::getFloatValue () const
{
  return (_value == 0 ? SGRawValue<float>::DefaultValue
	  : _value->getFloatValue());
}

double 
SGPropertyNode::getDoubleValue () const
{
  return (_value == 0 ? SGRawValue<double>::DefaultValue
	  : _value->getDoubleValue());
}

string
SGPropertyNode::getStringValue () const
{
  return (_value == 0 ? SGRawValue<string>::DefaultValue
	  : _value->getStringValue());
}

bool
SGPropertyNode::setBoolValue (bool val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setBoolValue(val);
}

bool
SGPropertyNode::setIntValue (int val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setIntValue(val);
}

bool
SGPropertyNode::setLongValue (long val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setLongValue(val);
}

bool
SGPropertyNode::setFloatValue (float val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setFloatValue(val);
}

bool
SGPropertyNode::setDoubleValue (double val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setDoubleValue(val);
}

bool
SGPropertyNode::setStringValue (string val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setStringValue(val);
}

bool
SGPropertyNode::setUnknownValue (string val)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->setUnknownValue(val);
}

bool
SGPropertyNode::isTied () const
{
  return (_value == 0 ? false : _value->isTied());
}

bool
SGPropertyNode::tie (const SGRawValue<bool> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::tie (const SGRawValue<int> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::tie (const SGRawValue<long> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::tie (const SGRawValue<float> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::tie (const SGRawValue<double> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::tie (const SGRawValue<string> &rawValue, bool useDefault)
{
  if (_value == 0)
    _value = new SGValue();
  return _value->tie(rawValue, useDefault);
}

bool
SGPropertyNode::untie ()
{
  return (_value == 0 ? false : _value->untie());
}

SGPropertyNode *
SGPropertyNode::getRootNode ()
{
  if (_parent == 0)
    return this;
  else
    return _parent->getRootNode();
}

const SGPropertyNode *
SGPropertyNode::getRootNode () const
{
  if (_parent == 0)
    return this;
  else
    return _parent->getRootNode();
}

SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path, bool create)
{
  vector<PathComponent> components;
  parse_path(relative_path, components);
  return find_node(this, components, 0, create);
}

const SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path) const
{
  vector<PathComponent> components;
  parse_path(relative_path, components);
				// FIXME: cast away const
  return find_node((SGPropertyNode *)this, components, 0, false);
}



////////////////////////////////////////////////////////////////////////
// Convenience methods using relative paths.
////////////////////////////////////////////////////////////////////////


/**
 * Test whether another node has a value attached.
 */
bool
SGPropertyNode::hasValue (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->hasValue());
}


/**
 * Get the value for another node.
 */
SGValue *
SGPropertyNode::getValue (const string &relative_path, bool create)
{
  SGPropertyNode * node = getNode(relative_path, create);
  if (node != 0 && !node->hasValue())
    node->setUnknownValue("");
  return (node == 0 ? 0 : node->getValue(create));
}


/**
 * Get the value for another node.
 */
const SGValue *
SGPropertyNode::getValue (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? 0 : node->getValue());
}


/**
 * Get the value type for another node.
 */
SGValue::Type
SGPropertyNode::getType (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? SGValue::UNKNOWN : node->getType());
}


/**
 * Get a bool value for another node.
 */
bool
SGPropertyNode::getBoolValue (const string &relative_path,
			      bool defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getBoolValue());
}


/**
 * Get an int value for another node.
 */
int
SGPropertyNode::getIntValue (const string &relative_path,
			     int defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getIntValue());
}


/**
 * Get a long value for another node.
 */
long
SGPropertyNode::getLongValue (const string &relative_path,
			      long defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getLongValue());
}


/**
 * Get a float value for another node.
 */
float
SGPropertyNode::getFloatValue (const string &relative_path,
			       float defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getFloatValue());
}


/**
 * Get a double value for another node.
 */
double
SGPropertyNode::getDoubleValue (const string &relative_path,
				double defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getDoubleValue());
}


/**
 * Get a string value for another node.
 */
string
SGPropertyNode::getStringValue (const string &relative_path,
				string defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getStringValue());
}


/**
 * Set a bool value for another node.
 */
bool
SGPropertyNode::setBoolValue (const string &relative_path, bool value)
{
  return getNode(relative_path, true)->setBoolValue(value);
}


/**
 * Set an int value for another node.
 */
bool
SGPropertyNode::setIntValue (const string &relative_path, int value)
{
  return getNode(relative_path, true)->setIntValue(value);
}


/**
 * Set a long value for another node.
 */
bool
SGPropertyNode::setLongValue (const string &relative_path, long value)
{
  return getNode(relative_path, true)->setLongValue(value);
}


/**
 * Set a float value for another node.
 */
bool
SGPropertyNode::setFloatValue (const string &relative_path, float value)
{
  return getNode(relative_path, true)->setFloatValue(value);
}


/**
 * Set a double value for another node.
 */
bool
SGPropertyNode::setDoubleValue (const string &relative_path, double value)
{
  return getNode(relative_path, true)->setDoubleValue(value);
}


/**
 * Set a string value for another node.
 */
bool
SGPropertyNode::setStringValue (const string &relative_path, string value)
{
  return getNode(relative_path, true)->setStringValue(value);
}


/**
 * Set an unknown value for another node.
 */
bool
SGPropertyNode::setUnknownValue (const string &relative_path, string value)
{
  return getNode(relative_path, true)->setUnknownValue(value);
}


/**
 * Test whether another node is tied.
 */
bool
SGPropertyNode::isTied (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->isTied());
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<bool> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<int> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<long> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<float> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<double> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<string> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Attempt to untie another node reached by a relative path.
 */
bool
SGPropertyNode::untie (const string &relative_path)
{
  SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->untie());
}

// end of props.cxx
