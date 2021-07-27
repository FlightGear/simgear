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

#include <stdlib.h>     // atof() atoi()

#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

#include "props.hxx"
#include "props_io.hxx"
#include "vectorPropTemplates.hxx"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>      // strcmp()
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

// Name of special node containing unused attributes
const std::string ATTR = "_attr_";


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
    bool hasChildren() const
    {
      int n_children = node->nChildren();
      return n_children > 1
         || (n_children == 1 && node->getChild(0)->getNameString() != ATTR);
    }
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
 * Set/unset a yes/no flag.
 */
static void
setFlag( int& mode,
         int mask,
         const std::string& flag,
         const sg_location& location )
{
  if( flag == "y" )
    mode |= mask;
  else if( flag == "n" )
    mode &= ~mask;
  else
  {
    string message = "Unrecognized flag value '";
    message += flag;
    message += '\'';
    throw sg_io_exception(message, location, SG_ORIGIN, false);
  }
}

void
PropsVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  const char * attval;
  const sg_location location(getPath(), getLine(), getColumn());

  if (_level == 0) {
    if (strcmp(name, "PropertyList")) {
      string message = "Root element name is ";
      message += name;
      message += "; expected PropertyList";
      throw sg_io_exception(message, location, SG_ORIGIN, false);
    }

    // Check for an include.
    attval = atts.getValue("include");
    if (attval != 0) {
      try {
          SGPath path = simgear::ResourceManager::instance()->findPath(attval, SGPath(_base).dir());
          if (path.isNull())
          {
              string message ="Cannot open file ";
              message += attval;
              throw sg_io_exception(message, location, SG_ORIGIN, false);
          }
          readProperties(path, _root, 0, _extended);
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
             << node->getPath(true) << "\n at " << location.asString());
      node = &null;
    }

    // TODO use correct default mode (keep for now to match past behavior)
    int mode = _default_mode | SGPropertyNode::READ | SGPropertyNode::WRITE;
    int omit = false;
    const char* type = 0;

    SGPropertyNode* attr_node = NULL;

    for(int i = 0; i < atts.size(); ++i)
    {
      const std::string att_name = atts.getName(i);
      const std::string val = atts.getValue(i);

      // Get the access-mode attributes,
      // but don't set yet (in case they
      // prevent us from recording the value).
      if( att_name == "read" )
        setFlag(mode, SGPropertyNode::READ, val, location);
      else if( att_name == "write" )
        setFlag(mode, SGPropertyNode::WRITE, val, location);
      else if( att_name == "archive" )
        setFlag(mode, SGPropertyNode::ARCHIVE, val, location);
      else if( att_name == "trace-read" )
        setFlag(mode, SGPropertyNode::TRACE_READ, val, location);
      else if( att_name == "trace-write" )
        setFlag(mode, SGPropertyNode::TRACE_WRITE, val, location);
      else if( att_name == "userarchive" )
        setFlag(mode, SGPropertyNode::USERARCHIVE, val, location);
      else if( att_name == "preserve" )
        setFlag(mode, SGPropertyNode::PRESERVE, val, location);
      // note we intentionally don't handle PROTECTED here, it's
      // designed to be only set from compiled code, not loaded
      // dynamically.

      // Check for an alias.
      else if( att_name == "alias" )
      {
        if( !node->alias(val) )
          SG_LOG
          (
            SG_INPUT,
            SG_ALERT,
            "Failed to set alias to " << val << "\n at " << location.asString()
          );
      }

      // Check for an include.
      else if( att_name == "include" )
      {
        try
        {
          SGPath path = simgear::ResourceManager::instance()
                      ->findPath(val, SGPath(_base).dir());
          if (path.isNull())
          {
            string message ="Cannot open file ";
            message += val;
            throw sg_io_exception(message, location, SG_ORIGIN, false);
          }
          readProperties(path, node, 0, _extended);
        }
        catch (sg_io_exception &e)
        {
          setException(e);
        }
      }

      else if( att_name == "omit-node" )
        setFlag(omit, 1, val, location);
      else if( att_name == "type" )
      {
        type = atts.getValue(i);

        // if a type is given and the node is tied,
        // don't clear the value because
        // clearValue() unties the property
        if( !node->isTied() )
          node->clearValue();
      }
      else if( att_name != "n" )
      {
        // Store all additional attributes in a special node named _attr_
        if( !attr_node )
          attr_node = node->getChild(ATTR, 0, true);

        attr_node->setUnspecifiedValue(att_name.c_str(), val.c_str());
      }
    }
    push_state(node, type, mode, omit);
  }
}

void
PropsVisitor::endElement (const char * name)
{
  State &st = state();
  bool ret;
  const sg_location location(getPath(), getLine(), getColumn());

  // If there are no children and it's
  // not an alias, then it's a leaf value.
  if( !st.hasChildren() && !st.node->isAlias() )
  {
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
      throw sg_io_exception(message, location, SG_ORIGIN, false);
    }
    if( !ret )
      SG_LOG
      (
        SG_INPUT,
        SG_ALERT,
        "readProperties: Failed to set " << st.node->getPath()
                         << " to value \"" << _data
                         << "\" with type " << st.type
                         << "\n at " << location.asString()
      );
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
    parent.node->removeChild(st.node->getName(), st.node->getIndex());
  }
  pop_state();
}

void
PropsVisitor::data (const char * s, int length)
{
  if( !state().hasChildren() )
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
readProperties (const SGPath &file, SGPropertyNode * start_node,
                int default_mode, bool extended)
{
  PropsVisitor visitor(start_node, file.utf8Str(), default_mode, extended);
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
writeAtts( std::ostream &output,
           const SGPropertyNode* node,
           bool forceindex,
           const SGPropertyNode* attr = NULL )
{
  int index = node->getIndex();

  if (index != 0 || forceindex)
    output << " n=\"" << index << '"';

  if( attr )
    for(int i = 0; i < attr->nChildren(); ++i)
    {
      output << ' ' << attr->getChild(i)->getName() << "=\"";

      const std::string data = attr->getChild(i)->getStringValue();
      for(int j = 0; j < (int)data.size(); ++j)
      {
        switch(data[j])
        {
          case '"':
            output << "\\\"";
            break;
          case '\\':
            output << "\\\\";
            break;
          default:
            output << data[i];
            break;
        }
      }

      output << '"';
    }

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
writeNode( std::ostream &output,
           const SGPropertyNode * node,
           bool write_all,
           int indent,
           SGPropertyNode::Attribute archive_flag )
{
  // Don't write the node or any of
  // its descendants unless it is
  // allowed to be archived.
  if (!write_all && !isArchivable(node, archive_flag))
    return true; // Everything's OK, but we won't write.

  const string name = node->getName();
  int nChildren = node->nChildren();
  const SGPropertyNode* attr_node = node->getChild(ATTR, 0);
  bool attr_written = false,
       node_has_value = false;

  // If there is a literal value,
  // write it first.
  if (node->hasValue() && (write_all || node->getAttribute(archive_flag))) {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node, nChildren != 0, attr_node);
    attr_written = true;
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
  if( nChildren > (attr_node ? 1 : 0) )
  {
    doIndent(output, indent);
    output << '<' << name;
    writeAtts(output, node, node_has_value, attr_written ? attr_node : NULL);
    output << '>' << endl;
    for(int i = 0; i < nChildren; i++)
    {
      if( node->getChild(i)->getNameString() != ATTR )
        writeNode( output,
                   node->getChild(i),
                   write_all,
                   indent + INDENT_STEP,
                   archive_flag );
    }
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
writeProperties (const SGPath &path, const SGPropertyNode * start_node,
                 bool write_all, SGPropertyNode::Attribute archive_flag)
{
  SGPath dpath(path);
  dpath.create_dir(0755);

  sg_ofstream output(path);
  if (output.good()) {
    writeProperties(output, start_node, write_all, archive_flag);
  } else {
    throw sg_io_exception("Cannot open file", sg_location(path.utf8Str()), "", false);
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


bool
copyPropertyValue(const SGPropertyNode *in, SGPropertyNode *out)
{
    using namespace simgear;
    bool retval = true;
    
    if (!in->hasValue()) {
        return true;
    }
    
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
            throw sg_error(message, SG_ORIGIN, false);
    }
    
    return retval;
}

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
  bool retval = copyPropertyValue(in, out);
  if (!retval) {
    return false;
  }
    
  // copy the attributes.
  out->setAttributes( in->getAttributes() );

  // Next, copy the children.
  int nChildren = in->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * in_child = in->getChild(i);
    SGPropertyNode * out_child = out->getChild(in_child->getNameString(),
                             in_child->getIndex(),
                             false);
      if (!out_child)
      {
          out_child = out->getChild(in_child->getNameString(),
                                    in_child->getIndex(),
                                    true);
      }

      if (out_child && !copyProperties(in_child, out_child))
          retval = false;
  }

  return retval;
}


bool
copyPropertiesWithAttribute(const SGPropertyNode *in, SGPropertyNode *out,
                             SGPropertyNode::Attribute attr)
{
    bool retval = copyPropertyValue(in, out);
    if (!retval) {
        return false;
    }
    out->setAttributes( in->getAttributes() );
    
    // if attribute is set directly on this node, we don't require it on
    // descendent nodes. (Allows setting an attribute on an entire sub-tree
    // of nodes)
    if ((attr != SGPropertyNode::NO_ATTR) && out->getAttribute(attr)) {
        attr = SGPropertyNode::NO_ATTR;
    }
    
    int nChildren = in->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* in_child = in->getChild(i);
        if ((attr != SGPropertyNode::NO_ATTR) && !isArchivable(in_child, attr))
            continue;
        
         SGPropertyNode* out_child = out->getChild(in_child->getNameString(),
                                      in_child->getIndex(),
                                      true);
        
        bool ok = copyPropertiesWithAttribute(in_child, out_child, attr);
        if (!ok) {
            return false;
        }
    }// of children iteration
    
    return true;
}

namespace {

// for normal recursion we want to call the predicate *before* creating children
bool _inner_copyPropertiesIf(const SGPropertyNode *in, SGPropertyNode *out,
                        PropertyPredicate predicate)
{
    bool retval = copyPropertyValue(in, out);
    if (!retval) {
        return false;
    }
    out->setAttributes( in->getAttributes() );
    
    int nChildren = in->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* in_child = in->getChild(i);
        // skip this child
        if (!predicate(in_child)) {
          continue;
        }

        SGPropertyNode* out_child = out->getChild(in_child->getNameString(),
                                      in_child->getIndex(),
                                      true);
        
        bool ok = copyPropertiesIf(in_child, out_child, predicate);
        if (!ok) {
            return false;
        }
    } // of children iteration
    return true;
}

} // of anonymous namespace

bool copyPropertiesIf(const SGPropertyNode *in, SGPropertyNode *out,
                        PropertyPredicate predicate)
{
  // allow the *entire* copy to be a no-op
    bool doCopy = predicate(in);
    if (!doCopy)
      return true; // doesn't count as failure

   return _inner_copyPropertiesIf(in, out, predicate);
}
// end of props_io.cxx
