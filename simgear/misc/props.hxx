// props.hxx -- class to manage global FlightGear properties.
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


#ifndef __PROPS_HXX
#define __PROPS_HXX

#include <string>
#include <map>

using std::string;
using std::map;



////////////////////////////////////////////////////////////////////////
// Values.
////////////////////////////////////////////////////////////////////////

/**
 * Abstract representation of a FlightGear value.
 *
 * This value is designed to be fairly robust -- it can exist without
 * a specified value, it can be any of several types, and it can
 * be tied to an external variable without disrupting any existing
 * pointers or references to the value.  Some basic type conversions
 * are also handled automatically.
 *
 * Values also have attributes that control whether they can be read
 * from, written to, or archived (i.e. saved to disk).
 */
class FGValue
{
public:

				// External getters
  typedef bool (*bool_getter)();
  typedef int (*int_getter)();
  typedef float (*float_getter)();
  typedef double (*double_getter)();
  typedef const string &(*string_getter)();

				// External setters
  typedef void (*bool_setter)(bool);
  typedef void (*int_setter)(int);
  typedef void (*float_setter)(float);
  typedef void (*double_setter)(double);
  typedef void (*string_setter)(const string &);

  enum Type {
    UNKNOWN,			// no value assigned yet
    BOOL,			// boolean
    INT,			// integer
    FLOAT,			// floating point
    DOUBLE,			// double precision
    STRING			// text
  };

  FGValue ();
  virtual ~FGValue ();

				// Meta information.
  virtual Type getType () const { return _type; }
  virtual bool isTied () const { return _tied; }

				// Accessors.
  virtual bool getBoolValue () const;
  virtual int getIntValue () const;
  virtual float getFloatValue () const;
  virtual double getDoubleValue () const;
  virtual const string &getStringValue () const;

				// Setters.
  virtual bool setBoolValue (bool value);
  virtual bool setIntValue (int value);
  virtual bool setFloatValue (float value);
  virtual bool setDoubleValue (double value);
  virtual bool setStringValue (const string &value);

				// Tie to external variables.
  virtual bool tieBool (bool_getter getter,
			bool_setter setter = 0,
			bool useDefault = true);
  virtual bool tieInt (int_getter getter,
		       int_setter setter = 0,
		       bool useDefault = true);
  virtual bool tieFloat (float_getter getter,
			 float_setter setter = 0,
			 bool useDefault = true);
  virtual bool tieDouble (double_getter getter,
			  double_setter setter = 0,
			  bool useDefault = true);
  virtual bool tieString (string_getter getter,
			  string_setter setter = 0,
			  bool useDefault = true);

				// Untie from external variables.
  virtual bool untieBool ();
  virtual bool untieInt ();
  virtual bool untieFloat ();
  virtual bool untieDouble ();
  virtual bool untieString ();

protected:

  bool getRawBool () const;
  int getRawInt () const;
  float getRawFloat () const;
  double getRawDouble () const;
  const string &getRawString () const;

  bool setRawBool (bool value);
  bool setRawInt (int value);
  bool setRawFloat (float value);
  bool setRawDouble (double value);
  bool setRawString (const string & value);

private:

  Type _type;
  bool _tied;

				// The value is one of the following...
  union {

    bool bool_val;
    int int_val;
    float float_val;
    double double_val;
    string * string_val;

    struct {
      bool_setter setter;
      bool_getter getter;
    } bool_func;

    struct {
      int_setter setter;
      int_getter getter;
    } int_func;

    struct {
      void * obj;
      float_setter setter;
      float_getter getter;
    } float_func;

    struct {
      void * obj;
      double_setter setter;
      double_getter getter;
    } double_func;

    struct {
      string_setter setter;
      string_getter getter;
    } string_func;

  } _value;

};



////////////////////////////////////////////////////////////////////////
// Top-level manager.
////////////////////////////////////////////////////////////////////////


/**
 * A list of FlightGear properties.
 *
 * This list associates names (conventional written as paths,
 * i.e. "/foo/bar/hack") with FGValue classes.  Once an FGValue
 * object is associated with the name, the association is
 * permanent -- it is safe to keep a pointer or reference.
 * however, that the type of a value may change if it is tied
 * to a variable.
 *
 * When iterating through the list, the value type is
 *
 *   pair<string,FGValue>
 *
 * To get the name from a const_iterator, use
 *
 *   it->first
 *
 * and to get the value from a const_iterator, use
 *
 *   it->second
 */
class FGPropertyList
{
public:
  typedef map<string, FGValue> value_map;

  typedef FGValue::bool_getter bool_getter;
  typedef FGValue::int_getter int_getter;
  typedef FGValue::float_getter float_getter;
  typedef FGValue::double_getter double_getter;
  typedef FGValue::string_getter string_getter;

  typedef FGValue::bool_setter bool_setter;
  typedef FGValue::int_setter int_setter;
  typedef FGValue::float_setter float_setter;
  typedef FGValue::double_setter double_setter;
  typedef FGValue::string_setter string_setter;

  typedef value_map::value_type value_type;
  typedef value_map::size_type size_type;
  typedef value_map::const_iterator const_iterator;

  FGPropertyList ();
  virtual ~FGPropertyList ();

  virtual size_type size () const { return _props.size(); }

  virtual const_iterator begin () const { return _props.begin(); }
  virtual const_iterator end () const { return _props.end(); }

  virtual FGValue * getValue (const string &name, bool create = false);
  virtual const FGValue * getValue (const string &name) const;

  virtual bool getBoolValue (const string &name) const;
  virtual int getIntValue (const string &name) const;
  virtual float getFloatValue (const string &name) const;
  virtual double getDoubleValue (const string &name) const;
  virtual const string &getStringValue (const string &name) const;

  virtual bool setBoolValue (const string &name, bool value);
  virtual bool setIntValue (const string &name, int value);
  virtual bool setFloatValue (const string &name, float value);
  virtual bool setDoubleValue (const string &name, double value);
  virtual bool setStringValue (const string &name, const string &value);

  virtual bool tieBool (const string &name,
			bool_getter getter,
			bool_setter setter = 0,
			bool useDefault = true);
  virtual bool tieInt (const string &name,
		       int_getter getter,
		       int_setter setter = 0,
		       bool useDefault = true);
  virtual bool tieFloat (const string &name,
			 float_getter getter,
			 float_setter setter = 0,
			 bool useDefault = true);
  virtual bool tieDouble (const string &name,
			  double_getter getter,
			  double_setter setter = 0,
			  bool useDefault = true);
  virtual bool tieString (const string &name,
			  string_getter getter,
			  string_setter setter = 0,
			  bool useDefault = true);

  virtual bool untieBool (const string &name);
  virtual bool untieInt (const string &name);
  virtual bool untieFloat (const string &name);
  virtual bool untieDouble (const string &name);
  virtual bool untieString (const string &name);

  virtual void dumpToLog () const;

private:
  value_map _props;
};



////////////////////////////////////////////////////////////////////////
// Global property manager.
////////////////////////////////////////////////////////////////////////

extern FGPropertyList current_properties;


#endif __PROPS_HXX
