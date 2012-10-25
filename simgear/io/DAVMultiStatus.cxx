// DAVMultiStatus.cxx -- parser for WebDAV MultiStatus XML data
//
// Copyright (C) 2012  James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "DAVMultiStatus.hxx"

#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sstream>

#include <boost/foreach.hpp>

#include "simgear/debug/logstream.hxx"
#include "simgear/xml/xmlparse.h"
#include "simgear/misc/strutils.hxx"
#include "simgear/structure/exception.hxx"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using namespace simgear;

#define DAV_NS "DAV::"
#define SUBVERSION_DAV_NS "http://subversion.tigris.org/xmlns/dav/"

const char* DAV_MULTISTATUS_TAG = DAV_NS "multistatus";
const char* DAV_RESPONSE_TAG = DAV_NS "response";
const char* DAV_PROPSTAT_TAG = DAV_NS "propstat";
const char* DAV_PROP_TAG = DAV_NS "prop";

const char* DAV_HREF_TAG = DAV_NS "href";
const char* DAV_RESOURCE_TYPE_TAG = DAV_NS "resourcetype";
const char* DAV_CONTENT_TYPE_TAG = DAV_NS "getcontenttype";
const char* DAV_CONTENT_LENGTH_TAG = DAV_NS "getcontentlength";
const char* DAV_VERSIONNAME_TAG = DAV_NS "version-name";
const char* DAV_COLLECTION_TAG = DAV_NS "collection";
const char* DAV_VCC_TAG = DAV_NS "version-controlled-configuration";

const char* SUBVERSION_MD5_CHECKSUM_TAG = SUBVERSION_DAV_NS ":md5-checksum";

DAVResource::DAVResource(const string& href) :
  _type(Unknown),
  _url(href),
  _container(NULL)
{
    assert(!href.empty()); 
}

void DAVResource::setVersionName(const std::string& aVersion)
{
  _versionName = aVersion;
}

void DAVResource::setVersionControlledConfiguration(const std::string& vcc)
{
    _vcc = vcc;
}

void DAVResource::setMD5(const std::string& md5Hex)
{
    _md5 = md5Hex;
}

std::string DAVResource::name() const
{
    string::size_type index = _url.rfind('/');
    if (index != string::npos) {
        return _url.substr(index + 1);
    }
    
    throw sg_exception("bad DAV resource HREF:" + _url);
}

////////////////////////////////////////////////////////////////////////////

DAVCollection::DAVCollection(const string& href) :
  DAVResource(href)
{
  _type = DAVResource::Collection;
}

DAVCollection::~DAVCollection()
{
  BOOST_FOREACH(DAVResource* c, _contents) {
    delete c;
  }
}

void DAVCollection::addChild(DAVResource *res)
{
  assert(res);
  if (res->container() == this) {
    return;
  }
  
  assert(res->container() == NULL);
  assert(std::find(_contents.begin(), _contents.end(), res) == _contents.end());
  
  if (!strutils::starts_with(res->url(), _url)) {
      std::cerr << "us: " << _url << std::endl;
      std::cerr << "child:" << res->url() << std::endl;
      
  }
  
  assert(strutils::starts_with(res->url(), _url));
  assert(childWithUrl(res->url()) == NULL);
  
  res->_container = this;
  _contents.push_back(res);
}

void DAVCollection::removeChild(DAVResource* res)
{
  assert(res);
  assert(res->container() == this);
  
  res->_container = NULL;
  DAVResourceList::iterator it = std::find(_contents.begin(), _contents.end(), res);
  assert(it != _contents.end());
  _contents.erase(it);
}

DAVCollection*
DAVCollection::createChildCollection(const std::string& name)
{
    DAVCollection* child = new DAVCollection(urlForChildWithName(name));
    addChild(child);
    return child;
}

DAVResourceList DAVCollection::contents() const
{
  return _contents;
}

DAVResource* DAVCollection::childWithUrl(const string& url) const
{
  if (url.empty())
    return NULL;
  
  BOOST_FOREACH(DAVResource* c, _contents) {
    if (c->url() == url) {
      return c;
    }
  }
  
  return NULL;
}

DAVResource* DAVCollection::childWithName(const string& name) const
{
  return childWithUrl(urlForChildWithName(name));
}

std::string DAVCollection::urlForChildWithName(const std::string& name) const
{
  return url() + "/" + name;
}

///////////////////////////////////////////////////////////////////////////////

class DAVMultiStatus::DAVMultiStatusPrivate
{
public:
  DAVMultiStatusPrivate() :
  parserInited(false)
  {
    rootResource = NULL;
  }
  
  void startElement (const char * name)
  {
    if (tagStack.empty()) {
      if (strcmp(name, DAV_MULTISTATUS_TAG)) {
        SG_LOG(SG_IO, SG_WARN, "root element is not " <<
               DAV_MULTISTATUS_TAG << ", got:" << name);
      } else {
        
      }
    } else {
      // not at the root element
      if (tagStack.back() == DAV_MULTISTATUS_TAG) {
        if (strcmp(name, DAV_RESPONSE_TAG)) {
          SG_LOG(SG_IO, SG_WARN, "multistatus child is not response: saw:"
                 << name);
        }
      }
      
      if (tagStack.back() == DAV_RESOURCE_TYPE_TAG) {
        if (!strcmp(name, DAV_COLLECTION_TAG)) {
          currentElementType = DAVResource::Collection;
        } else {
          currentElementType = DAVResource::Unknown;
        }
      }
    }
    
    tagStack.push_back(name);
    if (!strcmp(name, DAV_RESPONSE_TAG)) {
      currentElementType = DAVResource::Unknown;
      currentElementUrl.clear();
      currentElementMD5.clear();
      currentVersionName.clear();
      currentVCC.clear();
    }
  }
  
  void endElement (const char * name)
  {
    assert(tagStack.back() == name);
    tagStack.pop_back();
    
    if (!strcmp(name, DAV_RESPONSE_TAG)) {
      // finish complete response
      currentElementUrl = strutils::strip(currentElementUrl);
      
      DAVResource* res = NULL;
      if (currentElementType == DAVResource::Collection) {
        DAVCollection* col = new DAVCollection(currentElementUrl);
        res = col;
      } else {
        res = new DAVResource(currentElementUrl);
      }
      
      res->setVersionName(strutils::strip(currentVersionName));
      res->setVersionControlledConfiguration(currentVCC);
      if (rootResource &&
          strutils::starts_with(currentElementUrl, rootResource->url()))
      {
        static_cast<DAVCollection*>(rootResource)->addChild(res);
      }
      
      if (!rootResource) {
        rootResource = res;
      }
    }
  }
  
  void data (const char * s, int length)
  {
    if (tagStack.back() == DAV_HREF_TAG) {
      if (tagN(1) == DAV_RESPONSE_TAG) {
        currentElementUrl += string(s, length);
      } else if (tagN(1) == DAV_VCC_TAG) {
        currentVCC += string(s, length);
      }
    } else if (tagStack.back() == SUBVERSION_MD5_CHECKSUM_TAG) {
      currentElementMD5 = string(s, length);
    } else if (tagStack.back() == DAV_VERSIONNAME_TAG) {
      currentVersionName = string(s, length);
    } else if (tagStack.back() == DAV_CONTENT_LENGTH_TAG) {
      std::istringstream is(string(s, length));
      is >> currentElementLength;
    }
  }
  
  void pi (const char * target, const char * data) {}
  
  string tagN(const unsigned int n) const
  {
    int sz = tagStack.size();
    if (n >= sz) {
      return string();
    }
    
    return tagStack[sz - (1 + n)];
  }
  
  bool parserInited;
  XML_Parser xmlParser;
  DAVResource* rootResource;
  
  // in-flight data
  string_list tagStack;
  DAVResource::Type currentElementType;
  string currentElementUrl,
    currentVersionName,
    currentVCC;
  int currentElementLength;
  string currentElementMD5;
};


////////////////////////////////////////////////////////////////////////
// Static callback functions for Expat.
////////////////////////////////////////////////////////////////////////

#define VISITOR static_cast<DAVMultiStatus::DAVMultiStatusPrivate *>(userData)

static void
start_element (void * userData, const char * name, const char ** atts)
{
  VISITOR->startElement(name);
}

static void
end_element (void * userData, const char * name)
{
  VISITOR->endElement(name);
}

static void
character_data (void * userData, const char * s, int len)
{
  VISITOR->data(s, len);
}

static void
processing_instruction (void * userData,
                        const char * target,
                        const char * data)
{
  VISITOR->pi(target, data);
}

#undef VISITOR

///////////////////////////////////////////////////////////////////////////////

DAVMultiStatus::DAVMultiStatus() :
_d(new DAVMultiStatusPrivate)
{
  
}

DAVMultiStatus::~DAVMultiStatus()
{
  
}

void DAVMultiStatus::parseXML(const char* data, int size)
{
  if (!_d->parserInited) {
    _d->xmlParser = XML_ParserCreateNS(0, ':');
    XML_SetUserData(_d->xmlParser, _d.get());
    XML_SetElementHandler(_d->xmlParser, start_element, end_element);
    XML_SetCharacterDataHandler(_d->xmlParser, character_data);
    XML_SetProcessingInstructionHandler(_d->xmlParser, processing_instruction);
    _d->parserInited = true;
  }
  
  if (!XML_Parse(_d->xmlParser, data, size, false)) {
    SG_LOG(SG_IO, SG_WARN, "DAV parse error:" << XML_ErrorString(XML_GetErrorCode(_d->xmlParser))
           << " at line:" << XML_GetCurrentLineNumber(_d->xmlParser)
           << " column " << XML_GetCurrentColumnNumber(_d->xmlParser));
    
    XML_ParserFree(_d->xmlParser);
    _d->parserInited = false;
  }
}

void DAVMultiStatus::finishParse()
{
  if (_d->parserInited) {
    XML_Parse(_d->xmlParser, NULL, 0, true);
    XML_ParserFree(_d->xmlParser);
  }
  
  _d->parserInited = false;
}

DAVResource* DAVMultiStatus::resource()
{
  return _d->rootResource;
}


