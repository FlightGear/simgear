// props.cxx -- implementation of SimGear Property Manager.
//
// Written by David Megginson - david@megginson.com
//
// This module is in the PUBLIC DOMAIN.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See props.html for documentation [replace with URL when available].
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>

#include "props.hxx"

#include <stdlib.h>

#include <string>

using std::string;

FGPropertyList current_properties;

static string empty_string;



////////////////////////////////////////////////////////////////////////
// Implementation of FGValue.
////////////////////////////////////////////////////////////////////////


/**
 * Construct a new value.
 */
FGValue::FGValue ()
  : _type(UNKNOWN), _tied(false)
{
}


/**
 * Destroy a value.
 */
FGValue::~FGValue ()
{
}


/**
 * Return a raw boolean value (no type coercion).
 */
bool
FGValue::getRawBool () const
{
  if (_tied) {
    if (_value.bool_func.getter != 0)
      return (*(_value.bool_func.getter))();
    else
      return false;
  } else {
    return _value.bool_val;
  }
}


/**
 * Return a raw integer value (no type coercion).
 */
int
FGValue::getRawInt () const
{
  if (_tied) {
    if (_value.int_func.getter != 0)
      return (*(_value.int_func.getter))();
    else
      return 0;
  } else {
    return _value.int_val;
  }
}


/**
 * Return a raw floating-point value (no type coercion).
 */
float
FGValue::getRawFloat () const
{
  if (_tied) {
    if (_value.float_func.getter != 0)
      return (*(_value.float_func.getter))();
    else
      return 0.0;
  } else {
    return _value.float_val;
  }
}


/**
 * Return a raw double-precision floating-point value (no type coercion).
 */
double
FGValue::getRawDouble () const
{
  if (_tied) {
    if (_value.double_func.getter != 0)
      return (*(_value.double_func.getter))();
    else
      return 0.0L;
  } else {
    return _value.double_val;
  }
}


/**
 * Return a raw string value (no type coercion).
 */
const string &
FGValue::getRawString () const
{
  if (_tied && _value.string_func.getter != 0)
    return (*(_value.string_func.getter))();
  else
    return string_val;
}


/**
 * Set a raw boolean value (no type coercion).
 *
 * Return false if the value could not be set, true otherwise.
 */
bool
FGValue::setRawBool (bool value)
{
  if (_tied) {
    if (_value.bool_func.setter != 0) {
      (*_value.bool_func.setter)(value);
      return true;
    } else {
      return false;
    }
  } else {
    _value.bool_val = value;
    return true;
  }
}


/**
 * Set a raw integer value (no type coercion).
 *
 * Return false if the value could not be set, true otherwise.
 */
bool
FGValue::setRawInt (int value)
{
  if (_tied) {
    if (_value.int_func.setter != 0) {
      (*_value.int_func.setter)(value);
      return true;
    } else {
      return false;
    }
  } else {
    _value.int_val = value;
    return true;
  }
}


/**
 * Set a raw floating-point value (no type coercion).
 *
 * Return false if the value could not be set, true otherwise.
 */
bool
FGValue::setRawFloat (float value)
{
  if (_tied) {
    if (_value.float_func.setter != 0) {
      (*_value.float_func.setter)(value);
      return true;
    } else {
      return false;
    }
  } else {
    _value.float_val = value;
    return true;
  }
}


/**
 * Set a raw double-precision floating-point value (no type coercion).
 *
 * Return false if the value could not be set, true otherwise.
 */
bool
FGValue::setRawDouble (double value)
{
  if (_tied) {
    if (_value.double_func.setter != 0) {
      (*_value.double_func.setter)(value);
      return true;
    } else {
      return false;
    }
  } else {
    _value.double_val = value;
    return true;
  }
}


/**
 * Set a raw string value (no type coercion).
 *
 * Return false if the value could not be set, true otherwise.
 */
bool
FGValue::setRawString (const string &value)
{
  if (_tied) {
    if (_value.string_func.setter != 0) {
      (*_value.string_func.setter)(value);
      return true;
    } else {
      return false;
    }
  } else {
    string_val = value;
    return true;
  }
}


/**
 * Get the boolean value of a property.
 *
 * If the native type is not boolean, attempt to coerce it.
 */
bool
FGValue::getBoolValue () const
{
  switch (_type) {
  case BOOL:
    return getRawBool();
  case INT:
    return (getRawInt() == 0 ? false : true);
  case FLOAT:
    return (getRawFloat() == 0.0 ? false : true);
  case DOUBLE:
    return (getRawDouble() == 0.0 ? false : true);
  case UNKNOWN:
  case STRING:
    return ((getRawString() == "false" || getIntValue() == 0) ? false : true);
  }
  return false;
}


/**
 * Get the integer value of a property.
 *
 * If the native type is not integer, attempt to coerce it.
 */
int
FGValue::getIntValue () const
{
  switch (_type) {
  case BOOL:
    return getRawBool();
  case INT:
    return getRawInt();
  case FLOAT:
    return (int)(getRawFloat());
  case DOUBLE:
    return (int)(getRawDouble());
  case UNKNOWN:
  case STRING:
    return atoi(getRawString().c_str());
  }
  return false;
}


/**
 * Get the floating-point value of a property.
 *
 * If the native type is not float, attempt to coerce it.
 */
float
FGValue::getFloatValue () const
{
  switch (_type) {
  case UNKNOWN:
    return 0.0;
  case BOOL:
    return (float)(getRawBool());
  case INT:
    return (float)(getRawInt());
  case FLOAT:
    return getRawFloat();
  case DOUBLE:
    return (float)(getRawDouble());
  case STRING:
    return (float)atof(getRawString().c_str());
  }
  return false;
}


/**
 * Get the double-precision floating-point value of a property.
 *
 * If the native type is not double, attempt to coerce it.
 */
double
FGValue::getDoubleValue () const
{
  switch (_type) {
  case UNKNOWN:
    return 0.0;
  case BOOL:
    return (double)(getRawBool());
  case INT:
    return (double)(getRawInt());
  case FLOAT:
    return (double)(getRawFloat());
  case DOUBLE:
    return getRawDouble();
  case STRING:
    return atof(getRawString().c_str());
  }
  return false;
}


/**
 * Get the string value of a property.
 *
 * If the native type is not string, attempt to coerce it.
 */
const string &
FGValue::getStringValue () const
{
  char buf[512];
  switch (_type) {
  case UNKNOWN:
    return getRawString();
  case BOOL:
    if (getRawBool())
      string_val = "true";
    else
      string_val = "false";
    return string_val;
  case INT:
    sprintf(buf, "%d", getRawInt());
    string_val = buf;
    return string_val;
  case FLOAT:
    sprintf(buf, "%f", getRawFloat());
    string_val = buf;
    return string_val;
  case DOUBLE:
    sprintf(buf, "%f", getRawDouble());
    string_val = buf;
    return string_val;
  case STRING:
    return getRawString();
  }
  return empty_string;
}


/**
 * Set the boolean value and change the type if unknown.
 *
 * Returns true on success.
 */
bool
FGValue::setBoolValue (bool value)
{
  if (_type == UNKNOWN || _type == BOOL) {
    _type = BOOL;
    return setRawBool(value);
  } else {
    return false;
  }
}


/**
 * Set the integer value and change the type if unknown.
 *
 * Returns true on success.
 */
bool
FGValue::setIntValue (int value)
{
  if (_type == UNKNOWN || _type == INT) {
    _type = INT;
    return setRawInt(value);
  } else {
    return false;
  }
}


/**
 * Set the floating-point value and change the type if unknown.
 *
 * Returns true on success.
 */
bool
FGValue::setFloatValue (float value)
{
  if (_type == UNKNOWN || _type == FLOAT) {
    _type = FLOAT;
    return setRawFloat(value);
  } else {
    return false;
  }
}


/**
 * Set the double-precision value and change the type if unknown.
 *
 * Returns true on success.
 */
bool
FGValue::setDoubleValue (double value)
{
  if (_type == UNKNOWN || _type == DOUBLE) {
    _type = DOUBLE;
    return setRawDouble(value);
  } else {
    return false;
  }
}


/**
 * Set the string value and change the type if unknown.
 *
 * Returns true on success.
 */
bool
FGValue::setStringValue (const string &value)
{
  if (_type == UNKNOWN || _type == STRING) {
    _type = STRING;
    return setRawString(value);
  } else {
    return false;
  }
}


/**
 * Set a string value and don't modify the type.
 *
 * Returns true on success.
 */
bool
FGValue::setUnknownValue (const string &value)
{
  if (_type == UNKNOWN || _type == STRING) {
    return setRawString(value);
  } else {
    return false;
  }
}


/**
 * Tie a boolean value to external functions.
 *
 * If useDefault is true, attempt the assign the current value
 * (if any) after tying the functions.
 *
 * Returns true on success (i.e. the value is not currently tied).
 */
bool
FGValue::tieBool (bool_getter getter, bool_setter setter = 0,
		  bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getBoolValue());
    _tied = true;
    _type = BOOL;
    _value.bool_func.getter = getter;
    _value.bool_func.setter = setter;
    return true;
  }
}


/**
 * Tie an integer value to external functions.
 *
 * If useDefault is true, attempt the assign the current value
 * (if any) after tying the functions.
 *
 * Returns true on success (i.e. the value is not currently tied).
 */
bool
FGValue::tieInt (int_getter getter, int_setter setter = 0,
		 bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getIntValue());
    _tied = true;
    _type = INT;
    _value.int_func.getter = getter;
    _value.int_func.setter = setter;
    return true;
  }
}


/**
 * Tie a floating-point value to external functions.
 *
 * If useDefault is true, attempt the assign the current value
 * (if any) after tying the functions.
 *
 * Returns true on success (i.e. the value is not currently tied).
 */
bool
FGValue::tieFloat (float_getter getter, float_setter setter = 0,
		   bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getFloatValue());
    _tied = true;
    _type = FLOAT;
    _value.float_func.getter = getter;
    _value.float_func.setter = setter;
    return true;
  }
}


/**
 * Tie a double-precision floating-point value to external functions.
 *
 * If useDefault is true, attempt the assign the current value
 * (if any) after tying the functions.
 *
 * Returns true on success (i.e. the value is not currently tied).
 */
bool
FGValue::tieDouble (double_getter getter, double_setter setter = 0,
		    bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getDoubleValue());
    _tied = true;
    _type = DOUBLE;
    _value.double_func.getter = getter;
    _value.double_func.setter = setter;
    return true;
  }
}


/**
 * Tie a string value to external functions.
 *
 * If useDefault is true, attempt the assign the current value
 * (if any) after tying the functions.
 *
 * Returns true on success (i.e. the value is not currently tied).
 */
bool
FGValue::tieString (string_getter getter, string_setter setter = 0,
		    bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getStringValue());
    _tied = true;
    _type = STRING;
    _value.string_func.getter = getter;
    _value.string_func.setter = setter;
    return true;
  }
}


/**
 * Untie a value from external functions.
 *
 * Will always attempt to intialize the internal value from
 * the getter before untying.
 *
 * Returns true on success (i.e. the value had been tied).
 */
bool
FGValue::untie ()
{
  if (!_tied)
    return false;

  switch (_type) {
  case BOOL: {
    bool value = getRawBool();
    _tied = false;
    setRawBool(value);
    break;
  }
  case INT: {
    int value = getRawInt();
    _tied = false;
    setRawInt(value);
    break;
  }
  case FLOAT: {
    float value = getRawFloat();
    _tied = false;
    setRawFloat(value);
    break;
  }
  case DOUBLE: {
    double value = getRawDouble();
    _tied = false;
    setRawDouble(value);
    break;
  }
  case STRING: {
    string value = getRawString();
    _tied = false;
    setRawString(value);
    break;
  }
  }

  return true;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPropertyList.
////////////////////////////////////////////////////////////////////////


/**
 * Constructor.
 */
FGPropertyList::FGPropertyList ()
{
}


/**
 * Destructor.
 */
FGPropertyList::~FGPropertyList ()
{
}


/**
 * Look up the FGValue structure associated with a property.
 *
 * Run some basic validity checks on the property name: it must
 * not be empty, must begin with '/', must never have two '//' in a row,
 * and must not end with '/'.
 */
FGValue *
FGPropertyList::getValue (const string &name, bool create = false)
{
  const_iterator el = _props.find(name);
  if (el == _props.end()) {
    if (!create)
      return 0;
    else {
      FG_LOG(FG_GENERAL, FG_INFO, "Creating new property '" << name << '\'');
      if (name.size() == 0 ||
	  name[0] != '/' ||
	  name[name.size()-1] == '/' ||
	  name.find("//") != string::npos) {
	FG_LOG(FG_GENERAL, FG_ALERT, "Illegal property name: '"
	       << name << '\'');
	return 0;
      }
    }
  }
  return &(_props[name]);
}


/**
 * Look up a const value (never created).
 */
const FGValue *
FGPropertyList::getValue (const string &name) const
{
  value_map::const_iterator el = _props.find(name);
  if (el == _props.end())
    return 0;
  else
    return &(el->second);
}


/**
 * Extract a boolean from the value.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to get the FGValue and query it repeatedly.
 */
bool
FGPropertyList::getBoolValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return false;
  else
    return val->getBoolValue();
}


/**
 * Extract an integer from the value.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to get the FGValue and query it repeatedly.
 */
int
FGPropertyList::getIntValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0;
  else
    return val->getIntValue();
}


/**
 * Extract a float from the value.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to get the FGValue and query it repeatedly.
 */
float
FGPropertyList::getFloatValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0.0;
  else
    return val->getFloatValue();
}


/**
 * Extract a double from the value.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to get the FGValue and query it repeatedly.
 */
double
FGPropertyList::getDoubleValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0.0;
  else
    return val->getDoubleValue();
}


/**
 * Extract a string from the value.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and query it repeatedly.
 */
const string &
FGPropertyList::getStringValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return empty_string;
  else
    return val->getStringValue();
}


/**
 * Assign a bool to the value and change the type if unknown.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setBoolValue (const string &name, bool value)
{
  return getValue(name, true)->setBoolValue(value);
}


/**
 * Assign an integer to the value and change the type if unknown.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setIntValue (const string &name, int value)
{
  return getValue(name, true)->setIntValue(value);
}


/**
 * Assign a float to the value and change the type if unknown.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setFloatValue (const string &name, float value)
{
  return getValue(name, true)->setFloatValue(value);
}


/**
 * Assign a double to the value and change the type if unknown.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setDoubleValue (const string &name, double value)
{
  return getValue(name, true)->setDoubleValue(value);
}


/**
 * Assign a string to the value and change the type if unknown.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setStringValue (const string &name, const string &value)
{
  return getValue(name, true)->setStringValue(value);
}


/**
 * Assign a string to the value, but don't change the type.
 *
 * Note that this is inefficient for use in a tight loop: it is
 * better to save the FGValue and modify it repeatedly.
 *
 * Returns true on success.
 */
bool
FGPropertyList::setUnknownValue (const string &name, const string &value)
{
  return getValue(name, true)->setUnknownValue(value);
}


/**
 * Tie a boolean value to external functions.
 *
 * Invokes FGValue::tieBool
 */
bool
FGPropertyList::tieBool (const string &name, 
			 bool_getter getter,
			 bool_setter setter,
			 bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying bool property '" << name << '\'');
  return getValue(name, true)->tieBool(getter, setter, useDefault);
}


/**
 * Tie an integer value to external functions.
 *
 * Invokes FGValue::tieInt
 */
bool
FGPropertyList::tieInt (const string &name, 
			int_getter getter,
			int_setter setter,
			bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying int property '" << name << '\'');
  return getValue(name, true)->tieInt(getter, setter, useDefault);
}


/**
 * Tie a float value to external functions.
 *
 * Invokes FGValue::tieFloat
 */
bool
FGPropertyList::tieFloat (const string &name, 
			  float_getter getter,
			  float_setter setter,
			  bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying float property '" << name << '\'');
  return getValue(name, true)->tieFloat(getter, setter, useDefault);
}


/**
 * Tie a double value to external functions.
 *
 * Invokes FGValue::tieDouble
 */
bool
FGPropertyList::tieDouble (const string &name, 
			   double_getter getter,
			   double_setter setter,
			   bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying double property '" << name << '\'');
  return getValue(name, true)->tieDouble(getter, setter, useDefault);
}


/**
 * Tie a string value to external functions.
 *
 * Invokes FGValue::tieString
 */
bool
FGPropertyList::tieString (const string &name, 
			   string_getter getter,
			   string_setter setter,
			   bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying string property '" << name << '\'');
  return getValue(name, true)->tieString(getter, setter, useDefault);
}


/**
 * Untie a value from external functions.
 *
 * Invokes FGValue::untie
 */
bool
FGPropertyList::untie (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying property '" << name << '\'');
  return getValue(name, true)->untie();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPropertyNode.
////////////////////////////////////////////////////////////////////////


/**
 * Extract the base name of the next level down from the parent.
 *
 * The parent must have a '/' appended.  Note that basename may
 * be modified even if the test fails.
 */
static bool
get_base (const string &parent, const string &child,
	     string &basename)
{
				// First, check that the parent name
				// is a prefix of the child name, and
				// extract the remainder
  if (child.find(parent) != 0)
    return false;

  basename = child.substr(parent.size());

  string::size_type pos = basename.find('/');
  if (pos != string::npos) {
    basename.resize(pos);
  }

  if (basename.size() == 0)
    return false;
  else
    return true;
}


/**
 * Constructor.
 */
FGPropertyNode::FGPropertyNode (const string &path = "",
				FGPropertyList * props = 0)
  : _props(props)
{
  setPath(path);
}


/**
 * Destructor.
 */
FGPropertyNode::~FGPropertyNode ()
{
}


/**
 * Set the path.
 *
 * Strip the trailing '/', if any.
 */
void
FGPropertyNode::setPath (const string &path)
{
  _path = path;

				// Chop the final '/', if present.
  if (_path.size() > 0 && _path[_path.size()-1] == '/')
    _path.resize(_path.size()-1);
}


/**
 * Return the local name of the property.
 *
 * The local name is just everything after the last slash.
 */
const string &
FGPropertyNode::getName () const
{
  string::size_type pos = _path.rfind('/');
  if (pos != string::npos) {
    _name = _path.substr(pos+1);
    return _name;
  } else {
    return empty_string;
  }
}


/**
 * Return the value of the current node.
 *
 * Currently, this does a lookup each time, but we could cache the
 * value safely as long as it's non-zero.
 *
 * Note that this will not create the value if it doesn't already exist.
 */
FGValue *
FGPropertyNode::getValue ()
{
  if (_props == 0 || _path.size() == 0)
    return 0;
  else
    return _props->getValue(_path);
}


/**
 * Return the number of children for the current node.
 */
int
FGPropertyNode::size () const
{
  if (_props == 0)
    return 0;

  int s = 0;

  string base;
  string lastBase;
  string pattern = _path;
  pattern += '/';

  FGPropertyList::const_iterator it = _props->begin();
  FGPropertyList::const_iterator end = _props->end();
  while (it != end) {
    if (get_base(pattern, it->first, base) && base != lastBase) {
      s++;
      lastBase = base;
    }
    it++;
  }

  return s;
}


/**
 * Initialize a node to represent this node's parent.
 *
 * A return value of true means success; otherwise, the node supplied
 * is unmodified.
 */
bool
FGPropertyNode::getParent (FGPropertyNode &parent) const
{
  string::size_type pos = _path.rfind('/');
  if (pos != string::npos) {
    parent.setPath(_path.substr(0, pos-1));
    parent.setPropertyList(_props);
    return true;
  } else {
    return false;
  }
}


/**
 * Initialize a node to represent this node's nth child.
 *
 * A return value of true means success; otherwise, the node supplied
 * is unmodified.
 */
bool
FGPropertyNode::getChild (FGPropertyNode &child, int n) const
{
  if (_props == 0)
    return false;

  int s = 0;
  string base;
  string lastBase;
  string pattern = _path;
  pattern += '/';

  FGPropertyList::const_iterator it = _props->begin();
  FGPropertyList::const_iterator end = _props->end();
  while (it != end) {
    if (get_base(pattern, it->first, base) && base != lastBase) {
      if (s == n) {
	string path = _path;
	path += '/';
	path += base;
	child.setPath(path);
	child.setPropertyList(_props);
	return true;
      } else {
	s++;
	lastBase = base;
      }
    }
    it++;
  }

  return false;
}

// end of props.cxx
