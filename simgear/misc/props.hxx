// props.hxx -- declaration of SimGear Property Manager.
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

#ifndef __PROPS_HXX
#define __PROPS_HXX

#include <stdio.h>

#include <string>
#include <map>
#include <iostream>

using std::string;
using std::map;
using std::istream;
using std::ostream;

#ifdef UNKNOWN
#pragma warn A sloppy coder has defined UNKNOWN as a macro!
#undef UNKNOWN
#endif

#ifdef BOOL
#pragma warn A sloppy coder has defined BOOL as a macro!
#undef BOOL
#endif

#ifdef INT
#pragma warn A sloppy coder has defined INT as a macro!
#undef INT
#endif

#ifdef FLOAT
#pragma warn A sloppy coder has defined FLOAT as a macro!
#undef FLOAT
#endif

#ifdef DOUBLE
#pragma warn A sloppy coder has defined DOUBLE as a macro!
#undef DOUBLE
#endif

#ifdef STRING
#pragma warn A sloppy coder has defined STRING as a macro!
#undef STRING
#endif



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
class SGValue
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

  SGValue ();
  virtual ~SGValue ();

				// Meta information.
  virtual Type getType () const { return _type; }
  virtual bool isTied () const { return _tied; }

				// Accessors.
  virtual bool getBoolValue () const;
  virtual int getIntValue () const;
  virtual float getFloatValue () const;
  virtual double getDoubleValue () const;
  virtual const string & getStringValue () const;

				// Setters.
  virtual bool setBoolValue (bool value);
  virtual bool setIntValue (int value);
  virtual bool setFloatValue (float value);
  virtual bool setDoubleValue (double value);
  virtual bool setStringValue (const string &value);
  virtual bool setUnknownValue (const string &value);

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
  virtual bool untie ();

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

  mutable string string_val;

				// The value is one of the following...
  union {

    bool bool_val;
    int int_val;
    float float_val;
    double double_val;

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
 * i.e. "/foo/bar/hack") with SGValue classes.  Once an SGValue
 * object is associated with the name, the association is
 * permanent -- it is safe to keep a pointer or reference.
 * however, that the type of a value may change if it is tied
 * to a variable.
 *
 * When iterating through the list, the value type is
 *
 *   pair<string,SGValue>
 *
 * To get the name from a const_iterator, use
 *
 *   it->first
 *
 * and to get the value from a const_iterator, use
 *
 *   it->second
 */
class SGPropertyList
{
public:
  typedef map<string, SGValue> value_map;

  typedef SGValue::bool_getter bool_getter;
  typedef SGValue::int_getter int_getter;
  typedef SGValue::float_getter float_getter;
  typedef SGValue::double_getter double_getter;
  typedef SGValue::string_getter string_getter;

  typedef SGValue::bool_setter bool_setter;
  typedef SGValue::int_setter int_setter;
  typedef SGValue::float_setter float_setter;
  typedef SGValue::double_setter double_setter;
  typedef SGValue::string_setter string_setter;

  typedef value_map::value_type value_type;
  typedef value_map::size_type size_type;
  typedef value_map::const_iterator const_iterator;

  SGPropertyList ();
  virtual ~SGPropertyList ();

  virtual size_type size () const { return _props.size(); }

  virtual const_iterator begin () const { return _props.begin(); }
  virtual const_iterator end () const { return _props.end(); }

  virtual SGValue * getValue (const string &name, bool create = false);
  virtual const SGValue * getValue (const string &name) const;

  virtual bool getBoolValue (const string &name,
			     bool defaultValue = false) const;
  virtual int getIntValue (const string &name,
			   int defaultValue = 0) const;
  virtual float getFloatValue (const string &name,
			       float defaultValue = 0.0) const;
  virtual double getDoubleValue (const string &name,
				 double defaultValue = 0.0L) const;
  virtual const string & getStringValue (const string &name,
					 const string &defaultValue = "")
    const;

  virtual bool setBoolValue (const string &name, bool value);
  virtual bool setIntValue (const string &name, int value);
  virtual bool setFloatValue (const string &name, float value);
  virtual bool setDoubleValue (const string &name, double value);
  virtual bool setStringValue (const string &name, const string &value);
  virtual bool setUnknownValue (const string &name, const string &value);

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

  virtual bool untie (const string &name);

private:
  value_map _props;
};



////////////////////////////////////////////////////////////////////////
// Tree/node/directory view.
////////////////////////////////////////////////////////////////////////


/**
 * Tree view of a property list.
 *
 * This class provides a virtual tree view of a property list, without
 * actually constructing a tree -- the view always stays in sync with
 * the property list itself.
 *
 * This class is designed to be used for setup and configuration; it is
 * optimized for ease of use rather than performance, and shouldn't be
 * used inside a tight loop.
 *
 * Every node is actually just a path together with a pointer to
 * the real property list and a few convenient operations; to the
 * user, however, it looks like a node in a tree or a file system,
 * with the regular operations such as getChild and getParent.
 *
 * Note that a node may be both a branch and a leaf -- that is, it
 * may have a value itself and it may have children.  Here is a simple
 * example that prints the names of all of the different nodes inside 
 * "/controls":
 *
 *   SGPropertyNode controls("/controls", current_property_list);
 *   SGPropertyNode child;
 *   int size = controls.size();
 *   for (int i = 0; i < size; i++) {
 *     if (controls.getChild(child, i))
 *       cout << child.getName() << endl;
 *     else
 *       cerr << "Failed to read child " << i << endl;
 *   }
 */
class SGPropertyNode
{
public:
				// Constructor and destructor
  SGPropertyNode (const string &path = "", SGPropertyList * props = 0);
  virtual ~SGPropertyNode ();

				// Accessor and setter for the internal
				// path.
  virtual const string &getPath () const { return _path; }
  virtual void setPath (const string &path);

				// Accessor and setter for the real
				// property list.
  virtual SGPropertyList * getPropertyList () { return _props; }
  virtual void setPropertyList (SGPropertyList * props) {
    _props = props;
  }

				// Accessors for derived information.
  virtual int size () const;
  virtual const string &getName () const;
  virtual SGPropertyNode &getParent () const;
  virtual SGPropertyNode &getChild (int n) const;
  virtual SGPropertyNode &getSubNode (const string &subpath) const;

				// Get values directly.
  virtual SGValue * getValue (const string &subpath = "");
  virtual bool getBoolValue (const string &subpath = "",
			     bool defaultValue = false) const;
  virtual int getIntValue (const string &subpath = "",
			   int defaultValue = 0) const;
  virtual float getFloatValue (const string &subpath = "",
			       float defaultValue = 0.0) const;
  virtual double getDoubleValue (const string &subpath = "",
				 double defaultValue = 0.0L) const;
  virtual const string &
  getStringValue (const string &subpath = "",
		  const string &defaultValue = "") const;

private:
  string _path;
  SGPropertyList * _props;
				// for pointer persistence...
				// NOT THREAD SAFE!!!
				// (each thread must have its own node
				// object)
  mutable string _name;
  mutable SGPropertyNode * _node;
};



////////////////////////////////////////////////////////////////////////
// Input and output.
////////////////////////////////////////////////////////////////////////

extern bool readPropertyList (istream &input, SGPropertyList * props);
extern bool readPropertyList (const string &file, SGPropertyList * props);
extern bool writePropertyList (ostream &output, const SGPropertyList * props);
extern bool writePropertyList (const string &file,
			       const SGPropertyList * props);



////////////////////////////////////////////////////////////////////////
// Global property manager.
////////////////////////////////////////////////////////////////////////

extern SGPropertyList current_properties;


#endif __PROPS_HXX

// end of props.hxx
