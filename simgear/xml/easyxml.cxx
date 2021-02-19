/**
 * \file easyxml.cxx - implementation of EasyXML interfaces.
 * Written by David Megginson, 2000-2001
 * This file is in the Public Domain, and comes with NO WARRANTY of any kind.
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif
     
#include <simgear/compiler.h>

#include <string.h>		// strcmp()

#include "easyxml.hxx"
     
#ifdef SYSTEM_EXPAT
#  include <expat.h>
#else
#  include "sg_expat.h"     
#endif
     
#include <fstream>
#include <iostream>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

using std::ifstream;
using std::istream;
using std::string;


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


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void XMLVisitor::savePosition(void)
{
  if (parser) {
    column = XML_GetCurrentColumnNumber(parser);
    line = XML_GetCurrentLineNumber(parser);
  }
}

////////////////////////////////////////////////////////////////////////
// Attribute list wrapper for Expat.
////////////////////////////////////////////////////////////////////////

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

const char * 
ExpatAtts::getValue (const char * name) const
{
  return XMLAttributes::getValue(name);
}


////////////////////////////////////////////////////////////////////////
// Static callback functions for Expat.
////////////////////////////////////////////////////////////////////////

#define VISITOR (*((XMLVisitor *)userData))

static void
start_element (void * userData, const char * name, const char ** atts)
{
  VISITOR.savePosition();
  VISITOR.startElement(name, ExpatAtts(atts));
}

static void
end_element (void * userData, const char * name)
{
  VISITOR.savePosition();
  VISITOR.endElement(name);
}

static void
character_data (void * userData, const char * s, int len)
{
  VISITOR.savePosition();
  VISITOR.data(s, len);
}

static void
processing_instruction (void * userData,
			const char * target,
			const char * data)
{
  VISITOR.savePosition();
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

  visitor.setParser(parser);
  visitor.setPath(path);
  visitor.startXML();

  char buf[16384];
  while (!input.eof()) {

				// FIXME: get proper error string from system
    if (!input.good()) {
        sg_io_exception ex("Problem reading file",
                           sg_location(path,
                                       XML_GetCurrentLineNumber(parser),
                                       XML_GetCurrentColumnNumber(parser)),
                           "SimGear XML Parser",
                           false /* don't report */);
        visitor.setParser(0);
        XML_ParserFree(parser);
        throw ex;
    }

    input.read(buf,16384);
    if (!XML_Parse(parser, buf, input.gcount(), false)) {
        sg_io_exception ex(XML_ErrorString(XML_GetErrorCode(parser)),
                           sg_location(path,
                                       XML_GetCurrentLineNumber(parser),
                                       XML_GetCurrentColumnNumber(parser)),
                           "SimGear XML Parser",
                           false /* don't report */);
        visitor.setParser(0);
        XML_ParserFree(parser);
        throw ex;
    }

  }

				// Verify end of document.
  if (!XML_Parse(parser, buf, 0, true)) {
      sg_io_exception ex(XML_ErrorString(XML_GetErrorCode(parser)),
                         sg_location(path,
                                     XML_GetCurrentLineNumber(parser),
                                     XML_GetCurrentColumnNumber(parser)),
                         "SimGear XML Parser",
                         false /* don't report */);
      visitor.setParser(0);
      XML_ParserFree(parser);
      throw ex;
  }

  visitor.setParser(0);
  XML_ParserFree(parser);
  visitor.endXML();
}

void
readXML (const SGPath &path, XMLVisitor &visitor)
{
  sg_ifstream input(path);
  if (input.good()) {
    try {
      readXML(input, visitor, path.utf8Str());
    } catch (sg_io_exception &) {
      input.close();
      throw;
    } catch (sg_throwable &) {
      input.close();
      throw;
    }
  } else {
      throw sg_io_exception("Failed to open file", sg_location(path),
                            "SimGear XML Parser", false /* don't report */);
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
      sg_io_exception ex(XML_ErrorString(XML_GetErrorCode(parser)),
                         sg_location("In-memory XML buffer",
                                     XML_GetCurrentLineNumber(parser),
                                     XML_GetCurrentColumnNumber(parser)),
                         "SimGear XML Parser",
                         false /* don't report */);
      XML_ParserFree(parser);
      throw ex;
  }

  XML_ParserFree(parser);
  visitor.endXML();
}

// end of easyxml.cxx
