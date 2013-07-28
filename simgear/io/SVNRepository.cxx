// DAVMirrorTree -- mirror a DAV tree to the local file-system
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

#include "SVNRepository.hxx"

#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <fstream>

#include <boost/foreach.hpp>

#include "simgear/debug/logstream.hxx"
#include "simgear/misc/strutils.hxx"
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/DAVMultiStatus.hxx>
#include <simgear/io/SVNDirectory.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/SVNReportParser.hxx>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace simgear
{

typedef std::vector<HTTP::Request_ptr> RequestVector;

class SVNRepoPrivate
{
public:
    SVNRepoPrivate(SVNRepository* parent) : 
        p(parent), 
        isUpdating(false),
        status(SVNRepository::SVN_NO_ERROR)
    { ; }
    
    SVNRepository* p; // link back to outer
    SVNDirectory* rootCollection;
    HTTP::Client* http;
    std::string baseUrl;
    std::string vccUrl;
    std::string targetRevision;
    bool isUpdating;
    SVNRepository::ResultCode status;
    
    void svnUpdateDone()
    {
        isUpdating = false;
    }
    
    void updateFailed(HTTP::Request* req, SVNRepository::ResultCode err)
    {
        SG_LOG(SG_IO, SG_WARN, "SVN: failed to update from:" << req->url());
        isUpdating = false;
        status = err;
    }
      
    void propFindComplete(HTTP::Request* req, DAVCollection* col);
    void propFindFailed(HTTP::Request* req, SVNRepository::ResultCode err);
};


namespace { // anonmouse
    
    string makeAbsoluteUrl(const string& url, const string& base)
    {
      if (strutils::starts_with(url, "http://"))
        return url; // already absolute
  
      assert(strutils::starts_with(base, "http://"));
      int schemeEnd = base.find("://");
      int hostEnd = base.find('/', schemeEnd + 3);
      if (hostEnd < 0) {
        return url;
      }
  
      return base.substr(0, hostEnd) + url;
    }
    
    // keep the responses small by only requesting the properties we actually
    // care about; the ETag, length and MD5-sum
    const char* PROPFIND_REQUEST_BODY =
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
      "<D:propfind xmlns:D=\"DAV:\">"
      "<D:prop xmlns:R=\"http://subversion.tigris.org/xmlns/dav/\">"
      "<D:resourcetype/>"
      "<D:version-name/>"
      "<D:version-controlled-configuration/>"
      "</D:prop>"
      "</D:propfind>";

    class PropFindRequest : public HTTP::Request
    {
    public:
      PropFindRequest(SVNRepoPrivate* repo) :
        Request(repo->baseUrl, "PROPFIND"),
        _repo(repo)
      {
      }
  
      virtual string_list requestHeaders() const
      {
        string_list r;
        r.push_back("Depth");
        return r;
      }
  
      virtual string header(const string& name) const
      {
          if (name == "Depth") {
              return "0";
          }
          
          return string();
      }
  
      virtual string requestBodyType() const
      {
          return "application/xml; charset=\"utf-8\"";
      }
  
      virtual int requestBodyLength() const
      {
        return strlen(PROPFIND_REQUEST_BODY);
      }
  
      virtual int getBodyData(char* buf, int count) const
      {
        int bodyLen = strlen(PROPFIND_REQUEST_BODY);
        assert(count >= bodyLen);
        memcpy(buf, PROPFIND_REQUEST_BODY, bodyLen);
        return bodyLen;
      }

    protected:
      virtual void responseHeadersComplete()
      {
        if (responseCode() == 207) {
            // fine
        } else if (responseCode() == 404) {
            _repo->propFindFailed(this, SVNRepository::SVN_ERROR_NOT_FOUND);
        } else {
            SG_LOG(SG_IO, SG_WARN, "request for:" << url() << 
                " return code " << responseCode());
            _repo->propFindFailed(this, SVNRepository::SVN_ERROR_SOCKET);
        }
      }
  
      virtual void responseComplete()
      {
        if (responseCode() == 207) {
          _davStatus.finishParse();
          _repo->propFindComplete(this, (DAVCollection*) _davStatus.resource());
        }
      }
  
      virtual void gotBodyData(const char* s, int n)
      {
        if (responseCode() != 207) {
          return;
        }
        _davStatus.parseXML(s, n);
      }
        
        virtual void failed()
        {
            HTTP::Request::failed();
            _repo->propFindFailed(this, SVNRepository::SVN_ERROR_SOCKET);
        }
        
    private:
      SVNRepoPrivate* _repo;
      DAVMultiStatus _davStatus;
    };

class UpdateReportRequest : public HTTP::Request
{
public:
  UpdateReportRequest(SVNRepoPrivate* repo, 
      const std::string& aVersionName,
      bool startEmpty) :
    HTTP::Request("", "REPORT"),
    _requestSent(0),
    _parser(repo->p),
    _repo(repo),
    _failed(false)
  {       
    setUrl(repo->vccUrl);

    _request =
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    "<S:update-report send-all=\"true\" xmlns:S=\"svn:\">\n"
    "<S:src-path>" + repo->baseUrl + "</S:src-path>\n"
    "<S:depth>unknown</S:depth>\n";

    _request += "<S:entry rev=\"" + aVersionName + "\" depth=\"infinity\" start-empty=\"true\"/>\n";
     
    if (!startEmpty) {
        string_list entries;
        _repo->rootCollection->mergeUpdateReportDetails(0, entries);
        BOOST_FOREACH(string e, entries) {
            _request += e + "\n";
        }
    }

    _request += "</S:update-report>";   
  }

  virtual string requestBodyType() const
  {
    return "application/xml; charset=\"utf-8\"";
  }

  virtual int requestBodyLength() const
  {
    return _request.size();
  }

  virtual int getBodyData(char* buf, int count) const
  {
    int len = std::min(count, requestBodyLength() - _requestSent);
    memcpy(buf, _request.c_str() + _requestSent, len);
    _requestSent += len;
    return len;
  }

protected:
  virtual void responseHeadersComplete()
  {

  }

  virtual void responseComplete()
  {
      if (_failed) {
          return;
      }
      
    if (responseCode() == 200) {
          SVNRepository::ResultCode err = _parser.finishParse();
          if (err) {
              _repo->updateFailed(this, err);
              _failed = true;
          } else {
              _repo->svnUpdateDone();
          }
    } else if (responseCode() == 404) {
        _repo->updateFailed(this, SVNRepository::SVN_ERROR_NOT_FOUND);
        _failed = true;
    } else {
        SG_LOG(SG_IO, SG_WARN, "SVN: request for:" << url() <<
        " return code " << responseCode());
        _repo->updateFailed(this, SVNRepository::SVN_ERROR_SOCKET);
        _failed = true;
    }
  }

  virtual void gotBodyData(const char* s, int n)
  {    
      if (_failed) {
          return;
      }
      
    if (responseCode() != 200) {
        return;
    }
    
    //cout << "body data:" << string(s, n) << endl;
    SVNRepository::ResultCode err = _parser.parseXML(s, n);
    if (err) {
        _failed = true;
        SG_LOG(SG_IO, SG_WARN, "SVN: request for:" << url() <<
            " XML parse failed");
        _repo->updateFailed(this, err);
    }
  }

    virtual void failed()
    {
        HTTP::Request::failed();
        _repo->updateFailed(this, SVNRepository::SVN_ERROR_SOCKET);
    }
private:
  string _request;
  mutable int _requestSent;
  SVNReportParser _parser;
  SVNRepoPrivate* _repo;
  bool _failed;
};
        
} // anonymous 

SVNRepository::SVNRepository(const SGPath& base, HTTP::Client *cl) :
  _d(new SVNRepoPrivate(this))
{
  _d->http = cl;
  _d->rootCollection = new SVNDirectory(this, base);
  _d->baseUrl = _d->rootCollection->url();  
}

SVNRepository::~SVNRepository()
{
    delete _d->rootCollection;
}

void SVNRepository::setBaseUrl(const std::string &url)
{
  _d->baseUrl = url;
  _d->rootCollection->setBaseUrl(url);
}

std::string SVNRepository::baseUrl() const
{
  return _d->baseUrl;
}

HTTP::Client* SVNRepository::http() const
{
  return _d->http;
}

SGPath SVNRepository::fsBase() const
{
  return _d->rootCollection->fsPath();
}

bool SVNRepository::isBare() const
{
    if (!fsBase().exists() || Dir(fsBase()).isEmpty()) {
        return true;
    }
    
    if (_d->vccUrl.empty()) {
        return true;
    }
    
    return false;
}

void SVNRepository::update()
{  
    _d->status = SVN_NO_ERROR;
    if (_d->targetRevision.empty() || _d->vccUrl.empty()) {        
        _d->isUpdating = true;        
        PropFindRequest* pfr = new PropFindRequest(_d.get());
        http()->makeRequest(pfr);
        return;
    }
        
    if (_d->targetRevision == rootDir()->cachedRevision()) {
        SG_LOG(SG_IO, SG_DEBUG, baseUrl() << " in sync at version " << _d->targetRevision);
        _d->isUpdating = false;
        return;
    }
    
    _d->isUpdating = true;
    UpdateReportRequest* urr = new UpdateReportRequest(_d.get(), 
        _d->targetRevision, isBare());
    http()->makeRequest(urr);
}
  
bool SVNRepository::isDoingSync() const
{
    if (_d->status != SVN_NO_ERROR) {
        return false;
    }
    
    return _d->isUpdating || _d->rootCollection->isDoingSync();
}

SVNDirectory* SVNRepository::rootDir() const
{
    return _d->rootCollection;
}

SVNRepository::ResultCode
SVNRepository::failure() const
{
    return _d->status;
}

///////////////////////////////////////////////////////////////////////////

void SVNRepoPrivate::propFindComplete(HTTP::Request* req, DAVCollection* c)
{
    targetRevision = c->versionName();
    vccUrl = makeAbsoluteUrl(c->versionControlledConfiguration(), baseUrl);
    rootCollection->collection()->setVersionControlledConfiguration(vccUrl);    
    p->update();
}
  
void SVNRepoPrivate::propFindFailed(HTTP::Request *req, SVNRepository::ResultCode err)
{
    if (err != SVNRepository::SVN_ERROR_NOT_FOUND) {
        SG_LOG(SG_IO, SG_WARN, "PropFind failed for:" << req->url());
    }
    
    isUpdating = false;
    status = err;
}

} // of namespace simgear
