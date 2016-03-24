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


#include "HTTPClient.hxx"
#include "HTTPFileRequest.hxx"

#include <sstream>
#include <cassert>
#include <cstdlib> // rand()
#include <list>
#include <errno.h>
#include <map>
#include <stdexcept>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/simgear_config.h>

#if defined(ENABLE_CURL)
  #include <curl/multi.h>
#else
    #include <simgear/io/HTTPContentDecode.hxx>
#endif

#include <simgear/io/sg_netChat.hxx>

#include <simgear/misc/strutils.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/structure/exception.hxx>

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

class Connection;
typedef std::multimap<std::string, Connection*> ConnectionDict;
typedef std::list<Request_ptr> RequestList;

class Client::ClientPrivate
{
public:
#if defined(ENABLE_CURL)
    CURLM* curlMulti;
    bool haveActiveRequests;

    void createCurlMulti()
    {
        curlMulti = curl_multi_init();
        // see https://curl.haxx.se/libcurl/c/CURLMOPT_PIPELINING.html
        // we request HTTP 1.1 pipelining
        curl_multi_setopt(curlMulti, CURLMOPT_PIPELINING, 1 /* aka CURLPIPE_HTTP1 */);
        curl_multi_setopt(curlMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long) maxConnections);
        curl_multi_setopt(curlMulti, CURLMOPT_MAX_PIPELINE_LENGTH,
                          (long) maxPipelineDepth);
        curl_multi_setopt(curlMulti, CURLMOPT_MAX_HOST_CONNECTIONS,
                          (long) maxHostConnections);


    }
#else
    NetChannelPoller poller;
// connections by host (potentially more than one)
    ConnectionDict connections;
#endif

    std::string userAgent;
    std::string proxy;
    int proxyPort;
    std::string proxyAuth;
    unsigned int maxConnections;
    unsigned int maxHostConnections;
    unsigned int maxPipelineDepth;

    RequestList pendingRequests;

    SGTimeStamp timeTransferSample;
    unsigned int bytesTransferred;
    unsigned int lastTransferRate;
    uint64_t totalBytesDownloaded;
};

#if !defined(ENABLE_CURL)
class Connection : public NetChat
{
public:
    Connection(Client* pr, const std::string& conId) :
        client(pr),
        state(STATE_CLOSED),
        port(DEFAULT_HTTP_PORT),
        _connectionId(conId),
        _maxPipelineLength(255)
    {
    }

    virtual ~Connection()
    {
    }

    virtual void handleBufferRead (NetBuffer& buffer)
    {
      if( !activeRequest || !activeRequest->isComplete() )
        return NetChat::handleBufferRead(buffer);

      // Request should be aborted (signaled by setting its state to complete).

      // force the state to GETTING_BODY, to simplify logic in
      // responseComplete and handleClose
        setState(STATE_GETTING_BODY);
      responseComplete();
    }

    void setServer(const std::string& h, short p)
    {
        host = h;
        port = p;
    }

    void setMaxPipelineLength(unsigned int m)
    {
        _maxPipelineLength = m;
    }

    // socket-level errors
    virtual void handleError(int error)
    {
        const char* errStr = strerror(error);
        SG_LOG(SG_IO, SG_WARN, _connectionId << " handleError:" << error << " ("
               << errStr << ")");

        debugDumpRequests();

        if (!activeRequest)
        {
        // connection level failure, eg name lookup or routing
        // we won't have an active request yet, so let's fail all of the
        // requests since we presume it's a systematic failure for
        // the host in question
            BOOST_FOREACH(Request_ptr req, sentRequests) {
                req->setFailure(error, errStr);
            }

            BOOST_FOREACH(Request_ptr req, queuedRequests) {
                req->setFailure(error, errStr);
            }

            sentRequests.clear();
            queuedRequests.clear();
        }

        NetChat::handleError(error);
        if (activeRequest) {
            activeRequest->setFailure(error, errStr);
            activeRequest = NULL;
            _contentDecoder.reset();
        }

        setState(STATE_SOCKET_ERROR);
    }

    void handleTimeout()
    {
        handleError(ETIMEDOUT);
    }

    virtual void handleClose()
    {
        NetChat::handleClose();

    // closing of the connection from the server side when getting the body,
        bool canCloseState = (state == STATE_GETTING_BODY);
        if (canCloseState && activeRequest) {
            // check bodyTransferSize matches how much we actually transferred
            if (bodyTransferSize > 0) {
                if (_contentDecoder.getTotalReceivedBytes() != bodyTransferSize) {
                    SG_LOG(SG_IO, SG_WARN, _connectionId << " saw connection close while still receiving bytes for:" << activeRequest->url()
                           << "\n\thave:" << _contentDecoder.getTotalReceivedBytes() << " of " << bodyTransferSize);
                }
            }

        // force state here, so responseComplete can avoid closing the
        // socket again
            SG_LOG(SG_IO, SG_DEBUG, _connectionId << " saw connection close after getting:" << activeRequest->url());
            setState(STATE_CLOSED);
            responseComplete();
        } else {
            if (state == STATE_WAITING_FOR_RESPONSE) {
                SG_LOG(SG_IO, SG_DEBUG, _connectionId << ":close while waiting for response, front request is:"
                       << sentRequests.front()->url());
                assert(!sentRequests.empty());
                sentRequests.front()->setFailure(500, "server closed connection unexpectedly");
                // no active request, but don't restore the front sent one
                sentRequests.erase(sentRequests.begin());
            }

            if (activeRequest) {
                activeRequest->setFailure(500, "server closed connection");
                // remove the failed request from sentRequests, so it does
                // not get restored
                RequestList::iterator it = std::find(sentRequests.begin(),
                    sentRequests.end(), activeRequest);
                if (it != sentRequests.end()) {
                    sentRequests.erase(it);
                }
                activeRequest = NULL;
                _contentDecoder.reset();
            }

            setState(STATE_CLOSED);
        }

      if (sentRequests.empty()) {
        return;
      }

    // restore sent requests to the queue, so they will be re-sent
    // when the connection opens again
      queuedRequests.insert(queuedRequests.begin(),
                              sentRequests.begin(), sentRequests.end());
      sentRequests.clear();
    }

    void queueRequest(const Request_ptr& r)
    {
        queuedRequests.push_back(r);
        tryStartNextRequest();
    }

    void beginResponse()
    {
        assert(!sentRequests.empty());
        assert(state == STATE_WAITING_FOR_RESPONSE);

        activeRequest = sentRequests.front();
        try {
            SG_LOG(SG_IO, SG_DEBUG, "con:" << _connectionId << " saw start of response for " << activeRequest->url());
            activeRequest->responseStart(buffer);
        } catch (sg_exception& e) {
            handleError(EIO);
            return;
        }

      setState(STATE_GETTING_HEADERS);
      buffer.clear();
      if (activeRequest->responseCode() == 204) {
        noMessageBody = true;
      } else if (activeRequest->method() == "HEAD") {
        noMessageBody = true;
      } else {
        noMessageBody = false;
      }

      bodyTransferSize = -1;
      chunkedTransfer = false;
      _contentDecoder.reset();
    }

    void tryStartNextRequest()
    {
      while( !queuedRequests.empty()
          && queuedRequests.front()->isComplete() )
        queuedRequests.pop_front();

      if (queuedRequests.empty()) {
        idleTime.stamp();
        return;
      }

      if (sentRequests.size() >= _maxPipelineLength) {
        return;
      }

      if (state == STATE_CLOSED) {
          if (!connectToHost()) {
              setState(STATE_SOCKET_ERROR);
              return;
          }

          SG_LOG(SG_IO, SG_DEBUG, "connection " << _connectionId << " connected.");
          setTerminator("\r\n");
          setState(STATE_IDLE);
      }

      Request_ptr r = queuedRequests.front();
      r->requestStart();

      std::stringstream headerData;
      std::string path = r->path();
      assert(!path.empty());
      std::string query = r->query();
      std::string bodyData;

      if (!client->proxyHost().empty()) {
          path = r->scheme() + "://" + r->host() + r->path();
      }

      if (r->bodyType() == CONTENT_TYPE_URL_ENCODED) {
          headerData << r->method() << " " << path << " HTTP/1.1\r\n";
          bodyData = query.substr(1); // URL-encode, drop the leading '?'
          headerData << "Content-Type:" << CONTENT_TYPE_URL_ENCODED << "\r\n";
          headerData << "Content-Length:" << bodyData.size() << "\r\n";
      } else {
          headerData << r->method() << " " << path << query << " HTTP/1.1\r\n";
          if( r->hasBodyData() )
          {
            headerData << "Content-Length:" << r->bodyLength() << "\r\n";
            headerData << "Content-Type:" << r->bodyType() << "\r\n";
          }
      }

      headerData << "Host: " << r->hostAndPort() << "\r\n";
      headerData << "User-Agent:" << client->userAgent() << "\r\n";
      headerData << "Accept-Encoding: deflate, gzip\r\n";
      if (!client->proxyAuth().empty()) {
          headerData << "Proxy-Authorization: " << client->proxyAuth() << "\r\n";
      }

      BOOST_FOREACH(const StringMap::value_type& h, r->requestHeaders()) {
          headerData << h.first << ": " << h.second << "\r\n";
      }

      headerData << "\r\n"; // final CRLF to terminate the headers
      if (!bodyData.empty()) {
          headerData << bodyData;
      }

      bool ok = push(headerData.str().c_str());
      if (!ok) {
          SG_LOG(SG_IO, SG_WARN, "HTTPClient: over-stuffed the socket");
          // we've over-stuffed the socket, give up for now, let things
          // drain down before trying to start any more requests.
          return;
      }

      if( r->hasBodyData() )
        for(size_t body_bytes_sent = 0; body_bytes_sent < r->bodyLength();)
        {
          char buf[4096];
          size_t len = r->getBodyData(buf, body_bytes_sent, 4096);
          if( len )
          {
            if( !bufferSend(buf, len) )
            {
              SG_LOG(SG_IO,
                     SG_WARN,
                     "overflow the HTTP::Connection output buffer");
              state = STATE_SOCKET_ERROR;
              return;
            }
            body_bytes_sent += len;
          }
          else
          {
            SG_LOG(SG_IO,
                   SG_WARN,
                   "HTTP asynchronous request body generation is unsupported");
            break;
          }
        }

        SG_LOG(SG_IO, SG_DEBUG, "con:" << _connectionId << " did send request:" << r->url());
      // successfully sent, remove from queue, and maybe send the next
      queuedRequests.pop_front();
      sentRequests.push_back(r);
      if (state == STATE_IDLE) {
          setState(STATE_WAITING_FOR_RESPONSE);
      }

      // pipelining, let's maybe send the next request right away
      tryStartNextRequest();
    }

    virtual void collectIncomingData(const char* s, int n)
    {
        idleTime.stamp();
        client->receivedBytes(static_cast<unsigned int>(n));

        if(   (state == STATE_GETTING_BODY)
           || (state == STATE_GETTING_CHUNKED_BYTES) )
          _contentDecoder.receivedBytes(s, n);
        else
          buffer.append(s, n);
    }

    virtual void foundTerminator(void)
    {
        idleTime.stamp();
        switch (state) {
        case STATE_WAITING_FOR_RESPONSE:
            beginResponse();
            break;

        case STATE_GETTING_HEADERS:
            processHeader();
            buffer.clear();
            break;

        case STATE_GETTING_BODY:
            responseComplete();
            break;

        case STATE_GETTING_CHUNKED:
            processChunkHeader();
            break;

        case STATE_GETTING_CHUNKED_BYTES:
            setTerminator("\r\n");
            setState(STATE_GETTING_CHUNKED);
            buffer.clear();
            break;


        case STATE_GETTING_TRAILER:
            processTrailer();
            buffer.clear();
            break;

        case STATE_IDLE:
            SG_LOG(SG_IO, SG_WARN, "HTTP got data in IDLE state, bad server?");

        default:
            break;
        }
    }

    bool hasIdleTimeout() const
    {
        if ((state != STATE_IDLE) && (state != STATE_CLOSED)) {
            return false;
        }

        assert(sentRequests.empty());
        bool isTimedOut = (idleTime.elapsedMSec() > (1000 * 10)); // 10 seconds
        return isTimedOut;
    }

    bool hasErrorTimeout() const
    {
      if ((state == STATE_IDLE) || (state == STATE_CLOSED)) {
        return false;
      }

        bool isTimedOut = (idleTime.elapsedMSec() > (1000 * 30)); // 30 seconds
        return isTimedOut;
    }

    bool hasError() const
    {
        return (state == STATE_SOCKET_ERROR);
    }

    bool shouldStartNext() const
    {
      return !queuedRequests.empty() && (sentRequests.size() < _maxPipelineLength);
    }

    bool isActive() const
    {
        return !queuedRequests.empty() || !sentRequests.empty();
    }

    std::string connectionId() const
    {
        return _connectionId;
    }

    void debugDumpRequests() const
    {
        SG_LOG(SG_IO, SG_DEBUG, "requests for:" << host << ":" << port << " (conId=" << _connectionId
               << "; state=" << state << ")");
        if (activeRequest) {
            SG_LOG(SG_IO, SG_DEBUG, "\tactive:" << activeRequest->url());
        } else {
            SG_LOG(SG_IO, SG_DEBUG, "\tNo active request");
        }

        BOOST_FOREACH(Request_ptr req, sentRequests) {
            SG_LOG(SG_IO, SG_DEBUG, "\tsent:" << req->url());
        }

        BOOST_FOREACH(Request_ptr req, queuedRequests) {
            SG_LOG(SG_IO, SG_DEBUG, "\tqueued:" << req->url());
        }
    }
private:
    enum ConnectionState {
        STATE_IDLE = 0,
        STATE_WAITING_FOR_RESPONSE,
        STATE_GETTING_HEADERS,
        STATE_GETTING_BODY,
        STATE_GETTING_CHUNKED,
        STATE_GETTING_CHUNKED_BYTES,
        STATE_GETTING_TRAILER,
        STATE_SOCKET_ERROR,
        STATE_CLOSED             ///< connection should be closed now
    };

    void setState(ConnectionState newState)
    {
        if (state == newState) {
            return;
        }

        state = newState;
    }

    bool connectToHost()
    {
        SG_LOG(SG_IO, SG_DEBUG, "HTTP connecting to " << host << ":" << port);

        if (!open()) {
            SG_LOG(SG_IO, SG_WARN, "HTTP::Connection: connectToHost: open() failed");
            return false;
        }

        if (connect(host.c_str(), port) != 0) {
            SG_LOG(SG_IO, SG_WARN, "HTTP::Connection: connectToHost: connect() failed");
            return false;
        }

        return true;
    }


    void processHeader()
    {
        std::string h = strutils::simplify(buffer);
        if (h.empty()) { // blank line terminates headers
            headersComplete();
            return;
        }

        int colonPos = buffer.find(':');
        if (colonPos < 0) {
            SG_LOG(SG_IO, SG_WARN, "malformed HTTP response header:" << h);
            return;
        }

        std::string key = strutils::simplify(buffer.substr(0, colonPos));
        std::string lkey = boost::to_lower_copy(key);
        std::string value = strutils::strip(buffer.substr(colonPos + 1));

        // only consider these if getting headers (as opposed to trailers
        // of a chunked transfer)
        if (state == STATE_GETTING_HEADERS) {
            if (lkey == "content-length") {

                int sz = strutils::to_int(value);
                if (bodyTransferSize <= 0) {
                    bodyTransferSize = sz;
                }
                activeRequest->setResponseLength(sz);
            } else if (lkey == "transfer-length") {
                bodyTransferSize = strutils::to_int(value);
            } else if (lkey == "transfer-encoding") {
                processTransferEncoding(value);
            } else if (lkey == "content-encoding") {
                _contentDecoder.setEncoding(value);
            }
        }

        activeRequest->responseHeader(lkey, value);
    }

    void processTransferEncoding(const std::string& te)
    {
        if (te == "chunked") {
            chunkedTransfer = true;
        } else {
            SG_LOG(SG_IO, SG_WARN, "unsupported transfer encoding:" << te);
            // failure
        }
    }

    void processChunkHeader()
    {
        if (buffer.empty()) {
            // blank line after chunk data
            return;
        }

        int chunkSize = 0;
        int semiPos = buffer.find(';');
        if (semiPos >= 0) {
            // extensions ignored for the moment
            chunkSize = strutils::to_int(buffer.substr(0, semiPos), 16);
        } else {
            chunkSize = strutils::to_int(buffer, 16);
        }

        buffer.clear();
        if (chunkSize == 0) {  //  trailer start
            setState(STATE_GETTING_TRAILER);
            return;
        }

        setState(STATE_GETTING_CHUNKED_BYTES);
        setByteCount(chunkSize);
    }

    void processTrailer()
    {
        if (buffer.empty()) {
            // end of trailers
            responseComplete();
            return;
        }

    // process as a normal header
        processHeader();
    }

    void headersComplete()
    {
        activeRequest->responseHeadersComplete();
        _contentDecoder.initWithRequest(activeRequest);

        if (!activeRequest->serverSupportsPipelining()) {
            SG_LOG(SG_IO, SG_DEBUG, _connectionId << " disabling pipelining since server does not support it");
            _maxPipelineLength = 1;
        }

        if (chunkedTransfer) {
            setState(STATE_GETTING_CHUNKED);
        } else if (noMessageBody || (bodyTransferSize == 0)) {
            // force the state to GETTING_BODY, to simplify logic in
            // responseComplete and handleClose
            setState(STATE_GETTING_BODY);
            responseComplete();
        } else {
            setByteCount(bodyTransferSize); // may be -1, that's fine
            setState(STATE_GETTING_BODY);
        }
    }

    void responseComplete()
    {
        Request_ptr completedRequest = activeRequest;
        _contentDecoder.finish();

        assert(sentRequests.front() == activeRequest);
        sentRequests.pop_front();
        bool doClose = activeRequest->closeAfterComplete();
        activeRequest = NULL;

        if ((state == STATE_GETTING_BODY) || (state == STATE_GETTING_TRAILER)) {
            if (doClose) {
                SG_LOG(SG_IO, SG_DEBUG, _connectionId << " doClose requested");
          // this will bring us into handleClose() above, which updates
          // state to STATE_CLOSED
              close();

          // if we have additional requests waiting, try to start them now
              tryStartNextRequest();
            }
        }

        if (state != STATE_CLOSED)  {
            setState(sentRequests.empty() ? STATE_IDLE : STATE_WAITING_FOR_RESPONSE);
        }

    // notify request after we change state, so this connection is idle
    // if completion triggers other requests (which is likely)
        completedRequest->responseComplete();
        client->requestFinished(this);

        setTerminator("\r\n");
    }

    Client* client;
    Request_ptr activeRequest;
    ConnectionState state;
    std::string host;
    short port;
    std::string buffer;
    int bodyTransferSize;
    SGTimeStamp idleTime;
    bool chunkedTransfer;
    bool noMessageBody;

    RequestList queuedRequests;
    RequestList sentRequests;

    ContentDecoder _contentDecoder;
    std::string _connectionId;
    unsigned int _maxPipelineLength;
};
#endif // of !ENABLE_CURL

Client::Client() :
    d(new ClientPrivate)
{
    d->proxyPort = 0;
    d->maxConnections = 4;
    d->maxHostConnections = 4;
    d->bytesTransferred = 0;
    d->lastTransferRate = 0;
    d->timeTransferSample.stamp();
    d->totalBytesDownloaded = 0;
    d->maxPipelineDepth = 5;
    setUserAgent("SimGear-" SG_STRINGIZE(SIMGEAR_VERSION));
#if defined(ENABLE_CURL)
    static bool didInitCurlGlobal = false;
    if (!didInitCurlGlobal) {
      curl_global_init(CURL_GLOBAL_ALL);
      didInitCurlGlobal = true;
    }

    d->createCurlMulti();
#endif
}

Client::~Client()
{
#if defined(ENABLE_CURL)
  curl_multi_cleanup(d->curlMulti);
#endif
}

void Client::setMaxConnections(unsigned int maxCon)
{
    d->maxConnections = maxCon;
#if defined(ENABLE_CURL)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long) maxCon);
#endif
}

void Client::setMaxHostConnections(unsigned int maxHostCon)
{
    d->maxHostConnections = maxHostCon;
#if defined(ENABLE_CURL)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_HOST_CONNECTIONS, (long) maxHostCon);
#endif
}

void Client::setMaxPipelineDepth(unsigned int depth)
{
    d->maxPipelineDepth = depth;
#if defined(ENABLE_CURL)
    curl_multi_setopt(d->curlMulti, CURLMOPT_MAX_PIPELINE_LENGTH, (long) depth);
#else
    ConnectionDict::iterator it = d->connections.begin();
    for (; it != d->connections.end(); ) {
        it->second->setMaxPipelineLength(depth);
    }
#endif
}

void Client::update(int waitTimeout)
{
#if defined(ENABLE_CURL)
    int remainingActive, messagesInQueue;
    curl_multi_perform(d->curlMulti, &remainingActive);
    d->haveActiveRequests = (remainingActive > 0);

    CURLMsg* msg;
    while ((msg = curl_multi_info_read(d->curlMulti, &messagesInQueue))) {
      if (msg->msg == CURLMSG_DONE) {
        Request* req;
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(e, CURLINFO_PRIVATE, &req);

        long responseCode;
        curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &responseCode);

        if (msg->data.result == 0) {
          req->responseComplete();
        } else {
          fprintf(stderr, "Result: %d - %s\n",
                msg->data.result, curl_easy_strerror(msg->data.result));
          req->setFailure(msg->data.result, curl_easy_strerror(msg->data.result));
        }

        curl_multi_remove_handle(d->curlMulti, e);

        // balance the reference we take in makeRequest
        SGReferenced::put(req);
        curl_easy_cleanup(e);
      }
      else {
        SG_LOG(SG_IO, SG_ALERT, "CurlMSG:" << msg->msg);
      }
    } // of curl message processing loop
    SGTimeStamp::sleepForMSec(waitTimeout);
#else
    if (!d->poller.hasChannels() && (waitTimeout > 0)) {
        SGTimeStamp::sleepForMSec(waitTimeout);
    } else {
        d->poller.poll(waitTimeout);
    }

    bool waitingRequests = !d->pendingRequests.empty();
    ConnectionDict::iterator it = d->connections.begin();
    for (; it != d->connections.end(); ) {
        Connection* con = it->second;
        if (con->hasIdleTimeout() ||
            con->hasError() ||
            con->hasErrorTimeout() ||
            (!con->isActive() && waitingRequests))
        {
            if (con->hasErrorTimeout()) {
                // tell the connection we're timing it out
                con->handleTimeout();
            }

        // connection has been idle for a while, clean it up
        // (or if we have requests waiting for a different host,
        // or an error condition
            ConnectionDict::iterator del = it++;
            delete del->second;
            d->connections.erase(del);
        } else {
            if (it->second->shouldStartNext()) {
                it->second->tryStartNextRequest();
            }
            ++it;
        }
    } // of connection iteration

    if (waitingRequests && (d->connections.size() < d->maxConnections)) {
        RequestList waiting(d->pendingRequests);
        d->pendingRequests.clear();

        // re-submit all waiting requests in order; this takes care of
        // finding multiple pending items targetted to the same (new)
        // connection
        BOOST_FOREACH(Request_ptr req, waiting) {
            makeRequest(req);
        }
    }
#endif
}

void Client::makeRequest(const Request_ptr& r)
{
    if( r->isComplete() )
      return;

    if( r->url().find("://") == std::string::npos ) {
        r->setFailure(EINVAL, "malformed URL");
        return;
    }

    r->_client = this;

#if defined(ENABLE_CURL)
    CURL* curlRequest = curl_easy_init();
    curl_easy_setopt(curlRequest, CURLOPT_URL, r->url().c_str());

    // manually increase the ref count of the request
    SGReferenced::get(r.get());
    curl_easy_setopt(curlRequest, CURLOPT_PRIVATE, r.get());
    // disable built-in libCurl progress feedback
    curl_easy_setopt(curlRequest, CURLOPT_NOPROGRESS, 1);

    curl_easy_setopt(curlRequest, CURLOPT_WRITEFUNCTION, requestWriteCallback);
    curl_easy_setopt(curlRequest, CURLOPT_WRITEDATA, r.get());
    curl_easy_setopt(curlRequest, CURLOPT_HEADERFUNCTION, requestHeaderCallback);
    curl_easy_setopt(curlRequest, CURLOPT_HEADERDATA, r.get());

    curl_easy_setopt(curlRequest, CURLOPT_USERAGENT, d->userAgent.c_str());
    curl_easy_setopt(curlRequest, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    if (!d->proxy.empty()) {
      curl_easy_setopt(curlRequest, CURLOPT_PROXY, d->proxy.c_str());
      curl_easy_setopt(curlRequest, CURLOPT_PROXYPORT, d->proxyPort);

      if (!d->proxyAuth.empty()) {
        curl_easy_setopt(curlRequest, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curlRequest, CURLOPT_PROXYUSERPWD, d->proxyAuth.c_str());
      }
    }

    std::string method = boost::to_lower_copy(r->method());
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
    d->haveActiveRequests = true;

// FIXME - premature?
    r->requestStart();

#else
    if( r->url().find("http://") != 0 ) {
        r->setFailure(EINVAL, "only HTTP protocol is supported");
        return;
    }

    std::string host = r->host();
    int port = r->port();
    if (!d->proxy.empty()) {
        host = d->proxy;
        port = d->proxyPort;
    }

    Connection* con = NULL;
    std::stringstream ss;
    ss << host << "-" << port;
    std::string connectionId = ss.str();
    bool havePending = !d->pendingRequests.empty();
    bool atConnectionsLimit = d->connections.size() >= d->maxConnections;
    ConnectionDict::iterator consEnd = d->connections.end();

    // assign request to an existing Connection.
    // various options exist here, examined in order
    ConnectionDict::iterator it = d->connections.find(connectionId);
    if (atConnectionsLimit && (it == consEnd)) {
        // maximum number of connections active, queue this request
        // when a connection goes inactive, we'll start this one
        d->pendingRequests.push_back(r);
        return;
    }

    // scan for an idle Connection to the same host (likely if we're
    // retrieving multiple resources from the same host in quick succession)
    // if we have pending requests (waiting for a free Connection), then
    // force new requests on this id to always use the first Connection
    // (instead of the random selection below). This ensures that when
    // there's pressure on the number of connections to keep alive, one
    // host can't DoS every other.
    int count = 0;
    for (; (it != consEnd) && (it->first == connectionId); ++it, ++count) {
        if (havePending || !it->second->isActive()) {
            con = it->second;
            break;
        }
    }

    bool atHostConnectionsLimit = (count >= d->maxHostConnections);

    if (!con && (atConnectionsLimit || atHostConnectionsLimit)) {
        // all current connections are busy (active), and we don't
        // have free connections to allocate, so let's assign to
        // an existing one randomly. Ideally we'd used whichever one will
        // complete first but we don't have that info.
        int index = rand() % count;
        for (it = d->connections.find(connectionId); index > 0; --index, ++it) { ; }
        con = it->second;
    }

    // allocate a new connection object
    if (!con) {
        static int connectionSuffx = 0;

        std::stringstream ss;
        ss << connectionId << "-" << connectionSuffx++;

        SG_LOG(SG_IO, SG_DEBUG, "allocating new connection for ID:" << ss.str());
        con = new Connection(this, ss.str());
        con->setServer(host, port);
        con->setMaxPipelineLength(d->maxPipelineDepth);
        d->poller.addChannel(con);
        d->connections.insert(d->connections.end(),
            ConnectionDict::value_type(connectionId, con));
    }

    SG_LOG(SG_IO, SG_DEBUG, "queing request for " << r->url() << " on:" << con->connectionId());
    con->queueRequest(r);
#endif
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
  #if defined(ENABLE_CURL)
    return d->haveActiveRequests;
  #else
    ConnectionDict::const_iterator it = d->connections.begin();
    for (; it != d->connections.end(); ++it) {
        if (it->second->isActive()) return true;
    }

    return false;
#endif
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

size_t Client::requestHeaderCallback(char *rawBuffer, size_t size, size_t nitems, void *userdata)
{
  size_t byteSize = size * nitems;
  Request* req = static_cast<Request*>(userdata);
  std::string h = strutils::simplify(std::string(rawBuffer, byteSize));

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

  std::string key = strutils::simplify(h.substr(0, colonPos));
  std::string lkey = boost::to_lower_copy(key);
  std::string value = strutils::strip(h.substr(colonPos + 1));

  req->responseHeader(lkey, value);
  return byteSize;
}

void Client::debugDumpRequests()
{
#if defined(ENABLE_CURL)

#else
    SG_LOG(SG_IO, SG_INFO, "== HTTP connection dump");
    ConnectionDict::iterator it = d->connections.begin();
    for (; it != d->connections.end(); ++it) {
        it->second->debugDumpRequests();
    }
    SG_LOG(SG_IO, SG_INFO, "==");
#endif
}

void Client::clearAllConnections()
{
#if defined(ENABLE_CURL)
    curl_multi_cleanup(d->curlMulti);
    d->createCurlMulti();
#else
    ConnectionDict::iterator it = d->connections.begin();
    for (; it != d->connections.end(); ++it) {
        delete it->second;
    }
    d->connections.clear();
#endif
}

} // of namespace HTTP

} // of namespace simgear
