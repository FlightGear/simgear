/**
 * \file HTTPClient.cxx - simple HTTP client engine for SimHear
 */

// Written by James Turner
//
// Copyright (C) 2013  James Turner  <zakalawe@mac.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear_config.h>

#include "HTTPClient.hxx"
#include "HTTPFileRequest.hxx"

#include <sstream>
#include <cassert>
#include <cstdlib> // rand()
#include <list>
#include <errno.h>
#include <map>
#include <stdexcept>
#include <mutex>

#include <simgear/simgear_config.h>


#include <simgear/io/sg_netChat.hxx>

#include <simgear/misc/strutils.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/structure/exception.hxx>

#include "HTTPClient_private.hxx"
#include "HTTPTestApi_private.hxx"

#if defined( HAVE_VERSION_H ) && HAVE_VERSION_H
#include "version.h"
#else
#  if !defined(SIMGEAR_VERSION)
#    define SIMGEAR_VERSION "simgear-development"
#  endif
#endif

namespace simgear
{

namespace HTTP
{

extern const int DEFAULT_HTTP_PORT = 80;
const char* CONTENT_TYPE_URL_ENCODED = "application/x-www-form-urlencoded";

void Client::ClientPrivate::createCurlMulti() {
  curlMulti = curl_multi_init();
  // see https://curl.haxx.se/libcurl/c/CURLMOPT_PIPELINING.html
  // we request HTTP 1.1 pipelining
  curl_multi_setopt(curlMulti, CURLMOPT_PIPELINING, 1 /* aka CURLPIPE_HTTP1 */);
#if (LIBCURL_VERSION_MINOR >= 30)
  curl_multi_setopt(curlMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS,
                    (long)maxConnections);
  curl_multi_setopt(curlMulti, CURLMOPT_MAX_PIPELINE_LENGTH,
                    (long)maxPipelineDepth);
  curl_multi_setopt(curlMulti, CURLMOPT_MAX_HOST_CONNECTIONS,
                    (long)maxHostConnections);
#endif
}

Client::Client()
{
    static bool didInitCurlGlobal = false;
    static std::mutex initMutex;

    std::lock_guard<std::mutex> g(initMutex);
    if (!didInitCurlGlobal) {
      curl_global_init(CURL_GLOBAL_ALL);
      didInitCurlGlobal = true;
    }

    reset();
}

Client::~Client()
{
  curl_multi_cleanup(d->curlMulti);
}

void Client::setMaxConnections(unsigned int maxCon)
{
    d->maxConnections = maxCon;
#if (LIBCURL_VERSION_MINOR >= 30)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long) maxCon);
#endif
}

void Client::setMaxHostConnections(unsigned int maxHostCon)
{
    d->maxHostConnections = maxHostCon;
#if (LIBCURL_VERSION_MINOR >= 30)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_HOST_CONNECTIONS, (long) maxHostCon);
#endif
}

void Client::setMaxPipelineDepth(unsigned int depth)
{
    d->maxPipelineDepth = depth;
#if (LIBCURL_VERSION_MINOR >= 30)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_PIPELINE_LENGTH, (long) depth);
#endif
}

void Client::reset()
{
    if (d.get()) {
        curl_multi_cleanup(d->curlMulti);
    }

    d.reset(new ClientPrivate);

    d->proxyPort = 0;
    d->maxConnections = 4;
    d->maxHostConnections = 4;
    d->bytesTransferred = 0;
    d->lastTransferRate = 0;
    d->timeTransferSample.stamp();
    d->totalBytesDownloaded = 0;
    d->maxPipelineDepth = 5;
    d->curlPerformActive = false;
    setUserAgent("SimGear-" SG_STRINGIZE(SIMGEAR_VERSION));
    d->tlsCertificatePath = SGPath::fromEnv("SIMGEAR_TLS_CERT_PATH");
    d->createCurlMulti();
}

void Client::update(int waitTimeout)
{
    if (d->requests.empty()) {
        // curl_multi_wait returns immediately if there's no requests active,
        // but that can cause high CPU usage for us.
        SGTimeStamp::sleepForMSec(waitTimeout);
        return;
    }

    int remainingActive, messagesInQueue;
    int numFds;
    CURLMcode mc = curl_multi_wait(d->curlMulti, NULL, 0, waitTimeout, &numFds);
    if (mc != CURLM_OK) {
        SG_LOG(SG_IO, SG_WARN, "curl_multi_wait failed:" << curl_multi_strerror(mc));
        return;
    }

    d->curlPerformActive = true;
    mc = curl_multi_perform(d->curlMulti, &remainingActive);
    if (mc == CURLM_CALL_MULTI_PERFORM) {
        // we could loop here, but don't want to get blocked
        // also this shouldn't  ocurr in any modern libCurl
        curl_multi_perform(d->curlMulti, &remainingActive);
    } else if (mc != CURLM_OK) {
        d->curlPerformActive = false;
        SG_LOG(SG_IO, SG_WARN, "curl_multi_perform failed:" << curl_multi_strerror(mc));
        return;
    }

    d->curlPerformActive = false;
    for (auto r : d->pendingCancelRequests) {
        cancelRequest(r, "deferred cancellation");
    };

    CURLMsg* msg;
    while ((msg = curl_multi_info_read(d->curlMulti, &messagesInQueue))) {
      if (msg->msg == CURLMSG_DONE) {
        Request* rawReq = 0;
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(e, CURLINFO_PRIVATE, &rawReq);

        // ensure request stays valid for the moment
        // eg if responseComplete cancels us
        Request_ptr req(rawReq);

        long responseCode;
        curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &responseCode);

          // remove from the requests map now,
          // in case the callbacks perform a cancel. We'll use
          // the absence from the request dict in cancel to avoid
          // a double remove
          ClientPrivate::RequestCurlMap::iterator it = d->requests.find(req);
          assert(it != d->requests.end());
          assert(it->second == e);
          d->requests.erase(it);

          bool doProcess = true;
          if (d->testsuiteResponseDoneCallback) {
            doProcess =
                !d->testsuiteResponseDoneCallback(msg->data.result, req);
          }

          if (doProcess) {
            bool ok = false;
            SG_LOG(SG_IO, SG_DEBUG, "msg->data.result=" << msg->data.result);
            if (msg->data.result == CURLE_OK) {
              ok = true;
            }
            else if (req->range() != "" && (responseCode == 206 || responseCode == 416)) {
              SG_LOG(SG_IO, SG_DEBUG, "req->range() is set (to '" << req->range() << "')"
                      << " so treating as ok: msg->data.result=" << msg->data.result);
              ok = true;
            }
            if (ok) {
              req->responseComplete();
            } else {
              SG_LOG(SG_IO, SG_WARN,
                     "CURL Result:" << msg->data.result << " "
                                    << curl_easy_strerror(msg->data.result));
              req->setFailure(msg->data.result,
                              curl_easy_strerror(msg->data.result));
            }
          }

        curl_multi_remove_handle(d->curlMulti, e);
        curl_easy_cleanup(e);
      } else {
          // should never happen since CURLMSG_DONE is the only code
          // defined!
          SG_LOG(SG_IO, SG_ALERT, "unknown CurlMSG:" << msg->msg);
      }
    } // of curl message processing loop
}

void Client::makeRequest(const Request_ptr& r)
{
    if( r->isComplete() )
      return;

    if (r->url().empty()) {
        r->setFailure(EINVAL, "no URL specified on request");
        return;
    }

    if( r->url().find("://") == std::string::npos ) {
        r->setFailure(EINVAL, "malformed URL: '" + r->url() + "'");
        return;
    }

    r->_client = this;

    assert(d->requests.find(r) == d->requests.end());

    CURL* curlRequest = curl_easy_init();
    curl_easy_setopt(curlRequest, CURLOPT_URL, r->url().c_str());

    d->requests[r] = curlRequest;

    curl_easy_setopt(curlRequest, CURLOPT_PRIVATE, r.get());
    // disable built-in libCurl progress feedback
    curl_easy_setopt(curlRequest, CURLOPT_NOPROGRESS, 1);

    curl_easy_setopt(curlRequest, CURLOPT_WRITEFUNCTION, requestWriteCallback);
    curl_easy_setopt(curlRequest, CURLOPT_WRITEDATA, r.get());
    curl_easy_setopt(curlRequest, CURLOPT_HEADERFUNCTION, requestHeaderCallback);
    curl_easy_setopt(curlRequest, CURLOPT_HEADERDATA, r.get());
    unsigned long bps = r->getMaxBytesPerSec();
    if (bps != 0) {
        curl_easy_setopt(curlRequest, CURLOPT_MAX_RECV_SPEED_LARGE, bps);
    }

#if !defined(CURL_MAX_READ_SIZE)
	const int CURL_MAX_READ_SIZE = 512 * 1024;
#endif

    curl_easy_setopt(curlRequest, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);
    curl_easy_setopt(curlRequest, CURLOPT_USERAGENT, d->userAgent.c_str());
    curl_easy_setopt(curlRequest, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    if (sglog().would_log(SG_TERRASYNC, SG_DEBUG, __FILE__, __LINE__, __FUNCTION__)) {
        curl_easy_setopt(curlRequest, CURLOPT_VERBOSE, 1);
    }

    curl_easy_setopt(curlRequest, CURLOPT_FOLLOWLOCATION, 1);

    if (!d->tlsCertificatePath.isNull()) {
        const auto utf8 = d->tlsCertificatePath.utf8Str();
        curl_easy_setopt(curlRequest, CURLOPT_CAINFO, utf8.c_str());
    }

    if (!d->proxy.empty()) {
      curl_easy_setopt(curlRequest, CURLOPT_PROXY, d->proxy.c_str());
      curl_easy_setopt(curlRequest, CURLOPT_PROXYPORT, d->proxyPort);

      if (!d->proxyAuth.empty()) {
        curl_easy_setopt(curlRequest, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curlRequest, CURLOPT_PROXYUSERPWD, d->proxyAuth.c_str());
      }
    }

    std::string range = r->range();
    if (range != "") {
        curl_easy_setopt(curlRequest, CURLOPT_RANGE, range.c_str());
        SG_LOG(SG_GENERAL, SG_DEBUG, "Have added CURLOPT_RANGE: '" << range << "'");
    }
    
    const char* enc = r->getAcceptEncoding();
    if (enc) {
        curl_easy_setopt(curlRequest, CURLOPT_ACCEPT_ENCODING, const_cast<char*>(enc));
        SG_LOG(SG_GENERAL, SG_ALERT, "Have added CURLOPT_ACCEPT_ENCODING: '" << enc << "'");
    }

    const std::string method = strutils::lowercase (r->method());
    if (method == "get") {
      curl_easy_setopt(curlRequest, CURLOPT_HTTPGET, 1);
    } else if (method == "put") {
      curl_easy_setopt(curlRequest, CURLOPT_PUT, 1);
      curl_easy_setopt(curlRequest, CURLOPT_UPLOAD, 1);
    } else if (method == "post") {
      // see http://curl.haxx.se/libcurl/c/CURLOPT_POST.html
      curl_easy_setopt(curlRequest, CURLOPT_HTTPPOST, 1);

      std::string q = r->query().substr(1);
      curl_easy_setopt(curlRequest, CURLOPT_COPYPOSTFIELDS, q.c_str());

      // reset URL to exclude query pieces
      std::string urlWithoutQuery = r->url();
      std::string::size_type queryPos = urlWithoutQuery.find('?');
      urlWithoutQuery.resize(queryPos);
      curl_easy_setopt(curlRequest, CURLOPT_URL, urlWithoutQuery.c_str());
    } else {
      curl_easy_setopt(curlRequest, CURLOPT_CUSTOMREQUEST, r->method().c_str());
    }

    struct curl_slist* headerList = NULL;
    if (r->hasBodyData() && (method != "post")) {
      curl_easy_setopt(curlRequest, CURLOPT_UPLOAD, 1);
      curl_easy_setopt(curlRequest, CURLOPT_INFILESIZE, r->bodyLength());
      curl_easy_setopt(curlRequest, CURLOPT_READFUNCTION, requestReadCallback);
      curl_easy_setopt(curlRequest, CURLOPT_READDATA, r.get());
      std::string h = "Content-Type:" + r->bodyType();
      headerList = curl_slist_append(headerList, h.c_str());
    }

    StringMap::const_iterator it;
    for (it = r->requestHeaders().begin(); it != r->requestHeaders().end(); ++it) {
      std::string h = it->first + ": " + it->second;
      headerList = curl_slist_append(headerList, h.c_str());
    }

    if (headerList != NULL) {
      curl_easy_setopt(curlRequest, CURLOPT_HTTPHEADER, headerList);
    }

    curl_multi_add_handle(d->curlMulti, curlRequest);

// this seems premature, but we don't have a callback from Curl we could
// use to trigger when the requst is actually sent.
    r->requestStart();
}

void Client::cancelRequest(const Request_ptr &r, std::string reason)
{
    ClientPrivate::RequestCurlMap::iterator it = d->requests.find(r);
    if(it == d->requests.end()) {
        // already being removed, presumably inside ::update()
        // nothing more to do
        return;
    }

    // don't cancel from inside curl_multi_perform, causes
    // everything to melt. Defer until after it's done
    if (d->curlPerformActive) {
        d->pendingCancelRequests.push_back(r);
        return;
    }

    CURLMcode err = curl_multi_remove_handle(d->curlMulti, it->second);
    if (err != CURLM_OK) {
        SG_LOG(SG_IO, SG_WARN, "cancelRequest: curl_multi_remove_handle failed:" << err);
    }

    // clear the request pointer form the curl-easy object
    curl_easy_setopt(it->second, CURLOPT_PRIVATE, 0);

    curl_easy_cleanup(it->second);
    d->requests.erase(it);

    r->setFailure(-1, reason);
}

//------------------------------------------------------------------------------
FileRequestRef Client::save( const std::string& url,
                             const std::string& filename )
{
  FileRequestRef req = new FileRequest(url, filename);
  makeRequest(req);
  return req;
}

//------------------------------------------------------------------------------
MemoryRequestRef Client::load(const std::string& url)
{
  MemoryRequestRef req = new MemoryRequest(url);
  makeRequest(req);
  return req;
}

void Client::requestFinished(Connection* con)
{

}

void Client::setUserAgent(const std::string& ua)
{
    d->userAgent = ua;
}

const std::string& Client::userAgent() const
{
    return d->userAgent;
}

const std::string& Client::proxyHost() const
{
    return d->proxy;
}

const std::string& Client::proxyAuth() const
{
    return d->proxyAuth;
}

void Client::setProxy( const std::string& proxy,
                       int port,
                       const std::string& auth )
{
    d->proxy = proxy;
    d->proxyPort = port;
    d->proxyAuth = auth;
}

bool Client::hasActiveRequests() const
{
    return !d->requests.empty();
}

void Client::receivedBytes(unsigned int count)
{
    d->bytesTransferred += count;
    d->totalBytesDownloaded += count;
}

unsigned int Client::transferRateBytesPerSec() const
{
    unsigned int e = d->timeTransferSample.elapsedMSec();
    if (e > 400) {
        // too long a window, ignore
        d->timeTransferSample.stamp();
        d->bytesTransferred = 0;
        d->lastTransferRate = 0;
        return 0;
    }

    if (e < 100) { // avoid really narrow windows
        return d->lastTransferRate;
    }

    unsigned int ratio = (d->bytesTransferred * 1000) / e;
    // run a low-pass filter
    unsigned int smoothed = ((400 - e) * d->lastTransferRate) + (e * ratio);
    smoothed /= 400;

    d->timeTransferSample.stamp();
    d->bytesTransferred = 0;
    d->lastTransferRate = smoothed;
    return smoothed;
}

uint64_t Client::totalBytesDownloaded() const
{
    return d->totalBytesDownloaded;
}

size_t Client::requestWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  size_t byteSize = size * nmemb;
  Request* req = static_cast<Request*>(userdata);
  req->processBodyBytes(ptr, byteSize);

  Client* cl = req->http();
  if (cl) {
    cl->receivedBytes(byteSize);
  }

  return byteSize;
}

size_t Client::requestReadCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  size_t maxBytes = size * nmemb;
  Request* req = static_cast<Request*>(userdata);
  size_t actualBytes = req->getBodyData(ptr, 0, maxBytes);
  return actualBytes;
}

bool isRedirectStatus(int code)
{
    return ((code >= 300) && (code < 400));
}

size_t Client::requestHeaderCallback(char *rawBuffer, size_t size, size_t nitems, void *userdata)
{
  size_t byteSize = size * nitems;
  Request* req = static_cast<Request*>(userdata);
  std::string h = strutils::simplify(std::string(rawBuffer, byteSize));

  if (req->readyState() >= HTTP::Request::HEADERS_RECEIVED) {
      // this can happen with chunked transfers (secondary chunks)
      // or redirects
      if (isRedirectStatus(req->responseCode())) {
          req->responseStart(h);
          return byteSize;
      }
  }

  if (req->readyState() == HTTP::Request::OPENED) {
    req->responseStart(h);
    return byteSize;
  }

  if (h.empty()) {
      // got a 100-continue reponse; restart
      if (req->responseCode() == 100) {
          req->setReadyState(HTTP::Request::OPENED);
          return byteSize;
      }

    req->responseHeadersComplete();
    return byteSize;
  }

  if (req->responseCode() == 100) {
      return byteSize; // skip headers associated with 100-continue status
  }

  size_t colonPos = h.find(':');
  if (colonPos == std::string::npos) {
      SG_LOG(SG_IO, SG_WARN, "malformed HTTP response header:" << h);
      return byteSize;
  }

  const std::string key = strutils::simplify(h.substr(0, colonPos));
  const std::string lkey = strutils::lowercase (key);
  std::string value = strutils::strip(h.substr(colonPos + 1));

  req->responseHeader(lkey, value);
  return byteSize;
}

void Client::debugDumpRequests()
{
    SG_LOG(SG_IO, SG_INFO, "== HTTP request dump");
    ClientPrivate::RequestCurlMap::iterator it = d->requests.begin();
    for (; it != d->requests.end(); ++it) {
        SG_LOG(SG_IO, SG_INFO, "\t" << it->first->url());
    }
    SG_LOG(SG_IO, SG_INFO, "==");
}

void Client::clearAllConnections()
{
    curl_multi_cleanup(d->curlMulti);
    d->createCurlMulti();
}

/////////////////////////////////////////////////////////////////////

void TestApi::setResponseDoneCallback(Client *cl, ResponseDoneCallback cb) {
  cl->d->testsuiteResponseDoneCallback = cb;
}

void TestApi::markRequestAsFailed(Request_ptr req, int curlCode,
                                  const std::string &message) {
  req->setFailure(curlCode, message);
}

} // of namespace HTTP

} // of namespace simgear
