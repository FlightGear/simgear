// props.cxx -- implementation of FGFS global properties.
//
// Copyright (C) 2000  David Megginson - david@megginson.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
  if (!_tied && _type == STRING) {
    delete _value.string_val;
    _value.string_val = 0;
  }
}


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


const string &
FGValue::getRawString () const
{
  if (_tied) {
    if (_value.string_func.getter != 0)
      return (*(_value.string_func.getter))();
    else
      return empty_string;
  } else {
    return *_value.string_val;
  }
}


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
    if (_value.string_val == 0)
      _value.string_val = new string;
    *(_value.string_val) = value;
    return true;
  }
}


/**
 * Attempt to get the boolean value of a property.
 */
bool
FGValue::getBoolValue () const
{
  switch (_type) {
  case UNKNOWN:
    return false;
  case BOOL:
    return getRawBool();
  case INT:
    return (getRawInt() == 0 ? false : true);
  case FLOAT:
    return (getRawFloat() == 0.0 ? false : true);
  case DOUBLE:
    return (getRawDouble() == 0.0 ? false : true);
  case STRING:
    return (getRawString() == "false" ? false : true);
  }

  return false;
}


/**
 * Attempt to get the integer value of a property.
 */
int
FGValue::getIntValue () const
{
  switch (_type) {
  case UNKNOWN:
    return 0;
  case BOOL:
    return getRawBool();
  case INT:
    return getRawInt();
  case FLOAT:
    return (int)(getRawFloat());
  case DOUBLE:
    return (int)(getRawDouble());
  case STRING:
    return atoi(getRawString().c_str());
  }

  return 0;
}


/**
 * Attempt to get the float value of a property.
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

  return 0.0;
}


/**
 * Attempt to get the double value of a property.
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

  return 0.0;
}


/**
 * Attempt to get the string value of a property.
 */
const string &
FGValue::getStringValue () const
{
  switch (_type) {
  case UNKNOWN:
  case BOOL:
  case INT:
  case FLOAT:
  case DOUBLE:
    return empty_string;
  case STRING:
    return getRawString();
  }

  return empty_string;
}


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


bool
FGValue::tieString (string_getter getter, string_setter setter = 0,
		    bool useDefault = true)
{
  if (_tied) {
    return false;
  } else {
    if (useDefault && setter && _type != UNKNOWN)
      (*setter)(getStringValue());
    if (_type == STRING)
      delete _value.string_val;
    _tied = true;
    _type = STRING;
    _value.string_func.getter = getter;
    _value.string_func.setter = setter;
    return true;
  }
}


bool
FGValue::untieBool ()
{
  if (_tied && _type == BOOL) {
    bool value = getRawBool();
    _value.bool_val = value;
    _tied = false;
    return true;
  } else {
    return false;
  }
}


bool
FGValue::untieInt ()
{
  if (_tied && _type == INT) {
    int value = getRawInt();
    _value.int_val = value;
    _tied = false;
    return true;
  } else {
    return false;
  }
}


bool
FGValue::untieFloat ()
{
  if (_tied && _type == FLOAT) {
    float value = getRawFloat();
    _value.float_val = value;
    _tied = false;
    return true;
  } else {
    return false;
  }
}


bool
FGValue::untieDouble ()
{
  if (_tied && _type == DOUBLE) {
    double value = getRawDouble();
    _value.double_val = value;
    _tied = false;
    return true;
  } else {
    return false;
  }
}


bool
FGValue::untieString ()
{
  if (_tied && _type == STRING) {
    const string &value = getRawString();
    _value.string_val = new string(value);
    _tied = false;
    return true;
  } else {
    return false;
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPropertyList.
////////////////////////////////////////////////////////////////////////

FGPropertyList::FGPropertyList ()
{
}

FGPropertyList::~FGPropertyList ()
{
}


FGValue *
FGPropertyList::getValue (const string &name, bool create = false)
{
  const_iterator el = _props.find(name);
  if (el == _props.end()) {
    if (!create)
      return 0;
    else {
      FG_LOG(FG_GENERAL, FG_INFO, "Creating new property '" << name << '\'');
    }
  }
  return &(_props[name]);
}


const FGValue *
FGPropertyList::getValue (const string &name) const
{
  value_map::const_iterator el = _props.find(name);
  return &(el->second);
}


bool
FGPropertyList::getBoolValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return false;
  else
    return val->getBoolValue();
}


int
FGPropertyList::getIntValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0;
  else
    return val->getIntValue();
}


float
FGPropertyList::getFloatValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0.0;
  else
    return val->getFloatValue();
}


double
FGPropertyList::getDoubleValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return 0.0;
  else
    return val->getDoubleValue();
}


const string &
FGPropertyList::getStringValue (const string &name) const
{
  const FGValue * val = getValue(name);
  if (val == 0)
    return empty_string;
  else
    return val->getStringValue();
}


bool
FGPropertyList::setBoolValue (const string &name, bool value)
{
  return getValue(name, true)->setBoolValue(value);
}


bool
FGPropertyList::setIntValue (const string &name, int value)
{
  return getValue(name, true)->setIntValue(value);
}


bool
FGPropertyList::setFloatValue (const string &name, float value)
{
  return getValue(name, true)->setFloatValue(value);
}


bool
FGPropertyList::setDoubleValue (const string &name, double value)
{
  return getValue(name, true)->setDoubleValue(value);
}


bool
FGPropertyList::setStringValue (const string &name, const string &value)
{
  return getValue(name, true)->setStringValue(value);
}


bool
FGPropertyList::tieBool (const string &name, 
			 bool_getter getter,
			 bool_setter setter,
			 bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying bool property '" << name << '\'');
  return getValue(name, true)->tieBool(getter, setter, useDefault);
}


bool
FGPropertyList::tieInt (const string &name, 
			int_getter getter,
			int_setter setter,
			bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying int property '" << name << '\'');
  return getValue(name, true)->tieInt(getter, setter, useDefault);
}


bool
FGPropertyList::tieFloat (const string &name, 
			  float_getter getter,
			  float_setter setter,
			  bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying float property '" << name << '\'');
  return getValue(name, true)->tieFloat(getter, setter, useDefault);
}


bool
FGPropertyList::tieDouble (const string &name, 
			   double_getter getter,
			   double_setter setter,
			   bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying double property '" << name << '\'');
  return getValue(name, true)->tieDouble(getter, setter, useDefault);
}


bool
FGPropertyList::tieString (const string &name, 
			   string_getter getter,
			   string_setter setter,
			   bool useDefault = true)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Tying string property '" << name << '\'');
  return getValue(name, true)->tieString(getter, setter, useDefault);
}


bool
FGPropertyList::untieBool (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying bool property '" << name << '\'');
  return getValue(name, true)->untieBool();
}


bool
FGPropertyList::untieInt (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying int property '" << name << '\'');
  return getValue(name, true)->untieInt();
}


bool
FGPropertyList::untieFloat (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying float property '" << name << '\'');
  return getValue(name, true)->untieFloat();
}


bool
FGPropertyList::untieDouble (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying double property '" << name << '\'');
  return getValue(name, true)->untieDouble();
}


bool
FGPropertyList::untieString (const string &name)
{
  FG_LOG(FG_GENERAL, FG_INFO, "Untying string property '" << name << '\'');
  return getValue(name, true)->untieString();
}


void
FGPropertyList::dumpToLog () const
{
  const_iterator it = FGPropertyList::begin();
  FG_LOG(FG_GENERAL, FG_INFO, "Begin property list dump...");
  while (it != end()) {
    FG_LOG(FG_GENERAL, FG_INFO, "Property: " << it->first);
    it++;
  }
  FG_LOG(FG_GENERAL, FG_INFO, "...End property list dump");
}



// end of props.cxx
