// easyxml.cxx - implementation of EasyXML interfaces.

#include <simgear/compiler.h>

#include <string.h>		// strcmp()

#include "easyxml.hxx"
#include "xmlparse.h"

#include STL_FSTREAM
#include STL_IOSTREAM

SG_USING_STD(ifstream);



////////////////////////////////////////////////////////////////////////
// Implementation of XMLAttributes.
////////////////////////////////////////////////////////////////////////

XMLAttributes::XMLAttributes ()
{
}

XMLAttributes::~XMLAttributes ()
{
}

int
XMLAttributes::findAttribute (const char * name) const
{
  int s = size();
  for (int i = 0; i < s; i++) {
    if (strcmp(name, getName(i)) == 0)
      return i;
  }
  return -1;
}

bool
XMLAttributes::hasAttribute (const char * name) const
{
  return (findAttribute(name) != -1);
}

const char *
XMLAttributes::getValue (const char * name) const
{
  int pos = findAttribute(name);
  if (pos >= 0)
    return getValue(pos);
  else
    return 0;
}


////////////////////////////////////////////////////////////////////////
// Implementation of XMLAttributesDefault.
////////////////////////////////////////////////////////////////////////

XMLAttributesDefault::XMLAttributesDefault ()
{
}

XMLAttributesDefault::XMLAttributesDefault (const XMLAttributes &atts)
{
  int s = atts.size();
  for (int i = 0; i < s; i++)
    addAttribute(atts.getName(i), atts.getValue(i));
}

XMLAttributesDefault::~XMLAttributesDefault ()
{
}

int
XMLAttributesDefault::size () const
{
  return _atts.size() / 2;
}

const char *
XMLAttributesDefault::getName (int i) const
{
  return _atts[i*2].c_str();
}

const char *
XMLAttributesDefault::getValue (int i) const
{
  return _atts[i*2+1].c_str();
}

void
XMLAttributesDefault::addAttribute (const char * name, const char * value)
{
  _atts.push_back(name);
  _atts.push_back(value);
}

void
XMLAttributesDefault::setName (int i, const char * name)
{
  _atts[i*2] = name;
}

void
XMLAttributesDefault::setValue (int i, const char * name)
{
  _atts[i*2+1] = name;
}

void
XMLAttributesDefault::setValue (const char * name, const char * value)
{
  int pos = findAttribute(name);
  if (pos >= 0) {
    setName(pos, name);
    setValue(pos, value);
  } else {
    addAttribute(name, value);
  }
}



////////////////////////////////////////////////////////////////////////
// Attribute list wrapper for Expat.
////////////////////////////////////////////////////////////////////////

class ExpatAtts : public XMLAttributes
{
public:
  ExpatAtts (const char ** atts) : _atts(atts) {}
  
  virtual int size () const;
  virtual const char * getName (int i) const;
  virtual const char * getValue (int i) const;
  
private:
  const char ** _atts;
};

int
ExpatAtts::size () const
{
  int s = 0;
  for (int i = 0; _atts[i] != 0; i += 2)
    s++;
  return s;
}

const char *
ExpatAtts::getName (int i) const
{
  return _atts[i*2];
}

const char *
ExpatAtts::getValue (int i) const
{
  return _atts[i*2+1];
}



////////////////////////////////////////////////////////////////////////
// Static callback functions for Expat.
////////////////////////////////////////////////////////////////////////

#define VISITOR (*((XMLVisitor *)userData))

static void
start_element (void * userData, const char * name, const char ** atts)
{
  VISITOR.startElement(name, ExpatAtts(atts));
}

static void
end_element (void * userData, const char * name)
{
  VISITOR.endElement(name);
}

static void
character_data (void * userData, const char * s, int len)
{
  VISITOR.data(s, len);
}

static void
processing_instruction (void * userData,
			const char * target,
			const char * data)
{
  VISITOR.pi(target, data);
}

#undef VISITOR



////////////////////////////////////////////////////////////////////////
// Implementation of XMLReader.
////////////////////////////////////////////////////////////////////////

void
readXML (istream &input, XMLVisitor &visitor, const string &path)
{
  XML_Parser parser = XML_ParserCreate(0);
  XML_SetUserData(parser, &visitor);
  XML_SetElementHandler(parser, start_element, end_element);
  XML_SetCharacterDataHandler(parser, character_data);
  XML_SetProcessingInstructionHandler(parser, processing_instruction);

  visitor.startXML();

  char buf[16384];
  while (!input.eof()) {

				// FIXME: get proper error string from system
    if (!input.good()) {
      XML_ParserFree(parser);
      throw sg_io_exception("Problem reading file",
			    sg_location(path,
					XML_GetCurrentLineNumber(parser),
					XML_GetCurrentColumnNumber(parser)),
			    "SimGear XML Parser");
    }

    input.read(buf,16384);
    if (!XML_Parse(parser, buf, input.gcount(), false)) {
      XML_ParserFree(parser);
      throw sg_io_exception(XML_ErrorString(XML_GetErrorCode(parser)),
			    sg_location(path,
					XML_GetCurrentLineNumber(parser),
					XML_GetCurrentColumnNumber(parser)),
			    "SimGear XML Parser");
    }

  }

				// Verify end of document.
  if (!XML_Parse(parser, buf, 0, true)) {
    XML_ParserFree(parser);
    throw sg_io_exception(XML_ErrorString(XML_GetErrorCode(parser)),
			  sg_location(path,
				      XML_GetCurrentLineNumber(parser),
				      XML_GetCurrentColumnNumber(parser)),
			  "SimGear XML Parser");
  }

  XML_ParserFree(parser);
}

void
readXML (const string &path, XMLVisitor &visitor)
{
  ifstream input(path.c_str());
  if (input.good()) {
    try {
      readXML(input, visitor, path);
    } catch (sg_io_exception &e) {
      input.close();
      throw e;
    } catch (sg_throwable &t) {
      input.close();
      throw t;
    }
  } else {
    throw sg_io_exception("Failed to open file", sg_location(path),
			  "SimGear XML Parser");
  }
  input.close();
}

void
readXML (const char *buf, const int size, XMLVisitor &visitor)
{
  XML_Parser parser = XML_ParserCreate(0);
  XML_SetUserData(parser, &visitor);
  XML_SetElementHandler(parser, start_element, end_element);
  XML_SetCharacterDataHandler(parser, character_data);
  XML_SetProcessingInstructionHandler(parser, processing_instruction);

  visitor.startXML();

  if (!XML_Parse(parser, buf, size, false)) {
      XML_ParserFree(parser);
      throw sg_io_exception(XML_ErrorString(XML_GetErrorCode(parser)),
			    sg_location("In-memory XML buffer",
					XML_GetCurrentLineNumber(parser),
					XML_GetCurrentColumnNumber(parser)),
			    "SimGear XML Parser");
  }

  XML_ParserFree(parser);
}

// end of easyxml.cxx
