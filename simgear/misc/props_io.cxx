
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/xml/easyxml.hxx>

#include "props.hxx"

#include <iostream>
#include <string>
#include <vector>

using std::istream;
using std::ostream;
using std::string;
using std::vector;


////////////////////////////////////////////////////////////////////////
// Visitor class for building the property list.
////////////////////////////////////////////////////////////////////////

class PropVisitor : public XMLVisitor
{
public:
  PropVisitor (SGPropertyList * props) : _props(props), _level(0), _ok(true) {}
  void startDocument ();
  void startElement (const char * name, const XMLAttributes &atts);
  void endElement (const char * name);
  void data (const char * s, int length);
  void warning (const char * message, int line, int col);
  void error (const char * message, int line, int col);

  bool isOK () const { return _ok; }

private:

  void pushState (const char * name) {
    _states.push_back(_state);
    _level++;
    _state.name = name;
    _state.type = SGValue::UNKNOWN;
    _state.data = "";
    _state.hasChildren = false;
    _state.hasData = false;
  }

  void popState () {
    _state = _states.back();
    _states.pop_back();
    _level--;
  }

  struct State
  {
    State () : hasChildren(false), hasData(false) {}
    string name;
    SGValue::Type type;
    string data;
    bool hasChildren;
    bool hasData;
  };

  SGPropertyList * _props;
  State _state;
  vector<State> _states;
  int _level;
  bool _ok;
};

void
PropVisitor::startDocument ()
{
  _level = 0;
  _ok = true;
}


void
PropVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  if (!_ok)
    return;

  if (_level == 0 && strcmp(name, "PropertyList")) {
    _ok = false;
    FG_LOG(FG_INPUT, FG_ALERT, "XML document has root element \""
	   << name << "\" instead of \"PropertyList\"");
    return;
  }

				// Mixed content?
  _state.hasChildren = true;
  if (_state.hasData) {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "XML element has mixed elements and data in element "
	   << _state.name);
    _ok = false;
    return;
  }

				// Start a new state.
  pushState(name);

				// See if there's a type specified.
  const char * type = atts.getValue("type");
  if (type == 0 || !strcmp("unknown", type))
    _state.type = SGValue::UNKNOWN;
  else if (!strcmp("bool", type))
    _state.type = SGValue::BOOL;
  else if (!strcmp("int", type))
    _state.type = SGValue::INT;
  else if (!strcmp("float", type))
    _state.type = SGValue::FLOAT;
  else if (!strcmp("double", type))
    _state.type = SGValue::DOUBLE;
  else if (!strcmp("string", type))
    _state.type = SGValue::STRING;
  else
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized type " << type
	   << ", using UNKNOWN");
}

void
PropVisitor::endElement (const char * name)
{
  if (!_ok)
    return;

				// See if there's a property to add.
  if (_state.hasData) {
    bool status = false;

				// Figure out the path name.
    string path = "";
    for (int i = 2; i < _level; i++) {
      path += '/';
      path += _states[i].name;
    }
    path += '/';
    path += _state.name;

				// Set the value
    switch (_state.type) {
    case SGValue::BOOL:
      if (_state.data == "true" || _state.data == "TRUE") {
	status = _props->setBoolValue(path, true);
      } else if (atof(_state.data.c_str()) != 0.0) {
	status = _props->setBoolValue(path, true);
      } else {
	status =_props->setBoolValue(path, false);
      }
      break;
    case SGValue::INT :
      status = _props->setIntValue(path, atoi(_state.data.c_str()));
      break;
    case SGValue::FLOAT:
      status = _props->setFloatValue(path, atof(_state.data.c_str()));
      break;
    case SGValue::DOUBLE:
      status = _props->setDoubleValue(path, atof(_state.data.c_str()));
      break;
    case SGValue::STRING:
      status = _props->setStringValue(path, _state.data);
      break;
    default:
      status = _props->setUnknownValue(path, _state.data);
      break;
    }
    if (!status)
      FG_LOG(FG_INPUT, FG_ALERT, "Failed to set property "
	     << path << " to " << _state.data);
  }

				// Pop the stack.
  popState();
}

void
PropVisitor::data (const char * s, int length)
{
  if (!_ok)
    return;

				// Check if there is any non-whitespace
  if (!_state.hasData)
    for (int i = 0; i < length; i++) 
      if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
	_state.hasData = true;

  _state.data += string(s, length); // FIXME: inefficient
}

void
PropVisitor::warning (const char * message, int line, int col)
{
  FG_LOG(FG_INPUT, FG_ALERT, "Warning importing property list: "
	 << message << " (" << line << ',' << col << ')');
}

void
PropVisitor::error (const char * message, int line, int col)
{
  FG_LOG(FG_INPUT, FG_ALERT, "Error importing property list: "
	 << message << " (" << line << ',' << col << ')');
  _ok = false;
}



////////////////////////////////////////////////////////////////////////
// Property list reader.
////////////////////////////////////////////////////////////////////////

bool
readPropertyList (istream &input, SGPropertyList * props)
{
  PropVisitor visitor(props);
  return readXML(input, visitor) && visitor.isOK();
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
}


/**
 * Escape characters for output.
 */
static void
writeData (ostream &output, const string &data)
{
  for (int i = 0; i < data.size(); i++) {
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
writeNode (ostream &output, SGPropertyNode node, int indent)
{
  const string &name = node.getName();
  int size = node.size();

				// Write out the literal value, if any.
  SGValue * value = node.getValue();
  if (value != 0) {
    SGValue::Type type = value->getType();
    doIndent(output, indent);
    output << '<' << name;
    if (type != SGValue::UNKNOWN)
      output << " type=\"" << getTypeName(type) << '"';
    output << '>';
    writeData(output, value->getStringValue());
    output << "</" << name << '>' << endl;
  }

				// Write out the children, if any.
  if (size > 0) {
    doIndent(output, indent);
    output << '<' << name << '>' << endl;;
    for (int i = 0; i < size; i++) {
      writeNode(output, node.getChild(i), indent + INDENT_STEP);
    }
    doIndent(output, indent);
    output << "</" << name << '>' << endl;
  }
}

bool
writePropertyList (ostream &output, const SGPropertyList * props)
{
  SGPropertyNode root ("/", (SGPropertyList *)props); // FIXME

  output << "<?xml version=\"1.0\"?>" << endl << endl;
  output << "<PropertyList>" << endl;

  for (int i = 0; i < root.size(); i++) {
    writeNode(output, root.getChild(i), INDENT_STEP);
  }

  output << "</PropertyList>" << endl;
}
