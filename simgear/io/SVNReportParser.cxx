// SVNReportParser -- parser for SVN report XML data
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

#include "SVNReportParser.hxx"

#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <fstream>

#include <boost/foreach.hpp>

#include "simgear/misc/sg_path.hxx"
#include "simgear/misc/sg_dir.hxx"
#include "simgear/debug/logstream.hxx"
#include "simgear/xml/xmlparse.h"
#include "simgear/xml/easyxml.hxx"
#include "simgear/misc/strutils.hxx"
#include "simgear/package/md5.h"

#include "SVNDirectory.hxx"
#include "SVNRepository.hxx"
#include "DAVMultiStatus.hxx"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using namespace simgear;

#define DAV_NS "DAV::"
#define SVN_NS "svn::"
#define SUBVERSION_DAV_NS "http://subversion.tigris.org/xmlns/dav/"

namespace {
    
    #define MAX_ENCODED_INT_LEN 10
    
    static size_t
    decode_size(unsigned char* &p,
                const unsigned char *end)
    {
      if (p + MAX_ENCODED_INT_LEN < end)
        end = p + MAX_ENCODED_INT_LEN;
      /* Decode bytes until we're done.  */
      size_t result = 0;
      
      while (p < end) {
          result = (result << 7) | (*p & 0x7f);
          if (((*p++ >> 7) & 0x1) == 0) {
              break;
          }
      }
      
      return result;
    }
    
    static bool
    try_decode_size(unsigned char* &p,
                const unsigned char *end)
    {
      if (p + MAX_ENCODED_INT_LEN < end)
        end = p + MAX_ENCODED_INT_LEN;

      while (p < end) {
          if (((*p++ >> 7) & 0x1) == 0) {
              return true;
          }
      }
      
      return false;
    }
    
//  const char* SVN_UPDATE_REPORT_TAG = SVN_NS "update-report";
 // const char* SVN_TARGET_REVISION_TAG = SVN_NS "target-revision";
  const char* SVN_OPEN_DIRECTORY_TAG = SVN_NS "open-directory";
  const char* SVN_OPEN_FILE_TAG = SVN_NS "open-file";
  const char* SVN_ADD_DIRECTORY_TAG = SVN_NS "add-directory";
  const char* SVN_ADD_FILE_TAG = SVN_NS "add-file";
  const char* SVN_TXDELTA_TAG = SVN_NS "txdelta";
  const char* SVN_SET_PROP_TAG = SVN_NS "set-prop";
  const char* SVN_DELETE_ENTRY_TAG = SVN_NS "delete-entry";
  
  const char* SVN_DAV_MD5_CHECKSUM = SUBVERSION_DAV_NS ":md5-checksum";
  
  const char* DAV_HREF_TAG = DAV_NS "href";
  const char* DAV_CHECKED_IN_TAG = SVN_NS "checked-in";
  

  const int svn_txdelta_source = 0;
  const int svn_txdelta_target = 1;
  const int svn_txdelta_new = 2;

  const size_t DELTA_HEADER_SIZE = 4;
    
  /**
   * helper struct to decode and store the SVN delta header
   * values
   */
  struct SVNDeltaWindow
  {
  public:

      static bool isWindowComplete(unsigned char* buffer, size_t bytes)
      {
          unsigned char* p = buffer;
          unsigned char* pEnd = p + bytes;
          // if we can't decode five sizes, certainly incomplete
          for (int i=0; i<5; i++) {
              if (!try_decode_size(p, pEnd)) {
                  return false;
              }
          }
          
          p = buffer;
        // ignore these three
          decode_size(p, pEnd);
          decode_size(p, pEnd);
          decode_size(p, pEnd);
          size_t instructionLen = decode_size(p, pEnd);
          size_t newLength = decode_size(p, pEnd);
          size_t headerLength = p - buffer;
          
          return (bytes >= (instructionLen + newLength + headerLength));
      }
      
     SVNDeltaWindow(unsigned char* p) :
         headerLength(0),
         _ptr(p)
     {
         sourceViewOffset = decode_size(p, p+20);
         sourceViewLength = decode_size(p, p+20);
         targetViewLength = decode_size(p, p+20);
         instructionLength = decode_size(p, p+20);
         newLength = decode_size(p, p+20);
         
         headerLength = p - _ptr;
         _ptr = p;
     }
  
    bool apply(std::vector<char>& output, std::istream& source)
    {
        unsigned char* pEnd = _ptr + instructionLength;
        unsigned char* newData = pEnd;
        
        while (_ptr < pEnd) {
          int op = ((*_ptr >> 6) & 0x3);  
          if (op >= 3) {
	    SG_LOG(SG_IO, SG_INFO, "SVNDeltaWindow: bad opcode:" << op);
              return false;
          }
      
          int length = *_ptr++ & 0x3f;
          int offset = 0;
        
          if (length == 0) {
            length = decode_size(_ptr, pEnd);
          }
        
          if (length == 0) {
			SG_LOG(SG_IO, SG_INFO, "SVNDeltaWindow: malformed stream, 0 length" << op);
            return false;
          }
      
          // if op != new, decode another size value
          if (op != svn_txdelta_new) {
            offset = decode_size(_ptr, pEnd);
          }

          if (op == svn_txdelta_target) {
            while (length > 0) {
              output.push_back(output[offset++]);
              --length;
            }
          } else if (op == svn_txdelta_new) {
              output.insert(output.end(), newData, newData + length);
          } else if (op == svn_txdelta_source) {
            source.seekg(offset);
            char* sourceBuf = (char*) malloc(length);
            assert(sourceBuf);
            source.read(sourceBuf, length);
            output.insert(output.end(), sourceBuf, sourceBuf + length);
            free(sourceBuf);
          }
        } // of instruction loop
        
        return true;
    }  
  
    size_t size() const
    {
        return headerLength + instructionLength + newLength;
    }
  
    unsigned int sourceViewOffset;
    size_t sourceViewLength,
      targetViewLength;
    size_t headerLength,
        instructionLength,
        newLength;
    
private:  
    unsigned char* _ptr;
  };
  
  
} // of anonymous namespace

class SVNReportParser::SVNReportParserPrivate
{
public:
  SVNReportParserPrivate(SVNRepository* repo) :
    tree(repo),
    status(SVNRepository::SVN_NO_ERROR),
    parserInited(false),
    currentPath(repo->fsBase())
  {
    inFile = false;
    currentDir = repo->rootDir();
  }

  ~SVNReportParserPrivate()
  {
  }
  
  void startElement (const char * name, const char** attributes)
  {    
      if (status != SVNRepository::SVN_NO_ERROR) {
          return;
      }
      
    ExpatAtts attrs(attributes);
    tagStack.push_back(name);
    if (!strcmp(name, SVN_TXDELTA_TAG)) {
        txDeltaData.clear();
    } else if (!strcmp(name, SVN_ADD_FILE_TAG)) {
      string fileName(attrs.getValue("name"));
      SGPath filePath(currentDir->fsDir().file(fileName));
      currentPath = filePath;
      inFile = true;
    } else if (!strcmp(name, SVN_OPEN_FILE_TAG)) {
      string fileName(attrs.getValue("name"));
      SGPath filePath(Dir(currentPath).file(fileName));
      currentPath = filePath;
      
      DAVResource* res = currentDir->collection()->childWithName(fileName);   
      if (!res || !filePath.exists()) {
        // set error condition
      }
      
      inFile = true;
    } else if (!strcmp(name, SVN_ADD_DIRECTORY_TAG)) {
      string dirName(attrs.getValue("name"));
      Dir d(currentDir->fsDir().file(dirName));
      if (d.exists()) {
          // policy decision : if we're doing an add, wipe the existing
          d.remove(true);
      }
    
      currentDir = currentDir->addChildDirectory(dirName);
      currentPath = currentDir->fsPath();
      currentDir->beginUpdateReport();
      //cout << "addDir:" << currentPath << endl;
    } else if (!strcmp(name, SVN_SET_PROP_TAG)) {
      setPropName = attrs.getValue("name");
      setPropValue.clear();
    } else if (!strcmp(name, SVN_DAV_MD5_CHECKSUM)) {
      md5Sum.clear();
    } else if (!strcmp(name, SVN_OPEN_DIRECTORY_TAG)) {
        string dirName;
        if (attrs.getValue("name")) {
            dirName = string(attrs.getValue("name"));
        }
        openDirectory(dirName);
    } else if (!strcmp(name, SVN_DELETE_ENTRY_TAG)) {
        string entryName(attrs.getValue("name"));
        deleteEntry(entryName);
    } else if (!strcmp(name, DAV_CHECKED_IN_TAG) || !strcmp(name, DAV_HREF_TAG)) {
        // don't warn on these ones
    } else {
        //std::cerr << "unhandled element:" << name << std::endl;
    }
  } // of startElement
  
  void openDirectory(const std::string& dirName)
  {
      if (dirName.empty()) {
          // root directory, we shall assume
          currentDir = tree->rootDir();
      } else {
          assert(currentDir);
          currentDir = currentDir->child(dirName);
      }
      
      assert(currentDir);
      currentPath = currentDir->fsPath();
      currentDir->beginUpdateReport();
  }
  
  void deleteEntry(const std::string& entryName)
  {
      currentDir->deleteChildByName(entryName);
  }
  
  bool decodeTextDelta(const SGPath& outputPath)
  {    
    string decoded = strutils::decodeBase64(txDeltaData);
    size_t bytesToDecode = decoded.size();
    std::vector<char> output;      
    unsigned char* p = (unsigned char*) decoded.data();
    if (memcmp(p, "SVN\0", DELTA_HEADER_SIZE) != 0) {
        return false; // bad header
    }
    
    bytesToDecode -= DELTA_HEADER_SIZE;
    p += DELTA_HEADER_SIZE;
    std::ifstream source;
    source.open(outputPath.c_str(), std::ios::in | std::ios::binary);
    
    while (bytesToDecode > 0) {  
        if (!SVNDeltaWindow::isWindowComplete(p, bytesToDecode)) {
	  SG_LOG(SG_IO, SG_WARN, "SVN txdelta broken window");
	  return false;
	}
	
        SVNDeltaWindow window(p);      
        assert(bytesToDecode >= window.size());
        window.apply(output, source);
        bytesToDecode -= window.size();
        p += window.size();
    }

    source.close();

    std::ofstream f;
    f.open(outputPath.c_str(), 
      std::ios::out | std::ios::trunc | std::ios::binary);
    f.write(output.data(), output.size());

    // compute MD5 while we have the file in memory
    memset(&md5Context, 0, sizeof(SG_MD5_CTX));
    SG_MD5Init(&md5Context);
    SG_MD5Update(&md5Context, (unsigned char*) output.data(), output.size());
    SG_MD5Final(&md5Context);
    decodedFileMd5 = strutils::encodeHex(md5Context.digest, 16);

    return true;
  }
  
  void endElement (const char * name)
  {
      if (status != SVNRepository::SVN_NO_ERROR) {
          return;
      }
      
    assert(tagStack.back() == name);
    tagStack.pop_back();
        
    if (!strcmp(name, SVN_TXDELTA_TAG)) {
      if (!decodeTextDelta(currentPath)) {
        fail(SVNRepository::SVN_ERROR_TXDELTA);
      }
    } else if (!strcmp(name, SVN_ADD_FILE_TAG)) {
      finishFile(currentDir->addChildFile(currentPath.file()));
    } else if (!strcmp(name, SVN_OPEN_FILE_TAG)) {
      DAVResource* res = currentDir->collection()->childWithName(currentPath.file());   
      assert(res);
      finishFile(res);
    } else if (!strcmp(name, SVN_ADD_DIRECTORY_TAG)) {
      // pop directory
      currentPath = currentPath.dir();
      currentDir->updateReportComplete();
      currentDir = currentDir->parent();
    } else if (!strcmp(name, SVN_SET_PROP_TAG)) {
      if (setPropName == "svn:entry:committed-rev") {
        revision = strutils::to_int(setPropValue);
        currentVersionName = setPropValue;
        if (!inFile) {
          // for directories we have the resource already
          // for adding files, we might not; we set the version name
          // above when ending the add/open-file element
          currentDir->collection()->setVersionName(currentVersionName);
        } 
      }
    } else if (!strcmp(name, SVN_DAV_MD5_CHECKSUM)) {
      // validate against (presumably) just written file
      if (decodedFileMd5 != md5Sum) {
        fail(SVNRepository::SVN_ERROR_CHECKSUM);
      }
    } else if (!strcmp(name, SVN_OPEN_DIRECTORY_TAG)) {
        if (currentDir->parent()) {   
          // pop the collection stack
          currentDir = currentDir->parent();
        }
        
        currentDir->updateReportComplete();
        currentPath = currentDir->fsPath();
    } else {
    //  std::cout << "element:" << name;
    }
  }
  
  void finishFile(DAVResource* res)
  {
      res->setVersionName(currentVersionName);
      res->setMD5(md5Sum);
      currentPath = currentPath.dir();
      inFile = false;
  }
  
  void data (const char * s, int length)
  {
      if (status != SVNRepository::SVN_NO_ERROR) {
          return;
      }
      
    if (tagStack.back() == SVN_SET_PROP_TAG) {
      setPropValue += string(s, length);
    } else if (tagStack.back() == SVN_TXDELTA_TAG) {
      txDeltaData += string(s, length);
    } else if (tagStack.back() == SVN_DAV_MD5_CHECKSUM) {
      md5Sum += string(s, length);
    }
  }
  
  void pi (const char * target, const char * data) {}
  
  string tagN(const unsigned int n) const
  {
    size_t sz = tagStack.size();
    if (n >= sz) {
      return string();
    }
    
    return tagStack[sz - (1 + n)];
  }
  
  void fail(SVNRepository::ResultCode err)
  {
      status = err;
  }
  
  SVNRepository* tree;
  DAVCollection* rootCollection;
  SVNDirectory* currentDir;
  SVNRepository::ResultCode status;
  
  bool parserInited;
  XML_Parser xmlParser;
  
// in-flight data
  string_list tagStack;
  string currentVersionName;
  string txDeltaData;
  SGPath currentPath;
  bool inFile;
    
  unsigned int revision;
  SG_MD5_CTX md5Context;
  string md5Sum, decodedFileMd5;
  std::string setPropName, setPropValue;
};


////////////////////////////////////////////////////////////////////////
// Static callback functions for Expat.
////////////////////////////////////////////////////////////////////////

#define VISITOR static_cast<SVNReportParser::SVNReportParserPrivate *>(userData)

static void
start_element (void * userData, const char * name, const char ** atts)
{
  VISITOR->startElement(name, atts);
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

SVNReportParser::SVNReportParser(SVNRepository* repo) :
  _d(new SVNReportParserPrivate(repo))
{
  
}

SVNReportParser::~SVNReportParser()
{
}

SVNRepository::ResultCode
SVNReportParser::innerParseXML(const char* data, int size)
{
    if (_d->status != SVNRepository::SVN_NO_ERROR) {
        return _d->status;
    }
    
    bool isEnd = (data == NULL);
    if (!XML_Parse(_d->xmlParser, data, size, isEnd)) {
      SG_LOG(SG_IO, SG_INFO, "SVN parse error:" << XML_ErrorString(XML_GetErrorCode(_d->xmlParser))
             << " at line:" << XML_GetCurrentLineNumber(_d->xmlParser)
             << " column " << XML_GetCurrentColumnNumber(_d->xmlParser));
    
      XML_ParserFree(_d->xmlParser);
      _d->parserInited = false;
      return SVNRepository::SVN_ERROR_XML;
    } else if (isEnd) {
        XML_ParserFree(_d->xmlParser);
        _d->parserInited = false;
    }

    return _d->status;
}

SVNRepository::ResultCode
SVNReportParser::parseXML(const char* data, int size)
{
    if (_d->status != SVNRepository::SVN_NO_ERROR) {
        return _d->status;
    }
    
  if (!_d->parserInited) {
    _d->xmlParser = XML_ParserCreateNS(0, ':');
    XML_SetUserData(_d->xmlParser, _d.get());
    XML_SetElementHandler(_d->xmlParser, start_element, end_element);
    XML_SetCharacterDataHandler(_d->xmlParser, character_data);
    XML_SetProcessingInstructionHandler(_d->xmlParser, processing_instruction);
    _d->parserInited = true;
  }
  
  return innerParseXML(data, size);
}

SVNRepository::ResultCode SVNReportParser::finishParse()
{
    if (_d->status != SVNRepository::SVN_NO_ERROR) {
        return _d->status;
    }
    
    return innerParseXML(NULL, 0);
}

std::string SVNReportParser::etagFromRevision(unsigned int revision)
{
  // etags look like W/"7//", hopefully this is stable
  // across different servers and similar
  std::ostringstream os;
  os << "W/\"" << revision << "//";
  return os.str();
}


