
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdlib.h>		// atof() atoi()

#include <simgear/debug/logstream.hxx>
#include <simgear/xml/easyxml.hxx>

#include "fgpath.hxx"
#include "props.hxx"

#include STL_IOSTREAM
#if !defined(FG_HAVE_NATIVE_SGI_COMPILERS)
#  include <fstream>
#else
#  include <fstream.h>
#endif
#include STL_STRING
#include <vector>
#include <map>

#if !defined(FG_HAVE_NATIVE_SGI_COMPILERS)
FG_USING_STD(istream);
FG_USING_STD(ifstream);
FG_USING_STD(ostream);
FG_USING_STD(ofstream);
#endif
FG_USING_STD(string);
FG_USING_STD(vector);
FG_USING_STD(map);



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
    State () : node(0), type("") {}
    State (SGPropertyNode * _node, const char * _type)
      : node(_node), type(_type) {}
    SGPropertyNode * node;
    string type;
    map<string,int> counters;
  };

  State &state () { return _state_stack[_state_stack.size() - 1]; }

  void push_state (SGPropertyNode * node, const char * type) {
    if (type == 0)
      _state_stack.push_back(State(node, "unknown"));
    else
      _state_stack.push_back(State(node, type));
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

void
PropsVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  State &st = state();

  if (_level == 0) {
    if (string(name) != "PropertyList") {
      FG_LOG(FG_INPUT, FG_ALERT, "Root element name is " <<
	     name << "; expected PropertyList");
      _ok = false;
    }
    push_state(_root, "");
  }

  else {
				// Get the index.
    const char * att_n = atts.getValue("n");
    int index = 0;
    if (att_n != 0) {
      index = atoi(att_n);
      st.counters[name] = max(st.counters[name], index+1);
    } else {
      index = st.counters[name];
      st.counters[name]++;
    }

				// Check for an alias.
    SGPropertyNode * node = st.node->getChild(name, index, true);
    const char * att_alias = atts.getValue("alias");
    if (att_alias != 0) {
      if (!node->alias(att_alias))
	FG_LOG(FG_INPUT, FG_ALERT, "Failed to set alias to " << att_alias);
    }

				// Check for an include.
    const char * att_include = atts.getValue("include");
    if (att_include != 0) {
      FGPath path(FGPath(_base).dir());
      cerr << "Base is " << _base << endl;
      cerr << "Dir is " << FGPath(_base).dir() << endl;
      path.append(att_include);
      if (!readProperties(path.str(), node)) {
	FG_LOG(FG_INPUT, FG_ALERT, "Failed to read include file "
	       << att_include);
	_ok = false;
      }
    }

    push_state(node, atts.getValue("type"));
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
    } else if (st.type == "float") {
      ret = st.node->setFloatValue(atof(_data.c_str()));
    } else if (st.type == "double") {
      ret = st.node->setDoubleValue(atof(_data.c_str()));
    } else if (st.type == "string") {
      ret = st.node->setStringValue(_data);
    } else if (st.type == "unknown") {
      ret = st.node->setUnknownValue(_data);
    } else {
      FG_LOG(FG_INPUT, FG_ALERT, "Unknown data type " << st.type
	     << " assuming 'unknown'");
      ret = st.node->setUnknownValue(_data);
    }
    if (!ret)
      FG_LOG(FG_INPUT, FG_ALERT, "readProperties: Failed to set "
	     << st.node->getPath() << " to value \""
	     << _data << "\" with type " << st.type);
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
  FG_LOG(FG_INPUT, FG_ALERT, "readProperties: warning: "
	 << message << " at line " << line << ", column " << column);
}

void
PropsVisitor::error (const char * message, int line, int column)
{
  FG_LOG(FG_INPUT, FG_ALERT, "readProperties: FATAL: " <<
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
    FG_LOG(FG_INPUT, FG_ALERT, "Error reading property list from file "
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
getTypeName (SGValue::Type type)
{
  switch (type) {
  case SGValue::UNKNOWN:
    return "unknown";
  case SGValue::BOOL:
    return "bool";
  case SGValue::INT:
    return "int";
  case SGValue::FLOAT:
    return "float";
  case SGValue::DOUBLE:
    return "double";
  case SGValue::STRING:
    return "string";
  }

  // keep the compiler from squawking
  return "unknown";
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


static bool
writeNode (ostream &output, const SGPropertyNode * node, int indent)
{
  const string &name = node->getName();
  int index = node->getIndex();
  int nChildren = node->nChildren();

				// If there is a literal value,
				// write it first.
  if (node->hasValue()) {
    doIndent(output, indent);
    output << '<' << name << " n=\"" << index;
    if (node->isAlias() && node->getAliasTarget() != 0) {
      output << "\" alias=\""
	     << node->getAliasTarget()->getPath() << "\"/>" << endl;
    } else {
      if (node->getType() != SGValue::UNKNOWN)
	output << "\" type=\"" << getTypeName(node->getType()) << '"';
      output << '>';
      writeData(output, node->getStringValue());
      output << "</" << name << '>' << endl;;
    }
  }

				// If there are children, write them
				// next.
  if (nChildren > 0) {
    doIndent(output, indent);
    output << '<' << name << " n=\"" << index << "\">" << endl;;
    for (int i = 0; i < nChildren; i++)
      writeNode(output, node->getChild(i), indent + INDENT_STEP);
    doIndent(output, indent);
    output << "</" << name << '>' << endl;
  }

				// If there were no children and no
				// value, at least note the presence
				// of the node.
  if (nChildren == 0 && !node->hasValue()) {
    doIndent(output, indent);
    output << '<' << name << " n=\"" << index << "\"/>" << endl;
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
    FG_LOG(FG_INPUT, FG_ALERT, "Cannot write properties to file "
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
    case SGValue::BOOL:
      if (!out->setBoolValue(in->getBoolValue()))
	retval = false;
      break;
    case SGValue::INT:
      if (!out->setIntValue(in->getIntValue()))
	retval = false;
      break;
    case SGValue::FLOAT:
      if (!out->setFloatValue(in->getFloatValue()))
	retval = false;
      break;
    case SGValue::DOUBLE:
      if (!out->setDoubleValue(in->getDoubleValue()))
	retval = false;
      break;
    case SGValue::STRING:
      if (!out->setStringValue(in->getStringValue()))
	retval = false;
      break;
    case SGValue::UNKNOWN:
      if (!out->setUnknownValue(in->getStringValue()))
	retval = false;
      break;
    default:
      throw string("Unknown SGValue type");
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
