
#include <simgear/compiler.h>

#include <stdlib.h>		// atof() atoi()

#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/xml/easyxml.hxx>

#include "sg_path.hxx"
#include "props.hxx"

#include STL_IOSTREAM
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
#  include <fstream>
#else
#  include <fstream.h>
#endif
#include STL_STRING
#include <vector>
#include <map>

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istream);
SG_USING_STD(ifstream);
SG_USING_STD(ostream);
SG_USING_STD(ofstream);
#endif
SG_USING_STD(string);
SG_USING_STD(vector);
SG_USING_STD(map);

#define DEFAULT_MODE (SGPropertyNode::READ|SGPropertyNode::WRITE|SGPropertyNode::ARCHIVE)



////////////////////////////////////////////////////////////////////////
// Property list visitor, for XML parsing.
////////////////////////////////////////////////////////////////////////

class PropsVisitor : public XMLVisitor
{
public:

  PropsVisitor (SGPropertyNode * root, const string &base)
    : _ok(true), _root(root), _level(0), _base(base) {}

  void startXML ();
  void endXML ();
  void startElement (const char * name, const XMLAttributes &atts);
  void endElement (const char * name);
  void data (const char * s, int length);
  void warning (const char * message, int line, int column);
  void error (const char * message, int line, int column);

  bool isOK () const { return _ok; }

private:

  struct State
  {
    State () : node(0), type(""), mode(DEFAULT_MODE) {}
    State (SGPropertyNode * _node, const char * _type, int _mode)
      : node(_node), type(_type), mode(_mode) {}
    SGPropertyNode * node;
    string type;
    int mode;
    map<string,int> counters;
  };

  State &state () { return _state_stack[_state_stack.size() - 1]; }

  void push_state (SGPropertyNode * node, const char * type, int mode) {
    if (type == 0)
      _state_stack.push_back(State(node, "unspecified", mode));
    else
      _state_stack.push_back(State(node, type, mode));
    _level++;
    _data = "";
  }

  void pop_state () {
    _state_stack.pop_back();
    _level--;
  }

  bool _ok;
  string _data;
  SGPropertyNode * _root;
  int _level;
  vector<State> _state_stack;
  string _base;
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
 * Check a yes/no flag that defaults to 'yes'.
 */
static bool
checkFlag (const char * flag)
{
  if (flag == 0 || string(flag) == "y")
    return true;
  else if (string(flag) == "n")
    return false;
  else {
    SG_LOG(SG_INPUT, SG_ALERT, "Unrecognized flag value '" << flag
	   << "', assuming 'y'");
    return true;
  }
}

void
PropsVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  State &st = state();

  if (_level == 0) {
    if (string(name) != (string)"PropertyList") {
      SG_LOG(SG_INPUT, SG_ALERT, "Root element name is " <<
	     name << "; expected PropertyList");
      _ok = false;
    }
    push_state(_root, "", DEFAULT_MODE);
  }

  else {

    const char * attval;
				// Get the index.
    attval = atts.getValue("n");
    int index = 0;
    if (attval != 0) {
      index = atoi(attval);
      st.counters[name] = SG_MAX2(st.counters[name], index+1);
    } else {
      index = st.counters[name];
      st.counters[name]++;
    }

				// Got the index, so grab the node.
    SGPropertyNode * node = st.node->getChild(name, index, true);

				// Get the access-mode attributes,
				// but don't set yet (in case they
				// prevent us from recording the value).
    int mode = 0;

    attval = atts.getValue("read");
    if (checkFlag(attval))
      mode |= SGPropertyNode::READ;
    attval = atts.getValue("write");
    if (checkFlag(attval))
      mode |= SGPropertyNode::WRITE;
    attval = atts.getValue("archive");
    if (checkFlag(attval))
      mode |= SGPropertyNode::ARCHIVE;

				// Check for an alias.
    attval = atts.getValue("alias");
    if (attval != 0) {
      if (!node->alias(attval))
	SG_LOG(SG_INPUT, SG_ALERT, "Failed to set alias to " << attval);
    }

				// Check for an include.
    attval = atts.getValue("include");
    if (attval != 0) {
      SGPath path(SGPath(_base).dir());
      cerr << "Base is " << _base << endl;
      cerr << "Dir is " << SGPath(_base).dir() << endl;
      path.append(attval);
      if (!readProperties(path.str(), node)) {
	SG_LOG(SG_INPUT, SG_ALERT, "Failed to read include file "
	       << attval);
	_ok = false;
      }
    }

    push_state(node, atts.getValue("type"), mode);
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
      ret = st.node->setStringValue(_data);
    } else if (st.type == "unspecified") {
      ret = st.node->setUnspecifiedValue(_data);
    } else {
      SG_LOG(SG_INPUT, SG_ALERT, "Unrecognized data type " << st.type
	     << " assuming 'unspecified'");
      ret = st.node->setUnspecifiedValue(_data);
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

void
PropsVisitor::error (const char * message, int line, int column)
{
  SG_LOG(SG_INPUT, SG_ALERT, "readProperties: FATAL: " <<
	 message << " at line " << line << ", column " << column);
  _ok = false;
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
bool
readProperties (istream &input, SGPropertyNode * start_node,
		const string &base)
{
  PropsVisitor visitor(start_node, base);
  return readXML(input, visitor) && visitor.isOK();
}


/**
 * Read properties from a file.
 *
 * @param file A string containing the file path.
 * @param start_node The root node for reading properties.
 * @return true if the read succeeded, false otherwise.
 */
bool
readProperties (const string &file, SGPropertyNode * start_node)
{
  cerr << "Reading properties from " << file << endl;
  ifstream input(file.c_str());
  if (input.good()) {
    return readProperties(input, start_node, file);
  } else {
    SG_LOG(SG_INPUT, SG_ALERT, "Error reading property list from file "
	   << file);
    return false;
  }
}



////////////////////////////////////////////////////////////////////////
// Property list writer.
////////////////////////////////////////////////////////////////////////

#define INDENT_STEP 2

/**
 * Return the type name.
 */
static const char *
getTypeName (SGPropertyNode::Type type)
{
  switch (type) {
  case SGPropertyNode::UNSPECIFIED:
    return "unspecified";
  case SGPropertyNode::BOOL:
    return "bool";
  case SGPropertyNode::INT:
    return "int";
  case SGPropertyNode::LONG:
    return "long";
  case SGPropertyNode::FLOAT:
    return "float";
  case SGPropertyNode::DOUBLE:
    return "double";
  case SGPropertyNode::STRING:
    return "string";
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
writeAtts (ostream &output, const SGPropertyNode * node)
{
  int index = node->getIndex();

  if (index != 0)
    output << " n = \"" << index << '"';

  if (!node->getAttribute(SGPropertyNode::READ))
    output << " read=\"n\"";

  if (!node->getAttribute(SGPropertyNode::WRITE))
    output << " write=\"n\"";

  if (!node->getAttribute(SGPropertyNode::ARCHIVE))
    output << " archive=\"n\"";

}


static bool
writeNode (ostream &output, const SGPropertyNode * node, int indent)
{
				// Don't write the node or any of
				// its descendants unless it is
				// allowed to be archived.
  if (!node->getAttribute(SGPropertyNode::ARCHIVE))
    return true;		// Everything's OK, but we won't write.

  const string &name = node->getName();
  int index = node->getIndex();
  int nChildren = node->nChildren();

				// If there is a literal value,
				// write it first.
  if (node->hasValue()) {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node);
    if (node->isAlias() && node->getAliasTarget() != 0) {
      output << " alias=\"" << node->getAliasTarget()->getPath()
	     << "\"/>" << endl;
    } else {
      if (node->getType() != SGPropertyNode::UNSPECIFIED)
	output << " type=\"" << getTypeName(node->getType()) << '"';
      output << '>';
      writeData(output, node->getStringValue());
      output << "</" << name << '>' << endl;
    }
  }

				// If there are children, write them
				// next.
  if (nChildren > 0 || node->isAlias()) {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node);
    output << '>' << endl;
    for (int i = 0; i < nChildren; i++)
      writeNode(output, node->getChild(i), indent + INDENT_STEP);
    doIndent(output, indent);
    output << "</" << name << '>' << endl;
  }

  return true;
}


/**
 * Write a property tree to an output stream in XML format.
 *
 * @param output The output stream.
 * @param start_node The root node to write.
 * @return true if the write succeeded, false otherwise.
 */
bool
writeProperties (ostream &output, const SGPropertyNode * start_node)
{
  int nChildren = start_node->nChildren();

  output << "<?xml version=\"1.0\"?>" << endl << endl;
  output << "<PropertyList>" << endl;

  for (int i = 0; i < nChildren; i++) {
    writeNode(output, start_node->getChild(i), INDENT_STEP);
  }

  output << "</PropertyList>" << endl;

  return true;
}


/**
 * Write a property tree to a file in XML format.
 *
 * @param file The destination file.
 * @param start_node The root node to write.
 * @return true if the write succeeded, false otherwise.
 */
bool
writeProperties (const string &file, const SGPropertyNode * start_node)
{
  ofstream output(file.c_str());
  if (output.good()) {
    return writeProperties(output, start_node);
  } else {
    SG_LOG(SG_INPUT, SG_ALERT, "Cannot write properties to file "
	   << file);
    return false;
  }
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
  bool retval = true;

				// First, copy the actual value,
				// if any.
  if (in->hasValue()) {
    switch (in->getType()) {
    case SGPropertyNode::BOOL:
      if (!out->setBoolValue(in->getBoolValue()))
	retval = false;
      break;
    case SGPropertyNode::INT:
      if (!out->setIntValue(in->getIntValue()))
	retval = false;
      break;
    case SGPropertyNode::LONG:
      if (!out->setLongValue(in->getLongValue()))
	retval = false;
      break;
    case SGPropertyNode::FLOAT:
      if (!out->setFloatValue(in->getFloatValue()))
	retval = false;
      break;
    case SGPropertyNode::DOUBLE:
      if (!out->setDoubleValue(in->getDoubleValue()))
	retval = false;
      break;
    case SGPropertyNode::STRING:
      if (!out->setStringValue(in->getStringValue()))
	retval = false;
      break;
    case SGPropertyNode::UNSPECIFIED:
      if (!out->setUnspecifiedValue(in->getStringValue()))
	retval = false;
      break;
    default:
      throw string("Unrecognized SGPropertyNode type");
    }
  }

				// Next, copy the children.
  int nChildren = in->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * in_child = in->getChild(i);
    SGPropertyNode * out_child = out->getChild(in_child->getName(),
					       in_child->getIndex(),
					       true);
    if (!copyProperties(in_child, out_child))
      retval = false;
  }

  return retval;
}

// end of props_io.cxx
