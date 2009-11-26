/**
 * \file props_io.cxx
 * Started Fall 2000 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * See props.html for documentation [replace with URL when available].
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <stdlib.h>		// atof() atoi()

#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/xml/easyxml.hxx>

#include "props.hxx"
#include "props_io.hxx"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>		// strcmp()
#include <vector>
#include <map>

using std::istream;
using std::ifstream;
using std::ostream;
using std::ofstream;
using std::string;
using std::vector;
using std::map;

using std::endl;

#define DEFAULT_MODE (SGPropertyNode::READ|SGPropertyNode::WRITE)



////////////////////////////////////////////////////////////////////////
// Property list visitor, for XML parsing.
////////////////////////////////////////////////////////////////////////

class PropsVisitor : public XMLVisitor
{
public:

  PropsVisitor (SGPropertyNode * root, const string &base, int default_mode = 0,
                bool extended = false)
    : _default_mode(default_mode), _root(root), _level(0), _base(base),
      _hasException(false), _extended(extended)
  {}

  virtual ~PropsVisitor () {}

  void startXML ();
  void endXML ();
  void startElement (const char * name, const XMLAttributes &atts);
  void endElement (const char * name);
  void data (const char * s, int length);
  void warning (const char * message, int line, int column);

  bool hasException () const { return _hasException; }
  sg_io_exception &getException () { return _exception; }
  void setException (const sg_io_exception &exception) {
    _exception = exception;
    _hasException = true;
  }

private:

  struct State
  {
    State () : node(0), type(""), mode(DEFAULT_MODE), omit(false) {}
    State (SGPropertyNode * _node, const char * _type, int _mode, bool _omit)
      : node(_node), type(_type), mode(_mode), omit(_omit) {}
    SGPropertyNode * node;
    string type;
    int mode;
    bool omit;
    map<string,int> counters;
  };

  State &state () { return _state_stack[_state_stack.size() - 1]; }

  void push_state (SGPropertyNode * node, const char * type, int mode, bool omit = false) {
    if (type == 0)
      _state_stack.push_back(State(node, "unspecified", mode, omit));
    else
      _state_stack.push_back(State(node, type, mode, omit));
    _level++;
    _data = "";
  }

  void pop_state () {
    _state_stack.pop_back();
    _level--;
  }

  int _default_mode;
  string _data;
  SGPropertyNode * _root;
  SGPropertyNode null;
  int _level;
  vector<State> _state_stack;
  string _base;
  sg_io_exception _exception;
  bool _hasException;
  bool _extended;
};

void
PropsVisitor::startXML ()
{
  _level = 0;
  _state_stack.resize(0);
}

void
PropsVisitor::endXML ()
{
  _level = 0;
  _state_stack.resize(0);
}


/**
 * Check a yes/no flag, with default.
 */
static bool
checkFlag (const char * flag, bool defaultState = true)
{
  if (flag == 0)
    return defaultState;
  else if (!strcmp(flag, "y"))
    return true;
  else if (!strcmp(flag, "n"))
    return false;
  else {
    string message = "Unrecognized flag value '";
    message += flag;
    message += '\'';
				// FIXME: add location info
    throw sg_io_exception(message, "SimGear Property Reader");
  }
}

void
PropsVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  const char * attval;

  if (_level == 0) {
    if (strcmp(name, "PropertyList")) {
      string message = "Root element name is ";
      message += name;
      message += "; expected PropertyList";
      throw sg_io_exception(message, "SimGear Property Reader");
    }

				// Check for an include.
    attval = atts.getValue("include");
    if (attval != 0) {
      SGPath path(SGPath(_base).dir());
      path.append(attval);
      try {
	readProperties(path.str(), _root, 0, _extended);
      } catch (sg_io_exception &e) {
	setException(e);
      }
    }

    push_state(_root, "", DEFAULT_MODE);
  }

  else {
    State &st = state();
				// Get the index.
    attval = atts.getValue("n");
    int index = 0;
    string strName(name);
    if (attval != 0) {
      index = atoi(attval);
      st.counters[strName] = SG_MAX2(st.counters[strName], index+1);
    } else {
      index = st.counters[strName];
      st.counters[strName]++;
    }

				// Got the index, so grab the node.
    SGPropertyNode * node = st.node->getChild(strName, index, true);
    if (!node->getAttribute(SGPropertyNode::WRITE)) {
      SG_LOG(SG_INPUT, SG_ALERT, "Not overwriting write-protected property "
          << node->getPath(true));
      node = &null;
    }

				// Get the access-mode attributes,
				// but don't set yet (in case they
				// prevent us from recording the value).
    int mode = _default_mode;

    attval = atts.getValue("read");
    if (checkFlag(attval, true))
      mode |= SGPropertyNode::READ;
    attval = atts.getValue("write");
    if (checkFlag(attval, true))
      mode |= SGPropertyNode::WRITE;
    attval = atts.getValue("archive");
    if (checkFlag(attval, false))
      mode |= SGPropertyNode::ARCHIVE;
    attval = atts.getValue("trace-read");
    if (checkFlag(attval, false))
      mode |= SGPropertyNode::TRACE_READ;
    attval = atts.getValue("trace-write");
    if (checkFlag(attval, false))
      mode |= SGPropertyNode::TRACE_WRITE;
    attval = atts.getValue("userarchive");
    if (checkFlag(attval, false))
      mode |= SGPropertyNode::USERARCHIVE;

				// Check for an alias.
    attval = atts.getValue("alias");
    if (attval != 0) {
      if (!node->alias(attval))
	SG_LOG(SG_INPUT, SG_ALERT, "Failed to set alias to " << attval);
    }

				// Check for an include.
    bool omit = false;
    attval = atts.getValue("include");
    if (attval != 0) {
      SGPath path(SGPath(_base).dir());
      path.append(attval);
      try {
	readProperties(path.str(), node, 0, _extended);
      } catch (sg_io_exception &e) {
	setException(e);
      }

      attval = atts.getValue("omit-node");
      omit = checkFlag(attval, false);
    }

    const char *type = atts.getValue("type");
    if (type)
      node->clearValue();
    push_state(node, type, mode, omit);
  }
}

void
PropsVisitor::endElement (const char * name)
{
  State &st = state();
  bool ret;

				// If there are no children and it's
				// not an alias, then it's a leaf value.
  if (st.node->nChildren() == 0 && !st.node->isAlias()) {
    if (st.type == "bool") {
      if (_data == "true" || atoi(_data.c_str()) != 0)
	ret = st.node->setBoolValue(true);
      else
	ret = st.node->setBoolValue(false);
    } else if (st.type == "int") {
      ret = st.node->setIntValue(atoi(_data.c_str()));
    } else if (st.type == "long") {
      ret = st.node->setLongValue(strtol(_data.c_str(), 0, 0));
    } else if (st.type == "float") {
      ret = st.node->setFloatValue(atof(_data.c_str()));
    } else if (st.type == "double") {
      ret = st.node->setDoubleValue(strtod(_data.c_str(), 0));
    } else if (st.type == "string") {
      ret = st.node->setStringValue(_data.c_str());
    } else if (st.type == "vec3d" && _extended) {
      ret = st.node
        ->setValue(simgear::parseString<SGVec3d>(_data));
    } else if (st.type == "vec4d" && _extended) {
      ret = st.node
        ->setValue(simgear::parseString<SGVec4d>(_data));
    } else if (st.type == "unspecified") {
      ret = st.node->setUnspecifiedValue(_data.c_str());
    } else if (_level == 1) {
      ret = true;		// empty <PropertyList>
    } else {
      string message = "Unrecognized data type '";
      message += st.type;
      message += '\'';
				// FIXME: add location information
      throw sg_io_exception(message, "SimGear Property Reader");
    }
    if (!ret)
      SG_LOG(SG_INPUT, SG_ALERT, "readProperties: Failed to set "
	     << st.node->getPath() << " to value \""
	     << _data << "\" with type " << st.type);
  }

				// Set the access-mode attributes now,
				// once the value has already been 
				// assigned.
  st.node->setAttributes(st.mode);

  if (st.omit) {
    State &parent = _state_stack[_state_stack.size() - 2];
    int nChildren = st.node->nChildren();
    for (int i = 0; i < nChildren; i++) {
      SGPropertyNode *src = st.node->getChild(i);
      const char *name = src->getName();
      int index = parent.counters[name];
      parent.counters[name]++;
      SGPropertyNode *dst = parent.node->getChild(name, index, true);
      copyProperties(src, dst);
    }
    parent.node->removeChild(st.node->getName(), st.node->getIndex(), false);
  }
  pop_state();
}

void
PropsVisitor::data (const char * s, int length)
{
  if (state().node->nChildren() == 0)
    _data.append(string(s, length));
}

void
PropsVisitor::warning (const char * message, int line, int column)
{
  SG_LOG(SG_INPUT, SG_ALERT, "readProperties: warning: "
	 << message << " at line " << line << ", column " << column);
}



////////////////////////////////////////////////////////////////////////
// Property list reader.
////////////////////////////////////////////////////////////////////////


/**
 * Read properties from an input stream.
 *
 * @param input The input stream containing an XML property file.
 * @param start_node The root node for reading properties.
 * @param base A base path for resolving external include references.
 * @return true if the read succeeded, false otherwise.
 */
void
readProperties (istream &input, SGPropertyNode * start_node,
		const string &base, int default_mode, bool extended)
{
  PropsVisitor visitor(start_node, base, default_mode, extended);
  readXML(input, visitor, base);
  if (visitor.hasException())
    throw visitor.getException();
}


/**
 * Read properties from a file.
 *
 * @param file A string containing the file path.
 * @param start_node The root node for reading properties.
 * @return true if the read succeeded, false otherwise.
 */
void
readProperties (const string &file, SGPropertyNode * start_node,
                int default_mode, bool extended)
{
  PropsVisitor visitor(start_node, file, default_mode, extended);
  readXML(file, visitor);
  if (visitor.hasException())
    throw visitor.getException();
}


/**
 * Read properties from an in-memory buffer.
 *
 * @param buf A character buffer containing the xml data.
 * @param size The size/length of the buffer in bytes
 * @param start_node The root node for reading properties.
 * @return true if the read succeeded, false otherwise.
 */
void readProperties (const char *buf, const int size,
                     SGPropertyNode * start_node, int default_mode,
                     bool extended)
{
  PropsVisitor visitor(start_node, "", default_mode, extended);
  readXML(buf, size, visitor);
  if (visitor.hasException())
    throw visitor.getException();
}


////////////////////////////////////////////////////////////////////////
// Property list writer.
////////////////////////////////////////////////////////////////////////

#define INDENT_STEP 2

/**
 * Return the type name.
 */
static const char *
getTypeName (simgear::props::Type type)
{
  using namespace simgear;
  switch (type) {
  case props::UNSPECIFIED:
    return "unspecified";
  case props::BOOL:
    return "bool";
  case props::INT:
    return "int";
  case props::LONG:
    return "long";
  case props::FLOAT:
    return "float";
  case props::DOUBLE:
    return "double";
  case props::STRING:
    return "string";
  case props::VEC3D:
    return "vec3d";
  case props::VEC4D:
    return "vec4d";
  case props::ALIAS:
  case props::NONE:
    return "unspecified";
  default: // avoid compiler warning
    break;
  }

  // keep the compiler from squawking
  return "unspecified";
}


/**
 * Escape characters for output.
 */
static void
writeData (ostream &output, const string &data)
{
  for (int i = 0; i < (int)data.size(); i++) {
    switch (data[i]) {
    case '&':
      output << "&amp;";
      break;
    case '<':
      output << "&lt;";
      break;
    case '>':
      output << "&gt;";
      break;
    default:
      output << data[i];
      break;
    }
  }
}

static void
doIndent (ostream &output, int indent)
{
  while (indent-- > 0) {
    output << ' ';
  }
}


static void
writeAtts (ostream &output, const SGPropertyNode * node, bool forceindex)
{
  int index = node->getIndex();

  if (index != 0 || forceindex)
    output << " n=\"" << index << '"';

#if 0
  if (!node->getAttribute(SGPropertyNode::READ))
    output << " read=\"n\"";

  if (!node->getAttribute(SGPropertyNode::WRITE))
    output << " write=\"n\"";

  if (node->getAttribute(SGPropertyNode::ARCHIVE))
    output << " archive=\"y\"";
#endif

}


/**
 * Test whether a node is archivable or has archivable descendants.
 */
static bool
isArchivable (const SGPropertyNode * node, SGPropertyNode::Attribute archive_flag)
{
  // FIXME: it's inefficient to do this all the time
  if (node->getAttribute(archive_flag))
    return true;
  else {
    int nChildren = node->nChildren();
    for (int i = 0; i < nChildren; i++)
      if (isArchivable(node->getChild(i), archive_flag))
	return true;
  }
  return false;
}


static bool
writeNode (ostream &output, const SGPropertyNode * node,
           bool write_all, int indent, SGPropertyNode::Attribute archive_flag)
{
				// Don't write the node or any of
				// its descendants unless it is
				// allowed to be archived.
  if (!write_all && !isArchivable(node, archive_flag))
    return true;		// Everything's OK, but we won't write.

  const string name = node->getName();
  int nChildren = node->nChildren();
  bool node_has_value = false;

				// If there is a literal value,
				// write it first.
  if (node->hasValue() && (write_all || node->getAttribute(archive_flag))) {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node, nChildren != 0);
    if (node->isAlias() && node->getAliasTarget() != 0) {
      output << " alias=\"" << node->getAliasTarget()->getPath()
	     << "\"/>" << endl;
    } else {
      if (node->getType() != simgear::props::UNSPECIFIED)
	output << " type=\"" << getTypeName(node->getType()) << '"';
      output << '>';
      writeData(output, node->getStringValue());
      output << "</" << name << '>' << endl;
    }
    node_has_value = true;
  }

				// If there are children, write them next.
  if (nChildren > 0) {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node, node_has_value);
    output << '>' << endl;
    for (int i = 0; i < nChildren; i++)
      writeNode(output, node->getChild(i), write_all, indent + INDENT_STEP, archive_flag);
    doIndent(output, indent);
    output << "</" << name << '>' << endl;
  }

  return true;
}


void
writeProperties (ostream &output, const SGPropertyNode * start_node,
                 bool write_all, SGPropertyNode::Attribute archive_flag)
{
  int nChildren = start_node->nChildren();

  output << "<?xml version=\"1.0\"?>" << endl << endl;
  output << "<PropertyList>" << endl;

  for (int i = 0; i < nChildren; i++) {
    writeNode(output, start_node->getChild(i), write_all, INDENT_STEP, archive_flag);
  }

  output << "</PropertyList>" << endl;
}


void
writeProperties (const string &file, const SGPropertyNode * start_node,
                 bool write_all, SGPropertyNode::Attribute archive_flag)
{
  SGPath path(file.c_str());
  path.create_dir(0777);

  ofstream output(file.c_str());
  if (output.good()) {
    writeProperties(output, start_node, write_all, archive_flag);
  } else {
    throw sg_io_exception("Cannot open file", sg_location(file));
  }
}

// Another variation, useful when called from gdb
void
writeProperties (const char* file, const SGPropertyNode * start_node)
{
    writeProperties(string(file), start_node, true);
}



////////////////////////////////////////////////////////////////////////
// Copy properties from one tree to another.
////////////////////////////////////////////////////////////////////////


/**
 * Copy one property tree to another.
 * 
 * @param in The source property tree.
 * @param out The destination property tree.
 * @return true if all properties were copied, false if some failed
 *  (for example, if the property's value is tied read-only).
 */
bool
copyProperties (const SGPropertyNode *in, SGPropertyNode *out)
{
  using namespace simgear;
  bool retval = true;

				// First, copy the actual value,
				// if any.
  if (in->hasValue()) {
    switch (in->getType()) {
    case props::BOOL:
      if (!out->setBoolValue(in->getBoolValue()))
	retval = false;
      break;
    case props::INT:
      if (!out->setIntValue(in->getIntValue()))
	retval = false;
      break;
    case props::LONG:
      if (!out->setLongValue(in->getLongValue()))
	retval = false;
      break;
    case props::FLOAT:
      if (!out->setFloatValue(in->getFloatValue()))
	retval = false;
      break;
    case props::DOUBLE:
      if (!out->setDoubleValue(in->getDoubleValue()))
	retval = false;
      break;
    case props::STRING:
      if (!out->setStringValue(in->getStringValue()))
	retval = false;
      break;
    case props::UNSPECIFIED:
      if (!out->setUnspecifiedValue(in->getStringValue()))
	retval = false;
      break;
    case props::VEC3D:
      if (!out->setValue(in->getValue<SGVec3d>()))
        retval = false;
      break;
    case props::VEC4D:
      if (!out->setValue(in->getValue<SGVec4d>()))
        retval = false;
      break;
    default:
      if (in->isAlias())
	break;
      string message = "Unknown internal SGPropertyNode type";
      message += in->getType();
      throw sg_error(message, "SimGear Property Reader");
    }
  }

  				// copy the attributes.
  out->setAttributes( in->getAttributes() );

				// Next, copy the children.
  int nChildren = in->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * in_child = in->getChild(i);
    SGPropertyNode * out_child = out->getChild(in_child->getNameString(),
					       in_child->getIndex(),
					       true);
    if (!copyProperties(in_child, out_child))
      retval = false;
  }

  return retval;
}

// end of props_io.cxx
