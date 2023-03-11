/**
 * \file easyxml.cxx - implementation of EasyXML interfaces.
 * Written by David Megginson, 2000-2001
 * This file is in the Public Domain, and comes with NO WARRANTY of any kind.
 */

#include <string.h>		// strcmp()

#include "xml.h"
#include "easyxml.hxx"
         
#include <fstream>
#include <iostream>

#include <simgear/misc/sg_path.hxx>

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
        column = xmlErrorGetColumnNo(parser, 0);
        line = xmlErrorGetLineNo(parser, 1);
    }
}

////////////////////////////////////////////////////////////////////////
// Implementation of XMLReader.
////////////////////////////////////////////////////////////////////////
#define MAX_NAME_BUF	256
static void
processXML(xmlId *rid, XMLVisitor &visitor)
{
    int num = xmlNodeGetNum(rid, "*");
    if (num) /* elements */
    {
        xmlId *xid = xmlMarkId(rid);
        for (int i=0; i<num; i++)
        {
            if (xmlNodeGetPos(rid, xid, "*", i) != 0)
            {
                if (xmlNodeTest(xid, XML_COMMENT)) continue;

                ZXMLAttributes atts;
                for (int j=0; j<xmlAttributeGetNum(xid); ++j)
                {
                    char value[MAX_NAME_BUF+1];
                    char attr[MAX_NAME_BUF+1];

                    xmlAttributeCopyName(xid, attr, MAX_NAME_BUF, j);
                    xmlAttributeCopyString(xid, attr, value, MAX_NAME_BUF);
                    atts.add(attr, value);
                }

                char name[MAX_NAME_BUF+1];
                int res = xmlNodeCopyName(xid, name, MAX_NAME_BUF);
                if (res)
                {
                    visitor.startElement(name, atts);

                    processXML(xid, visitor);

                    visitor.endElement(name);
                }
            }
        }
        free(xid);
    }
    else /* data */
    {
        char *s = xmlGetString(rid);
        if (s)
        {
            visitor.data(s, strlen(s));
            free(s);
        }
    }
}
#undef MAX_NAME_BUF

XML_API void XML_APIENTRY
readXML (std::istream &input, XMLVisitor &visitor, const std::string &path)
{
    // get length of file
    input.seekg (0, input.end);
    size_t fsize = input.tellg();
    input.seekg (0, input.beg);

    // Set exceptions to be thrown on failure
    input.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string buf;
    try {

        char tmp[16384];
        while (fsize) {

            size_t len = (fsize < 16384) ? fsize : 16384;
            input.read(tmp, len);
            buf.append(tmp, input.gcount());
            fsize -= len;
        }

    } catch (const std::system_error& ex) {
        throw ex;
    }

    xmlId *xid = xmlInitBuffer(buf.data(), buf.length());
    if (!xid) {
        // TODO: report line and column number
        std::string s = xmlErrorGetString(xid, XML_TRUE);
        throw std::runtime_error(s + " opening XML buffer: " + path);
    }

    visitor.setParser(xid);
    visitor.setPath(path);
    visitor.startXML();

    processXML(xid, visitor);

    visitor.endXML();
}

XML_API void XML_APIENTRY
readXML (const SGPath &path, XMLVisitor &visitor)
{
    xmlId *xid = xmlOpen(path.utf8Str().c_str());
    if (!xid) {
        // TODO: report line and column number
        std::string s = xmlErrorGetString(xid, XML_TRUE);
        throw std::runtime_error(s + " reading file: " + path.utf8Str());
    }

    visitor.setParser(xid);
    visitor.setPath(path.utf8Str());
    visitor.startXML();

    processXML(xid, visitor);

    visitor.endXML();
}

XML_API void XML_APIENTRY
readXML (const char *buf, const int size, XMLVisitor &visitor)
{
    xmlId *xid = xmlInitBuffer(buf, size);
    if (!xid) {
        // TODO: report line and column number
        std::string s = xmlErrorGetString(xid, XML_TRUE);
        throw std::runtime_error("Error opening XML buffer:" + s);
    }

    visitor.setParser(xid);
    visitor.startXML();

    processXML(xid, visitor);

    visitor.endXML();
}

// end of easyxml.cxx
