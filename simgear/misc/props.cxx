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

#if !defined(FG_HAVE_NATIVE_SGI_COMPILERS)
FG_USING_STD(cerr);
FG_USING_STD(endl);
#endif
FG_USING_STD(sort);



////////////////////////////////////////////////////////////////////////
// Convenience macros for value access.
////////////////////////////////////////////////////////////////////////

#define GET_BOOL (_value.bool_val->getValue())
#define GET_INT (_value.int_val->getValue())
#define GET_FLOAT (_value.float_val->getValue())
#define GET_DOUBLE (_value.double_val->getValue())
#define GET_STRING (_value.string_val->getValue())

#define SET_BOOL(val) (_value.bool_val->setValue(val))
#define SET_INT(val) (_value.int_val->setValue(val))
#define SET_FLOAT(val) (_value.float_val->setValue(val))
#define SET_DOUBLE(val) (_value.double_val->setValue(val))
#define SET_STRING(val) (_value.string_val->setValue(val))



////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

const bool SGRawValue<bool>::DefaultValue = false;
const int SGRawValue<int>::DefaultValue = 0;
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
  if (current == 0) {
    return 0;
  }

  else if (position >= components.size()) {
    return current;
  }

  else if (components[position].name == "") {
    return find_node(current->getRootNode(), components, position + 1, create);
  }

  else if (components[position].name == ".") {
    return find_node(current, components, position + 1, create);
  }

  else if (components[position].name == "..") {
    SGPropertyNode * parent = current->getParent();
    if (parent == 0)
      throw string("Attempt to move past root with '..'");
    else
      return find_node(parent, components, position + 1, create);
  }

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
  case BOOL:
    _value.bool_val = source._value.bool_val->clone();
    break;
  case INT:
    _value.int_val = source._value.int_val->clone();
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
  clear_value();
}


/**
 * Delete and clear the current value.
 */
void
SGValue::clear_value ()
{
  switch (_type) {
  case BOOL:
    delete _value.bool_val;
    _value.bool_val = 0;
    break;
  case INT:
    delete _value.int_val;
    _value.int_val = 0;
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
 * Get a boolean value.
 */
bool
SGValue::getBoolValue () const
{
  switch (_type) {
  case BOOL:
    return GET_BOOL;
  case INT:
    return GET_INT == 0 ? false : true;
  case FLOAT:
    return GET_FLOAT == 0.0 ? false : true;
  case DOUBLE:
    return GET_DOUBLE == 0.0L ? false : true;
  case STRING:
  case UNKNOWN:
    return (GET_STRING == "true" || getDoubleValue() != 0.0L);
  }
}


/**
 * Get an integer value.
 */
int
SGValue::getIntValue () const
{
  switch (_type) {
  case BOOL:
    return (int)GET_BOOL;
  case INT:
    return GET_INT;
  case FLOAT:
    return (int)GET_FLOAT;
  case DOUBLE:
    return (int)GET_DOUBLE;
  case STRING:
  case UNKNOWN:
    return atoi(GET_STRING.c_str());
  }
}


/**
 * Get a float value.
 */
float
SGValue::getFloatValue () const
{
  switch (_type) {
  case BOOL:
    return (float)GET_BOOL;
  case INT:
    return (float)GET_INT;
  case FLOAT:
    return GET_FLOAT;
  case DOUBLE:
    return GET_DOUBLE;
  case STRING:
  case UNKNOWN:
    return atof(GET_STRING.c_str());
  }
}


/**
 * Get a double value.
 */
double
SGValue::getDoubleValue () const
{
  switch (_type) {
  case BOOL:
    return (double)GET_BOOL;
  case INT:
    return (double)GET_INT;
  case FLOAT:
    return (double)GET_FLOAT;
  case DOUBLE:
    return GET_DOUBLE;
  case STRING:
  case UNKNOWN:
    return (double)atof(GET_STRING.c_str());
  }
}


/**
 * Get a string value.
 */
string
SGValue::getStringValue () const
{
  char buf[128];

  switch (_type) {
  case BOOL:
    if (GET_BOOL)
      return "true";
    else
      return "false";
  case INT:
    sprintf(buf, "%d", GET_INT);
    return buf;
  case FLOAT:
    sprintf(buf, "%f", GET_FLOAT);
    return buf;
  case DOUBLE:
    sprintf(buf, "%lf", GET_DOUBLE);
    return buf;
  case STRING:
  case UNKNOWN:
    return GET_STRING;
  }
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
  case BOOL:
    return SET_BOOL(value);
  case INT:
    return SET_INT((int)value);
  case FLOAT:
    return SET_FLOAT((float)value);
  case DOUBLE:
    return SET_DOUBLE((double)value);
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
  case BOOL:
    return SET_BOOL(value == 0 ? false : true);
  case INT:
    return SET_INT(value);
  case FLOAT:
    return SET_FLOAT((float)value);
  case DOUBLE:
    return SET_DOUBLE((double)value);
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
  case BOOL:
    return SET_BOOL(value == 0.0 ? false : true);
  case INT:
    return SET_INT((int)value);
  case FLOAT:
    return SET_FLOAT(value);
  case DOUBLE:
    return SET_DOUBLE((double)value);
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
  case BOOL:
    return SET_BOOL(value == 0.0L ? false : true);
  case INT:
    return SET_INT((int)value);
  case FLOAT:
    return SET_FLOAT((float)value);
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
  case BOOL:
    return SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
  case INT:
    return SET_INT(atoi(value.c_str()));
  case FLOAT:
    return SET_FLOAT(atof(value.c_str()));
  case DOUBLE:
    return SET_DOUBLE((double)atof(value.c_str()));
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
  case BOOL:
    return SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
  case INT:
    return SET_INT(atoi(value.c_str()));
  case FLOAT:
    return SET_FLOAT(atof(value.c_str()));
  case DOUBLE:
    return SET_DOUBLE((double)atof(value.c_str()));
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
  if (_tied)
    return false;

  bool old_val;
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
  if (_tied)
    return false;

  int old_val;
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
 * Tie a float value.
 */
bool
SGValue::tie (const SGRawValue<float> &value, bool use_default)
{
  if (_tied)
    return false;

  float old_val;
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
  if (_tied)
    return false;

  double old_val;
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
  if (_tied)
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
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _value(0), _name(""), _index(0), _parent(0)
{
}


/**
 * Convenience constructor.
 */
SGPropertyNode::SGPropertyNode (const string &name,
				int index, SGPropertyNode * parent)
  : _value(0), _name(name), _index(index), _parent(parent)
{
}

SGPropertyNode::~SGPropertyNode ()
{
  delete _value;
  for (int i = 0; i < _children.size(); i++)
    delete _children[i];
}

SGPropertyNode *
SGPropertyNode::getChild (int position)
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}

const SGPropertyNode *
SGPropertyNode::getChild (int position) const
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}

SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index, bool create)
{
  int pos = find_child(name, index, _children);
  if (pos >= 0) {
    return getChild(pos);
  } else if (create) {
    _children.push_back(new SGPropertyNode(name, index, this));
    return _children[_children.size()-1];
  } else {
    return 0;
  }
}

const SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index) const
{
  int pos = find_child(name, index, _children);
  if (pos >= 0)
    _children[_children.size()-1];
  else
    return 0;
}


class CompareIndices
{
public:
  int operator() (const SGPropertyNode * n1, const SGPropertyNode *n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};


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
 * Get all children with the same name (but different indices).
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
  return (node == 0 ? 0 : node->getValue());
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
