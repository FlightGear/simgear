// props.cxx - implementation of a property list.
// Started Fall 2000 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// See props.html for documentation [replace with URL when available].
//
// $Id$

#include "props.hxx"

#include <algorithm>

#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <string.h>

#include <simgear/math/SGMath.hxx>

#if PROPS_STANDALONE
#include <iostream>
#else

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

#if ( _MSC_VER == 1200 )
// MSVC 6 is buggy, and needs something strange here
using std::vector<SGPropertyNode_ptr>;
using std::vector<SGPropertyChangeListener *>;
using std::vector<SGPropertyNode *>;
#endif
#endif

#if PROPS_STANDALONE
using std::cerr;
#endif
using std::endl;
using std::find;
using std::sort;
using std::string;
using std::vector;
using std::stringstream;

using namespace simgear;
using namespace simgear::props;


////////////////////////////////////////////////////////////////////////
// Local classes.
////////////////////////////////////////////////////////////////////////

/**
 * Comparator class for sorting by index.
 */
class CompareIndices
{
public:
  int operator() (const SGPropertyNode_ptr n1, const SGPropertyNode_ptr n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};



////////////////////////////////////////////////////////////////////////
// Convenience macros for value access.
////////////////////////////////////////////////////////////////////////

#define TEST_READ(dflt) if (!getAttribute(READ)) return dflt
#define TEST_WRITE if (!getAttribute(WRITE)) return false

////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

template<> const bool SGRawValue<bool>::DefaultValue = false;
template<> const int SGRawValue<int>::DefaultValue = 0;
template<> const long SGRawValue<long>::DefaultValue = 0L;
template<> const float SGRawValue<float>::DefaultValue = 0.0;
template<> const double SGRawValue<double>::DefaultValue = 0.0L;
template<> const char * const SGRawValue<const char *>::DefaultValue = "";
template<> const SGVec3d SGRawValue<SGVec3d>::DefaultValue = SGVec3d();
template<> const SGVec4d SGRawValue<SGVec4d>::DefaultValue = SGVec4d();

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
static inline const string
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
      throw string("illegal character after " + name);
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


static char *
copy_string (const char * s)
{
  unsigned long int slen = strlen(s);
  char * copy = new char[slen + 1];

  // the source string length is known so no need to check for '\0'
  // when copying every single character
  memcpy(copy, s, slen);
  *(copy + slen) = '\0';
  return copy;
}

static bool
compare_strings (const char * s1, const char * s2)
{
  return !strncmp(s1, s2, SGPropertyNode::MAX_STRING_LEN);
}

/**
 * Locate a child node by name and index.
 */
static int
find_child (const char * name, int index, const PropertyList& nodes)
{
  int nNodes = nodes.size();
  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];

    // searching for a mathing index is a lot less time consuming than
    // comparing two strings so do that first.
    if (node->getIndex() == index && compare_strings(node->getName(), name))
      return i;
  }
  return -1;
}

/**
 * Locate the child node with the highest index of the same name
 */
static int
find_last_child (const char * name, const PropertyList& nodes)
{
  int nNodes = nodes.size();
  int index = 0;

  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (compare_strings(node->getName(), name))
    {
      int idx = node->getIndex();
      if (idx > index) index = idx;
    }
  }
  return index;
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
    return (current->getAttribute(SGPropertyNode::REMOVED) ? 0 : current);
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
      throw string("attempt to move past root with '..'");
    else
      return find_node(parent, components, position + 1, create);
  }

				// Otherwise, a child name
  else {
    SGPropertyNode * child =
      current->getChild(components[position].name.c_str(),
			components[position].index,
			create);
    return find_node(child, components, position + 1, create);
  }
}



////////////////////////////////////////////////////////////////////////
// Private methods from SGPropertyNode (may be inlined for speed).
////////////////////////////////////////////////////////////////////////

inline bool
SGPropertyNode::get_bool () const
{
  if (_tied)
    return static_cast<SGRawValue<bool>*>(_value.val)->getValue();
  else
    return _local_val.bool_val;
}

inline int
SGPropertyNode::get_int () const
{
  if (_tied)
      return (static_cast<SGRawValue<int>*>(_value.val))->getValue();
  else
    return _local_val.int_val;
}

inline long
SGPropertyNode::get_long () const
{
  if (_tied)
    return static_cast<SGRawValue<long>*>(_value.val)->getValue();
  else
    return _local_val.long_val;
}

inline float
SGPropertyNode::get_float () const
{
  if (_tied)
    return static_cast<SGRawValue<float>*>(_value.val)->getValue();
  else
    return _local_val.float_val;
}

inline double
SGPropertyNode::get_double () const
{
  if (_tied)
    return static_cast<SGRawValue<double>*>(_value.val)->getValue();
  else
    return _local_val.double_val;
}

inline const char *
SGPropertyNode::get_string () const
{
  if (_tied)
      return static_cast<SGRawValue<const char*>*>(_value.val)->getValue();
  else
    return _local_val.string_val;
}

inline bool
SGPropertyNode::set_bool (bool val)
{
  if (_tied) {
    if (static_cast<SGRawValue<bool>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.bool_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_int (int val)
{
  if (_tied) {
    if (static_cast<SGRawValue<int>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.int_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_long (long val)
{
  if (_tied) {
    if (static_cast<SGRawValue<long>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.long_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_float (float val)
{
  if (_tied) {
    if (static_cast<SGRawValue<float>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.float_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_double (double val)
{
  if (_tied) {
    if (static_cast<SGRawValue<double>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.double_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_string (const char * val)
{
  if (_tied) {
      if (static_cast<SGRawValue<const char*>*>(_value.val)->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    delete [] _local_val.string_val;
    _local_val.string_val = copy_string(val);
    fireValueChanged();
    return true;
  }
}

void
SGPropertyNode::clearValue ()
{
    if (_type == ALIAS) {
        put(_value.alias);
        _value.alias = 0;
    } else if (_type != NONE) {
        switch (_type) {
        case BOOL:
            _local_val.bool_val = SGRawValue<bool>::DefaultValue;
            break;
        case INT:
            _local_val.int_val = SGRawValue<int>::DefaultValue;
            break;
        case LONG:
            _local_val.long_val = SGRawValue<long>::DefaultValue;
            break;
        case FLOAT:
            _local_val.float_val = SGRawValue<float>::DefaultValue;
            break;
        case DOUBLE:
            _local_val.double_val = SGRawValue<double>::DefaultValue;
            break;
        case STRING:
        case UNSPECIFIED:
            if (!_tied) {
                delete [] _local_val.string_val;
            }
            _local_val.string_val = 0;
            break;
        }
        delete _value.val;
        _value.val = 0;
    }
    _tied = false;
    _type = NONE;
}


/**
 * Get the value as a string.
 */
const char *
SGPropertyNode::make_string () const
{
    if (!getAttribute(READ))
        return "";
    switch (_type) {
    case ALIAS:
        return _value.alias->getStringValue();
    case BOOL:
        return get_bool() ? "true" : "false";
    case STRING:
    case UNSPECIFIED:
        return get_string();
    case NONE:
        return "";
    default:
        break;
    }
    stringstream sstr;
    switch (_type) {
    case INT:
        sstr << get_int();
        break;
    case LONG:
        sstr << get_long();
        break;
    case FLOAT:
        sstr << get_float();
        break;
    case DOUBLE:
        sstr << std::setprecision(10) << get_double();
        break;
    case EXTENDED:
    {
        Type realType = _value.val->getType();
        // Perhaps this should be done for all types?
        if (realType == VEC3D || realType == VEC4D)
            sstr.precision(10);
        static_cast<SGRawExtended*>(_value.val)->printOn(sstr);
    }
        break;
    default:
        return "";
    }
    _buffer = sstr.str();
    return _buffer.c_str();
}

/**
 * Trace a write access for a property.
 */
void
SGPropertyNode::trace_write () const
{
#if PROPS_STANDALONE
  cerr << "TRACE: Write node " << getPath () << ", value \""
       << make_string() << '"' << endl;
#else
  SG_LOG(SG_GENERAL, SG_ALERT, "TRACE: Write node " << getPath()
	 << ", value \"" << make_string() << '"');
#endif
}

/**
 * Trace a read access for a property.
 */
void
SGPropertyNode::trace_read () const
{
#if PROPS_STANDALONE
  cerr << "TRACE: Write node " << getPath () << ", value \""
       << make_string() << '"' << endl;
#else
  SG_LOG(SG_GENERAL, SG_ALERT, "TRACE: Read node " << getPath()
	 << ", value \"" << make_string() << '"');
#endif
}


////////////////////////////////////////////////////////////////////////
// Public methods from SGPropertyNode.
////////////////////////////////////////////////////////////////////////

/**
 * Last used attribute
 * Update as needed when enum Attribute is changed
 */
const int SGPropertyNode::LAST_USED_ATTRIBUTE = USERARCHIVE;

/**
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _index(0),
    _parent(0),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  _local_val.string_val = 0;
  _value.val = 0;
}


/**
 * Copy constructor.
 */
SGPropertyNode::SGPropertyNode (const SGPropertyNode &node)
  : _index(node._index),
    _name(node._name),
    _parent(0),			// don't copy the parent
    _path_cache(0),
    _type(node._type),
    _tied(node._tied),
    _attr(node._attr),
    _listeners(0)		// CHECK!!
{
  _local_val.string_val = 0;
  _value.val = 0;
  if (_type == NONE)
    return;
  if (_type == ALIAS) {
    _value.alias = node._value.alias;
    get(_value.alias);
    _tied = false;
    return;
  }
  if (_tied || _type == EXTENDED) {
    _value.val = node._value.val->clone();
    return;
  }
  switch (_type) {
  case BOOL:
    set_bool(node.get_bool());    
    break;
  case INT:
    set_int(node.get_int());
    break;
  case LONG:
    set_long(node.get_long());
    break;
  case FLOAT:
    set_float(node.get_float());
    break;
  case DOUBLE:
    set_double(node.get_double());
    break;
  case STRING:
  case UNSPECIFIED:
    set_string(node.get_string());
    break;
  default:
    break;
  }
}


/**
 * Convenience constructor.
 */
SGPropertyNode::SGPropertyNode (const char * name,
				int index,
				SGPropertyNode * parent)
  : _index(index),
    _parent(parent),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  int i = 0;
  _local_val.string_val = 0;
  _value.val = 0;
  _name = parse_name(name, i);
  if (i != int(strlen(name)) || name[0] == '.')
    throw string("plain name expected instead of '") + name + '\'';
}


/**
 * Destructor.
 */
SGPropertyNode::~SGPropertyNode ()
{
  // zero out all parent pointers, else they might be dangling
  for (unsigned i = 0; i < _children.size(); ++i)
    _children[i]->_parent = 0;
  for (unsigned i = 0; i < _removedChildren.size(); ++i)
    _removedChildren[i]->_parent = 0;
  delete _path_cache;
  clearValue();

  if (_listeners) {
    vector<SGPropertyChangeListener*>::iterator it;
    for (it = _listeners->begin(); it != _listeners->end(); ++it)
      (*it)->unregister_property(this);
    delete _listeners;
  }
}


/**
 * Alias to another node.
 */
bool
SGPropertyNode::alias (SGPropertyNode * target)
{
  if (target == 0 || _type == ALIAS || _tied)
    return false;
  clearValue();
  get(target);
  _value.alias = target;
  _type = ALIAS;
  return true;
}


/**
 * Alias to another node by path.
 */
bool
SGPropertyNode::alias (const char * path)
{
  return alias(getNode(path, true));
}


/**
 * Remove an alias.
 */
bool
SGPropertyNode::unalias ()
{
  if (_type != ALIAS)
    return false;
  clearValue();
  return true;
}


/**
 * Get the target of an alias.
 */
SGPropertyNode *
SGPropertyNode::getAliasTarget ()
{
  return (_type == ALIAS ? _value.alias : 0);
}


const SGPropertyNode *
SGPropertyNode::getAliasTarget () const
{
  return (_type == ALIAS ? _value.alias : 0);
}

/**
 * create a non-const child by name after the last node with the same name.
 */
SGPropertyNode *
SGPropertyNode::addChild (const char * name)
{
  int pos = find_last_child(name, _children)+1;

  SGPropertyNode_ptr node;
  node = new SGPropertyNode(name, pos, this);
  _children.push_back(node);
  fireChildAdded(node);
  return node;
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
SGPropertyNode::getChild (const char * name, int index, bool create)
{
  int pos = find_child(name, index, _children);
  if (pos >= 0) {
    return _children[pos];
  } else if (create) {
    SGPropertyNode_ptr node;
    pos = find_child(name, index, _removedChildren);
    if (pos >= 0) {
      PropertyList::iterator it = _removedChildren.begin();
      it += pos;
      node = _removedChildren[pos];
      _removedChildren.erase(it);
      node->setAttribute(REMOVED, false);
    } else {
      node = new SGPropertyNode(name, index, this);
    }
    _children.push_back(node);
    fireChildAdded(node);
    return node;
  } else {
    return 0;
  }
}


/**
 * Get a const child by name and index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (const char * name, int index) const
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
PropertyList
SGPropertyNode::getChildren (const char * name) const
{
  PropertyList children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (compare_strings(_children[i]->getName(), name))
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


/**
 * Remove this node and all children from nodes that link to them
 * in their path cache.
 */
void
SGPropertyNode::remove_from_path_caches ()
{
  for (unsigned int i = 0; i < _children.size(); ++i)
    _children[i]->remove_from_path_caches();

  for (unsigned int i = 0; i < _linkedNodes.size(); i++)
    _linkedNodes[i]->erase(this);
  _linkedNodes.clear();
}


/**
 * Remove child by position.
 */
SGPropertyNode_ptr
SGPropertyNode::removeChild (int pos, bool keep)
{
  SGPropertyNode_ptr node;
  if (pos < 0 || pos >= (int)_children.size())
    return node;

  PropertyList::iterator it = _children.begin();
  it += pos;
  node = _children[pos];
  _children.erase(it);
  if (keep) {
    _removedChildren.push_back(node);
  }

  node->remove_from_path_caches();
  node->setAttribute(REMOVED, true);
  node->clearValue();
  fireChildRemoved(node);
  return node;
}


/**
 * Remove a child node
 */
SGPropertyNode_ptr
SGPropertyNode::removeChild (const char * name, int index, bool keep)
{
  SGPropertyNode_ptr ret;
  int pos = find_child(name, index, _children);
  if (pos >= 0)
    ret = removeChild(pos, keep);
  return ret;
}


/**
  * Remove all children with the specified name.
  */
PropertyList
SGPropertyNode::removeChildren (const char * name, bool keep)
{
  PropertyList children;

  for (int pos = _children.size() - 1; pos >= 0; pos--)
    if (compare_strings(_children[pos]->getName(), name))
      children.push_back(removeChild(pos, keep));

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


/**
  * Remove a linked node.
  */
bool
SGPropertyNode::remove_linked_node (hash_table * node)
{
  for (unsigned int i = 0; i < _linkedNodes.size(); i++) {
    if (_linkedNodes[i] == node) {
      vector<hash_table *>::iterator it = _linkedNodes.begin();
      it += i;
      _linkedNodes.erase(it);
      return true;
    }
  }
  return false;
}


string
SGPropertyNode::getDisplayName (bool simplify) const
{
  string display_name = _name;
  if (_index != 0 || !simplify) {
    stringstream sstr;
    sstr << '[' << _index << ']';
    display_name += sstr.str();
  }
  return display_name;
}


const char *
SGPropertyNode::getPath (bool simplify) const
{
  // Calculate the complete path only once.
  if (_parent != 0 && _path.empty()) {
    _path = _parent->getPath(simplify);
    _path += '/';
    _path += getDisplayName(simplify);
  }

  return _path.c_str();
}

Type
SGPropertyNode::getType () const
{
  if (_type == ALIAS)
    return _value.alias->getType();
  else if (_type == EXTENDED)
      return _value.val->getType();
  else
    return _type;
}


bool 
SGPropertyNode::getBoolValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == BOOL)
    return get_bool();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<bool>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getBoolValue();
  case BOOL:
    return get_bool();
  case INT:
    return get_int() == 0 ? false : true;
  case LONG:
    return get_long() == 0L ? false : true;
  case FLOAT:
    return get_float() == 0.0 ? false : true;
  case DOUBLE:
    return get_double() == 0.0L ? false : true;
  case STRING:
  case UNSPECIFIED:
    return (compare_strings(get_string(), "true") || getDoubleValue() != 0.0L);
  case NONE:
  default:
    return SGRawValue<bool>::DefaultValue;
  }
}

int 
SGPropertyNode::getIntValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == INT)
    return get_int();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<int>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getIntValue();
  case BOOL:
    return int(get_bool());
  case INT:
    return get_int();
  case LONG:
    return int(get_long());
  case FLOAT:
    return int(get_float());
  case DOUBLE:
    return int(get_double());
  case STRING:
  case UNSPECIFIED:
    return atoi(get_string());
  case NONE:
  default:
    return SGRawValue<int>::DefaultValue;
  }
}

long 
SGPropertyNode::getLongValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == LONG)
    return get_long();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<long>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getLongValue();
  case BOOL:
    return long(get_bool());
  case INT:
    return long(get_int());
  case LONG:
    return get_long();
  case FLOAT:
    return long(get_float());
  case DOUBLE:
    return long(get_double());
  case STRING:
  case UNSPECIFIED:
    return strtol(get_string(), 0, 0);
  case NONE:
  default:
    return SGRawValue<long>::DefaultValue;
  }
}

float 
SGPropertyNode::getFloatValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == FLOAT)
    return get_float();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<float>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getFloatValue();
  case BOOL:
    return float(get_bool());
  case INT:
    return float(get_int());
  case LONG:
    return float(get_long());
  case FLOAT:
    return get_float();
  case DOUBLE:
    return float(get_double());
  case STRING:
  case UNSPECIFIED:
    return atof(get_string());
  case NONE:
  default:
    return SGRawValue<float>::DefaultValue;
  }
}

double 
SGPropertyNode::getDoubleValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == DOUBLE)
    return get_double();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<double>::DefaultValue;

  switch (_type) {
  case ALIAS:
    return _value.alias->getDoubleValue();
  case BOOL:
    return double(get_bool());
  case INT:
    return double(get_int());
  case LONG:
    return double(get_long());
  case FLOAT:
    return double(get_float());
  case DOUBLE:
    return get_double();
  case STRING:
  case UNSPECIFIED:
    return strtod(get_string(), 0);
  case NONE:
  default:
    return SGRawValue<double>::DefaultValue;
  }
}

const char *
SGPropertyNode::getStringValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == STRING)
    return get_string();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<const char *>::DefaultValue;
  return make_string();
}

bool
SGPropertyNode::setBoolValue (bool value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == BOOL)
    return set_bool(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _tied = false;
    _type = BOOL;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setBoolValue(value);
    break;
  case BOOL:
    result = set_bool(value);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value ? "true" : "false");
    break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setIntValue (int value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == INT)
    return set_int(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = INT;
    _local_val.int_val = 0;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setIntValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0 ? false : true);
    break;
  case INT:
    result = set_int(value);
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%d", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setLongValue (long value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == LONG)
    return set_long(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = LONG;
    _local_val.long_val = 0L;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setLongValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0L ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(value);
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%ld", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setFloatValue (float value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == FLOAT)
    return set_float(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = FLOAT;
    _local_val.float_val = 0;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setFloatValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0.0 ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(value);
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setDoubleValue (double value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == DOUBLE)
    return set_double(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _local_val.double_val = value;
    _type = DOUBLE;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setDoubleValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0.0L ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(value);
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setStringValue (const char * value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == STRING)
    return set_string(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = STRING;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setStringValue(value);
    break;
  case BOOL:
    result = set_bool((compare_strings(value, "true")
		       || atoi(value)) ? true : false);
    break;
  case INT:
    result = set_int(atoi(value));
    break;
  case LONG:
    result = set_long(strtol(value, 0, 0));
    break;
  case FLOAT:
    result = set_float(atof(value));
    break;
  case DOUBLE:
    result = set_double(strtod(value, 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value);
    break;
  case EXTENDED:
  {
    stringstream sstr(value);
    static_cast<SGRawExtended*>(_value.val)->readFrom(sstr);
  }
  break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setUnspecifiedValue (const char * value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE) {
    clearValue();
    _type = UNSPECIFIED;
  }
  Type type = _type;
  if (type == EXTENDED)
      type = _value.val->getType();
  switch (type) {
  case ALIAS:
    result = _value.alias->setUnspecifiedValue(value);
    break;
  case BOOL:
    result = set_bool((compare_strings(value, "true")
		       || atoi(value)) ? true : false);
    break;
  case INT:
    result = set_int(atoi(value));
    break;
  case LONG:
    result = set_long(strtol(value, 0, 0));
    break;
  case FLOAT:
    result = set_float(atof(value));
    break;
  case DOUBLE:
    result = set_double(strtod(value, 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value);
    break;
  case VEC3D:
      result = static_cast<SGRawValue<SGVec3d>*>(_value.val)->setValue(parseString<SGVec3d>(value));
      break;
  case VEC4D:
      result = static_cast<SGRawValue<SGVec4d>*>(_value.val)->setValue(parseString<SGVec4d>(value));
      break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

std::ostream& SGPropertyNode::printOn(std::ostream& stream) const
{
    if (!getAttribute(READ))
        return stream;
    switch (_type) {
    case ALIAS:
        return _value.alias->printOn(stream);
    case BOOL:
        stream << (get_bool() ? "true" : "false");
        break;
    case INT:
        stream << get_int();
        break;
    case LONG:
        stream << get_long();
        break;
    case FLOAT:
        stream << get_float();
        break;
    case DOUBLE:
        stream << get_double();
        break;
    case STRING:
    case UNSPECIFIED:
        stream << get_string();
        break;
    case EXTENDED:
        static_cast<SGRawExtended*>(_value.val)->printOn(stream);
        break;
    case NONE:
        break;
    }
    return stream;
}

template<>
bool SGPropertyNode::tie (const SGRawValue<const char *> &rawValue,
                          bool useDefault)
{
    if (_type == ALIAS || _tied)
        return false;

    useDefault = useDefault && hasValue();
    std::string old_val;
    if (useDefault)
        old_val = getStringValue();
    clearValue();
    _type = STRING;
    _tied = true;
    _value.val = rawValue.clone();

    if (useDefault)
        setStringValue(old_val.c_str());

    return true;
}
bool
SGPropertyNode::untie ()
{
  if (!_tied)
    return false;

  switch (_type) {
  case BOOL: {
    bool val = getBoolValue();
    clearValue();
    _type = BOOL;
    _local_val.bool_val = val;
    break;
  }
  case INT: {
    int val = getIntValue();
    clearValue();
    _type = INT;
    _local_val.int_val = val;
    break;
  }
  case LONG: {
    long val = getLongValue();
    clearValue();
    _type = LONG;
    _local_val.long_val = val;
    break;
  }
  case FLOAT: {
    float val = getFloatValue();
    clearValue();
    _type = FLOAT;
    _local_val.float_val = val;
    break;
  }
  case DOUBLE: {
    double val = getDoubleValue();
    clearValue();
    _type = DOUBLE;
    _local_val.double_val = val;
    break;
  }
  case STRING:
  case UNSPECIFIED: {
    string val = getStringValue();
    clearValue();
    _type = STRING;
    _local_val.string_val = copy_string(val.c_str());
    break;
  }
  case EXTENDED: {
    SGRawExtended* val = static_cast<SGRawExtended*>(_value.val);
    _value.val = 0;             // Prevent clearValue() from deleting
    clearValue();
    _type = EXTENDED;
    _value.val = val->makeContainer();
    delete val;
    break;
  }
  case NONE:
  default:
    break;
  }

  _tied = false;
  return true;
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
SGPropertyNode::getNode (const char * relative_path, bool create)
{
  if (_path_cache == 0)
    _path_cache = new hash_table;

  SGPropertyNode * result = _path_cache->get(relative_path);
  if (result == 0) {
    vector<PathComponent> components;
    parse_path(relative_path, components);
    result = find_node(this, components, 0, create);
    if (result != 0)
      _path_cache->put(relative_path, result);
  }

  return result;
}

SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index, bool create)
{
  vector<PathComponent> components;
  parse_path(relative_path, components);
  if (components.size() > 0)
    components.back().index = index;
  return find_node(this, components, 0, create);
}

const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path) const
{
  return ((SGPropertyNode *)this)->getNode(relative_path, false);
}

const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index) const
{
  return ((SGPropertyNode *)this)->getNode(relative_path, index, false);
}


////////////////////////////////////////////////////////////////////////
// Convenience methods using relative paths.
////////////////////////////////////////////////////////////////////////


/**
 * Test whether another node has a value attached.
 */
bool
SGPropertyNode::hasValue (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->hasValue());
}


/**
 * Get the value type for another node.
 */
Type
SGPropertyNode::getType (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? UNSPECIFIED : (Type)(node->getType()));
}


/**
 * Get a bool value for another node.
 */
bool
SGPropertyNode::getBoolValue (const char * relative_path,
			      bool defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getBoolValue());
}


/**
 * Get an int value for another node.
 */
int
SGPropertyNode::getIntValue (const char * relative_path,
			     int defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getIntValue());
}


/**
 * Get a long value for another node.
 */
long
SGPropertyNode::getLongValue (const char * relative_path,
			      long defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getLongValue());
}


/**
 * Get a float value for another node.
 */
float
SGPropertyNode::getFloatValue (const char * relative_path,
			       float defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getFloatValue());
}


/**
 * Get a double value for another node.
 */
double
SGPropertyNode::getDoubleValue (const char * relative_path,
				double defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getDoubleValue());
}


/**
 * Get a string value for another node.
 */
const char *
SGPropertyNode::getStringValue (const char * relative_path,
				const char * defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getStringValue());
}


/**
 * Set a bool value for another node.
 */
bool
SGPropertyNode::setBoolValue (const char * relative_path, bool value)
{
  return getNode(relative_path, true)->setBoolValue(value);
}


/**
 * Set an int value for another node.
 */
bool
SGPropertyNode::setIntValue (const char * relative_path, int value)
{
  return getNode(relative_path, true)->setIntValue(value);
}


/**
 * Set a long value for another node.
 */
bool
SGPropertyNode::setLongValue (const char * relative_path, long value)
{
  return getNode(relative_path, true)->setLongValue(value);
}


/**
 * Set a float value for another node.
 */
bool
SGPropertyNode::setFloatValue (const char * relative_path, float value)
{
  return getNode(relative_path, true)->setFloatValue(value);
}


/**
 * Set a double value for another node.
 */
bool
SGPropertyNode::setDoubleValue (const char * relative_path, double value)
{
  return getNode(relative_path, true)->setDoubleValue(value);
}


/**
 * Set a string value for another node.
 */
bool
SGPropertyNode::setStringValue (const char * relative_path, const char * value)
{
  return getNode(relative_path, true)->setStringValue(value);
}


/**
 * Set an unknown value for another node.
 */
bool
SGPropertyNode::setUnspecifiedValue (const char * relative_path,
				     const char * value)
{
  return getNode(relative_path, true)->setUnspecifiedValue(value);
}


/**
 * Test whether another node is tied.
 */
bool
SGPropertyNode::isTied (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->isTied());
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<bool> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<int> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<long> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<float> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<double> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<const char *> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Attempt to untie another node reached by a relative path.
 */
bool
SGPropertyNode::untie (const char * relative_path)
{
  SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->untie());
}

void
SGPropertyNode::addChangeListener (SGPropertyChangeListener * listener,
                                   bool initial)
{
  if (_listeners == 0)
    _listeners = new vector<SGPropertyChangeListener*>;
  _listeners->push_back(listener);
  listener->register_property(this);
  if (initial)
    listener->valueChanged(this);
}

void
SGPropertyNode::removeChangeListener (SGPropertyChangeListener * listener)
{
  vector<SGPropertyChangeListener*>::iterator it =
    find(_listeners->begin(), _listeners->end(), listener);
  if (it != _listeners->end()) {
    _listeners->erase(it);
    listener->unregister_property(this);
    if (_listeners->empty()) {
      vector<SGPropertyChangeListener*>* tmp = _listeners;
      _listeners = 0;
      delete tmp;
    }
  }
}

void
SGPropertyNode::fireValueChanged ()
{
  fireValueChanged(this);
}

void
SGPropertyNode::fireChildAdded (SGPropertyNode * child)
{
  fireChildAdded(this, child);
}

void
SGPropertyNode::fireChildRemoved (SGPropertyNode * child)
{
  fireChildRemoved(this, child);
}

void
SGPropertyNode::fireValueChanged (SGPropertyNode * node)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->valueChanged(node);
    }
  }
  if (_parent != 0)
    _parent->fireValueChanged(node);
}

void
SGPropertyNode::fireChildAdded (SGPropertyNode * parent,
				SGPropertyNode * child)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->childAdded(parent, child);
    }
  }
  if (_parent != 0)
    _parent->fireChildAdded(parent, child);
}

void
SGPropertyNode::fireChildRemoved (SGPropertyNode * parent,
				  SGPropertyNode * child)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->childRemoved(parent, child);
    }
  }
  if (_parent != 0)
    _parent->fireChildRemoved(parent, child);
}



////////////////////////////////////////////////////////////////////////
// Simplified hash table for caching paths.
////////////////////////////////////////////////////////////////////////

#define HASH_TABLE_SIZE 199

SGPropertyNode::hash_table::entry::entry ()
  : _value(0)
{
}

SGPropertyNode::hash_table::entry::~entry ()
{
				// Don't delete the value; we don't own
				// the pointer.
}

void
SGPropertyNode::hash_table::entry::set_key (const char * key)
{
  _key = key;
}

void
SGPropertyNode::hash_table::entry::set_value (SGPropertyNode * value)
{
  _value = value;
}

SGPropertyNode::hash_table::bucket::bucket ()
  : _length(0),
    _entries(0)
{
}

SGPropertyNode::hash_table::bucket::~bucket ()
{
  for (int i = 0; i < _length; i++)
    delete _entries[i];
  delete [] _entries;
}

SGPropertyNode::hash_table::entry *
SGPropertyNode::hash_table::bucket::get_entry (const char * key, bool create)
{
  int i;
  for (i = 0; i < _length; i++) {
    if (!strcmp(_entries[i]->get_key(), key))
      return _entries[i];
  }
  if (create) {
    entry ** new_entries = new entry*[_length+1];
    for (i = 0; i < _length; i++) {
      new_entries[i] = _entries[i];
    }
    delete [] _entries;
    _entries = new_entries;
    _entries[_length] = new entry;
    _entries[_length]->set_key(key);
    _length++;
    return _entries[_length - 1];
  } else {
    return 0;
  }
}

bool
SGPropertyNode::hash_table::bucket::erase (SGPropertyNode * node)
{
  for (int i = 0; i < _length; i++) {
    if (_entries[i]->get_value() == node) {
      delete _entries[i];
      for (++i; i < _length; i++) {
        _entries[i-1] = _entries[i];
      }
      _length--;
      return true;
    }
  }
  return false;
}

void
SGPropertyNode::hash_table::bucket::clear (SGPropertyNode::hash_table * owner)
{
  for (int i = 0; i < _length; i++) {
    SGPropertyNode * node = _entries[i]->get_value();
    if (node)
      node->remove_linked_node(owner);
  }
}

SGPropertyNode::hash_table::hash_table ()
  : _data_length(0),
    _data(0)
{
}

SGPropertyNode::hash_table::~hash_table ()
{
  for (unsigned int i = 0; i < _data_length; i++) {
    if (_data[i]) {
      _data[i]->clear(this);
      delete _data[i];
    }
  }
  delete [] _data;
}

SGPropertyNode *
SGPropertyNode::hash_table::get (const char * key)
{
  if (_data_length == 0)
    return 0;
  unsigned int index = hashcode(key) % _data_length;
  if (_data[index] == 0)
    return 0;
  entry * e = _data[index]->get_entry(key);
  if (e == 0)
    return 0;
  else
    return e->get_value();
}

void
SGPropertyNode::hash_table::put (const char * key, SGPropertyNode * value)
{
  if (_data_length == 0) {
    _data = new bucket*[HASH_TABLE_SIZE];
    _data_length = HASH_TABLE_SIZE;
    for (unsigned int i = 0; i < HASH_TABLE_SIZE; i++)
      _data[i] = 0;
  }
  unsigned int index = hashcode(key) % _data_length;
  if (_data[index] == 0) {
    _data[index] = new bucket;
  }
  entry * e = _data[index]->get_entry(key, true);
  e->set_value(value);
  value->add_linked_node(this);
}

bool
SGPropertyNode::hash_table::erase (SGPropertyNode * node)
{
  for (unsigned int i = 0; i < _data_length; i++)
    if (_data[i] && _data[i]->erase(node))
      return true;

  return false;
}

unsigned int
SGPropertyNode::hash_table::hashcode (const char * key)
{
  unsigned int hash = 0;
  while (*key != 0) {
    hash = 31 * hash + *key;
    key++;
  }
  return hash;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyChangeListener.
////////////////////////////////////////////////////////////////////////

SGPropertyChangeListener::~SGPropertyChangeListener ()
{
  for (int i = _properties.size() - 1; i >= 0; i--)
    _properties[i]->removeChangeListener(this);
}

void
SGPropertyChangeListener::valueChanged (SGPropertyNode * node)
{
  // NO-OP
}

void
SGPropertyChangeListener::childAdded (SGPropertyNode * node,
				      SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::childRemoved (SGPropertyNode * parent,
					SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::register_property (SGPropertyNode * node)
{
  _properties.push_back(node);
}

void
SGPropertyChangeListener::unregister_property (SGPropertyNode * node)
{
  vector<SGPropertyNode *>::iterator it =
    find(_properties.begin(), _properties.end(), node);
  if (it != _properties.end())
    _properties.erase(it);
}

namespace simgear
{
template<>
std::ostream& SGRawBase<SGVec3d>::printOn(std::ostream& stream) const
{
    const SGVec3d vec
        = static_cast<const SGRawValue<SGVec3d>*>(this)->getValue();
    for (int i = 0; i < 3; ++i) {
        stream << vec[i];
        if (i < 2)
            stream << ' ';
    }
    return stream;
}

template<>
std::istream& readFrom<SGVec3d>(std::istream& stream, SGVec3d& result)
{
    for (int i = 0; i < 3; ++i) {
        stream >> result[i];
    }
    return stream;
}

template<>
std::ostream& SGRawBase<SGVec4d>::printOn(std::ostream& stream) const
{
    const SGVec4d vec
        = static_cast<const SGRawValue<SGVec4d>*>(this)->getValue();    
    for (int i = 0; i < 4; ++i) {
        stream << vec[i];
        if (i < 3)
            stream << ' ';
    }
    return stream;
}

template<>
std::istream& readFrom<SGVec4d>(std::istream& stream, SGVec4d& result)
{
    for (int i = 0; i < 4; ++i) {
        stream >> result[i];
    }
    return stream;
}

}

// end of props.cxx
