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

#include <sstream>
#include <cassert>
#include <cstdlib> // rand()
#include <list>
#include <iostream>
#include <errno.h>
#include <map>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <zlib.h>

#include <simgear/io/sg_netChat.hxx>
#include <simgear/io/lowlevel.hxx>
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

using std::string;
using std::stringstream;
using std::vector;

namespace simgear
{

namespace HTTP
{

extern const int DEFAULT_HTTP_PORT = 80;
const char* CONTENT_TYPE_URL_ENCODED = "application/x-www-form-urlencoded";
const unsigned int MAX_INFLIGHT_REQUESTS = 32;
const int ZLIB_DECOMPRESS_BUFFER_SIZE = 32 * 1024;
const int ZLIB_INFLATE_WINDOW_BITS = -MAX_WBITS;
  
// see http://www.ietf.org/rfc/rfc1952.txt for these values and
// detailed description of the logic 
const int GZIP_HEADER_ID1 = 31;
const int GZIP_HEADER_ID2 = 139;
const int GZIP_HEADER_METHOD_DEFLATE = 8;
const unsigned int GZIP_HEADER_SIZE = 10;
const int GZIP_HEADER_FEXTRA = 1 << 2;
const int GZIP_HEADER_FNAME = 1 << 3;
const int GZIP_HEADER_COMMENT = 1 << 4;
const int GZIP_HEADER_CRC = 1 << 1;

class Connection;
typedef std::multimap<std::string, Connection*> ConnectionDict;
typedef std::list<Request_ptr> RequestList;

class Client::ClientPrivate
{
public:
    std::string userAgent;
    std::string proxy;
    int proxyPort;
    std::string proxyAuth;
    NetChannelPoller poller;
    unsigned int maxConnections;
    
    RequestList pendingRequests;
    
// connections by host (potentially more than one)
    ConnectionDict connections;
};
  
class Connection : public NetChat
{
public:
    Connection(Client* pr) :
        client(pr),
        state(STATE_CLOSED),
        port(DEFAULT_HTTP_PORT),
        zlibInflateBuffer(NULL),
        zlibInflateBufferSize(0),
        zlibOutputBuffer(NULL)
    {
        
    }
    
    virtual ~Connection()
    {
      if (zlibInflateBuffer) {
        free(zlibInflateBuffer);
      }
      
      if (zlibOutputBuffer) {
        free(zlibOutputBuffer);
      }
    }
  
    void setServer(const string& h, short p)
    {
        host = h;
        port = p;
    }
    
    // socket-level errors
    virtual void handleError(int error)
    {
        if (error == ENOENT) {
        // name lookup failure
            // we won't have an active request yet, so the logic below won't
            // fire to actually call setFailure. Let's fail all of the requests
            BOOST_FOREACH(Request_ptr req, sentRequests) {
                req->setFailure(error, "hostname lookup failure");
            }
            
            BOOST_FOREACH(Request_ptr req, queuedRequests) {
                req->setFailure(error, "hostname lookup failure");
            }
            
        // name lookup failure, abandon all requests on this connection
            sentRequests.clear();
            queuedRequests.clear();
        }
        
        NetChat::handleError(error);
        if (activeRequest) {            
            SG_LOG(SG_IO, SG_INFO, "HTTP socket error");
            activeRequest->setFailure(error, "socket error");
            activeRequest = NULL;
        }
    
        state = STATE_SOCKET_ERROR;
    }
    
    virtual void handleClose()
    {      
        NetChat::handleClose();

    // closing of the connection from the server side when getting the body,
        bool canCloseState = (state == STATE_GETTING_BODY);
        if (canCloseState && activeRequest) {
        // force state here, so responseComplete can avoid closing the 
        // socket again
            state =  STATE_CLOSED;
            responseComplete();
        } else {
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
            }
            
            state = STATE_CLOSED;
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
      
      activeRequest = sentRequests.front();      
      activeRequest->responseStart(buffer);
      state = STATE_GETTING_HEADERS;
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
      contentGZip = contentDeflate = false;
    }
  
    void tryStartNextRequest()
    {
      if (queuedRequests.empty()) {
        idleTime.stamp();
        return;
      }
      
      if (sentRequests.size() > MAX_INFLIGHT_REQUESTS) {
        return;
      }
      
      if (state == STATE_CLOSED) {
          if (!connectToHost()) {
              return;
          }
          
          setTerminator("\r\n");
          state = STATE_IDLE;
      }
     
      Request_ptr r = queuedRequests.front();
      r->requestStart();
      requestBodyBytesToSend = r->requestBodyLength();
          
      stringstream headerData;
      string path = r->path();
      assert(!path.empty());
      string query = r->query();
      string bodyData;
      
      if (!client->proxyHost().empty()) {
          path = r->scheme() + "://" + r->host() + r->path();
      }

      if (r->requestBodyType() == CONTENT_TYPE_URL_ENCODED) {
          headerData << r->method() << " " << path << " HTTP/1.1\r\n";
          bodyData = query.substr(1); // URL-encode, drop the leading '?'
          headerData << "Content-Type:" << CONTENT_TYPE_URL_ENCODED << "\r\n";
          headerData << "Content-Length:" << bodyData.size() << "\r\n";
      } else {
          headerData << r->method() << " " << path << query << " HTTP/1.1\r\n";
          if (requestBodyBytesToSend >= 0) {
            headerData << "Content-Length:" << requestBodyBytesToSend << "\r\n";
            headerData << "Content-Type:" << r->requestBodyType() << "\r\n";
          }
      }
      
      headerData << "Host: " << r->hostAndPort() << "\r\n";
      headerData << "User-Agent:" << client->userAgent() << "\r\n";
      headerData << "Accept-Encoding: deflate, gzip\r\n";
      if (!client->proxyAuth().empty()) {
          headerData << "Proxy-Authorization: " << client->proxyAuth() << "\r\n";
      }

      BOOST_FOREACH(string h, r->requestHeaders()) {
          headerData << h << ": " << r->header(h) << "\r\n";
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
      
      while (requestBodyBytesToSend > 0) {
        char buf[4096];
        int len = r->getBodyData(buf, 4096);
        if (len > 0) {
          requestBodyBytesToSend -= len;
          if (!bufferSend(buf, len)) {
            SG_LOG(SG_IO, SG_WARN, "overflow the HTTP::Connection output buffer");
            state = STATE_SOCKET_ERROR;
            return;
          }
      //    SG_LOG(SG_IO, SG_INFO, "sent body:\n" << string(buf, len) << "\n%%%%%%%%%");
        } else {
          SG_LOG(SG_IO, SG_WARN, "HTTP asynchronous request body generation is unsupported");
          break;
        }
      }
      
      SG_LOG(SG_IO, SG_DEBUG, "did start request:" << r->url() <<
          "\n\t @ " << reinterpret_cast<void*>(r.ptr()) <<
          "\n\t on connection " << this);
    // successfully sent, remove from queue, and maybe send the next
      queuedRequests.pop_front();
      sentRequests.push_back(r);
      
    // pipelining, let's maybe send the next request right away
      tryStartNextRequest();
    }
    
    virtual void collectIncomingData(const char* s, int n)
    {
        idleTime.stamp();
        if ((state == STATE_GETTING_BODY) || (state == STATE_GETTING_CHUNKED_BYTES)) {
          if (contentGZip || contentDeflate) {
            expandCompressedData(s, n);
          } else {
            activeRequest->processBodyBytes(s, n);
          }
        } else {
            buffer += string(s, n);
        }
    }

    
    void expandCompressedData(const char* s, int n)
    {
      int reqSize = n + zlib.avail_in;
      if (reqSize > zlibInflateBufferSize) {
      // reallocate
        unsigned char* newBuf = (unsigned char*) malloc(reqSize);
        memcpy(newBuf, zlib.next_in, zlib.avail_in);
        memcpy(newBuf + zlib.avail_in, s, n);
        free(zlibInflateBuffer);
        zlibInflateBuffer = newBuf;
        zlibInflateBufferSize = reqSize;
      } else {
      // important to use memmove here, since it's very likely
      // the source and destination ranges overlap
        memmove(zlibInflateBuffer, zlib.next_in, zlib.avail_in);
        memcpy(zlibInflateBuffer + zlib.avail_in, s, n);
      }
            
      zlib.next_in = (unsigned char*) zlibInflateBuffer;
      zlib.avail_in = reqSize;
      zlib.next_out = zlibOutputBuffer;
      zlib.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
      
      if (contentGZip && !handleGZipHeader()) {
          return;
      }
      
      int writtenSize = 0;
      do {
        int result = inflate(&zlib, Z_NO_FLUSH);
        if (result == Z_OK || result == Z_STREAM_END) {
            // nothing to do
        } else {
          SG_LOG(SG_IO, SG_WARN, "HTTP: got Zlib error:" << result);
          return;
        }
            
        writtenSize = ZLIB_DECOMPRESS_BUFFER_SIZE - zlib.avail_out;
        if (result == Z_STREAM_END) {
            break;
        }
      } while ((writtenSize == 0) && (zlib.avail_in > 0));
    
      if (writtenSize > 0) {
        activeRequest->processBodyBytes((const char*) zlibOutputBuffer, writtenSize);
      }
    }
    
    bool handleGZipHeader()
    {
        // we clear this down to contentDeflate once the GZip header has been seen
        if (zlib.avail_in < GZIP_HEADER_SIZE) {
          return false; // need more header bytes
        }
        
        if ((zlibInflateBuffer[0] != GZIP_HEADER_ID1) ||
            (zlibInflateBuffer[1] != GZIP_HEADER_ID2) ||
            (zlibInflateBuffer[2] != GZIP_HEADER_METHOD_DEFLATE))
        {
          return false; // invalid GZip header
        }
        
        char flags = zlibInflateBuffer[3];
        unsigned int gzipHeaderSize =  GZIP_HEADER_SIZE;
        if (flags & GZIP_HEADER_FEXTRA) {
          gzipHeaderSize += 2;
          if (zlib.avail_in < gzipHeaderSize) {
            return false; // need more header bytes
          }
          
          unsigned short extraHeaderBytes = *(reinterpret_cast<unsigned short*>(zlibInflateBuffer + GZIP_HEADER_FEXTRA));
          if ( sgIsBigEndian() ) {
              sgEndianSwap( &extraHeaderBytes );
          }
          
          gzipHeaderSize += extraHeaderBytes;
          if (zlib.avail_in < gzipHeaderSize) {
            return false; // need more header bytes
          }
        }
        
        if (flags & GZIP_HEADER_FNAME) {
          gzipHeaderSize++;
          while (gzipHeaderSize <= zlib.avail_in) {
            if (zlibInflateBuffer[gzipHeaderSize-1] == 0) {
              break; // found terminating NULL character
            }
          }
        }
        
        if (flags & GZIP_HEADER_COMMENT) {
          gzipHeaderSize++;
          while (gzipHeaderSize <= zlib.avail_in) {
            if (zlibInflateBuffer[gzipHeaderSize-1] == 0) {
              break; // found terminating NULL character
            }
          }
        }
        
        if (flags & GZIP_HEADER_CRC) {
          gzipHeaderSize += 2;
        }
        
        if (zlib.avail_in < gzipHeaderSize) {
          return false; // need more header bytes
        }
        
        zlib.next_in += gzipHeaderSize;
        zlib.avail_in -= gzipHeaderSize;
      // now we've processed the GZip header, can decode as deflate
        contentGZip = false;
        contentDeflate = true;
        return true;
    }
    
    virtual void foundTerminator(void)
    {
        idleTime.stamp();
        switch (state) {
        case STATE_IDLE:
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
            state = STATE_GETTING_CHUNKED;
            buffer.clear();
            break;
            

        case STATE_GETTING_TRAILER:
            processTrailer();
            buffer.clear();
            break;
        
        default:
            break;
        }
    }
    
    bool hasIdleTimeout() const
    {
        if (state != STATE_IDLE) {
            return false;
        }
        
        return idleTime.elapsedMSec() > 1000 * 10; // ten seconds
    }
  
    bool hasErrorTimeout() const
    {
      if (state == STATE_IDLE) {
        return false;
      }
      
      return idleTime.elapsedMSec() > (1000 * 30); // 30 seconds
    }
    
    bool hasError() const
    {
        return (state == STATE_SOCKET_ERROR);
    }
    
    bool shouldStartNext() const
    {
      return !queuedRequests.empty() && (sentRequests.size() < MAX_INFLIGHT_REQUESTS);
    }
    
    bool isActive() const
    {
        return !queuedRequests.empty() || !sentRequests.empty();
    }
private:
    bool connectToHost()
    {
        SG_LOG(SG_IO, SG_DEBUG, "HTTP connecting to " << host << ":" << port);
        
        if (!open()) {
            SG_LOG(SG_ALL, SG_WARN, "HTTP::Connection: connectToHost: open() failed");
            return false;
        }
        
        if (connect(host.c_str(), port) != 0) {
            return false;
        }
        
        return true;
    }
    
    
    void processHeader()
    {
        string h = strutils::simplify(buffer);
        if (h.empty()) { // blank line terminates headers
            headersComplete();
            
            if (contentGZip || contentDeflate) {
                memset(&zlib, 0, sizeof(z_stream));
                if (!zlibOutputBuffer) {
                    zlibOutputBuffer = (unsigned char*) malloc(ZLIB_DECOMPRESS_BUFFER_SIZE);
                }
            
                // NULLs means we'll get default alloc+free methods
                // which is absolutely fine
                zlib.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
                zlib.next_out = zlibOutputBuffer;
                if (inflateInit2(&zlib, ZLIB_INFLATE_WINDOW_BITS) != Z_OK) {
                  SG_LOG(SG_IO, SG_WARN, "inflateInit2 failed");
                }
            }
          
            if (chunkedTransfer) {
                state = STATE_GETTING_CHUNKED;
            } else if (noMessageBody || (bodyTransferSize == 0)) {
                // force the state to GETTING_BODY, to simplify logic in
                // responseComplete and handleClose
                state = STATE_GETTING_BODY;
                responseComplete();
            } else {
                setByteCount(bodyTransferSize); // may be -1, that's fine              
                state = STATE_GETTING_BODY;
            }
            
            return;
        }
              
        int colonPos = buffer.find(':');
        if (colonPos < 0) {
            SG_LOG(SG_IO, SG_WARN, "malformed HTTP response header:" << h);
            return;
        }
        
        string key = strutils::simplify(buffer.substr(0, colonPos));
        string lkey = boost::to_lower_copy(key);
        string value = strutils::strip(buffer.substr(colonPos + 1));
        
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
              if (value == "gzip") {
                contentGZip = true;
              } else if (value == "deflate") {
                contentDeflate = true;
              } else if (value != "identity") {
                SG_LOG(SG_IO, SG_WARN, "unsupported content encoding:" << value);
              }
            }
        }
    
        activeRequest->responseHeader(lkey, value);
    }
    
    void processTransferEncoding(const string& te)
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
            state = STATE_GETTING_TRAILER;
            return;
        }
        
        state = STATE_GETTING_CHUNKED_BYTES;
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
    }
    
    void responseComplete()
    {
     //   SG_LOG(SG_IO, SG_INFO, "*** responseComplete:" << activeRequest->url());
        activeRequest->responseComplete();
        client->requestFinished(this);
      
        if (contentDeflate) {
          inflateEnd(&zlib);
        }
      
        assert(sentRequests.front() == activeRequest);
        sentRequests.pop_front();
        bool doClose = activeRequest->closeAfterComplete();
        activeRequest = NULL;
      
        if ((state == STATE_GETTING_BODY) || (state == STATE_GETTING_TRAILER)) {
            if (doClose) {
          // this will bring us into handleClose() above, which updates
          // state to STATE_CLOSED
            close();
              
          // if we have additional requests waiting, try to start them now
            tryStartNextRequest();
          }
        }
        
      if (state != STATE_CLOSED)  {
        state = STATE_IDLE;
      }
      setTerminator("\r\n");
    }
    
    enum ConnectionState {
        STATE_IDLE = 0,
        STATE_GETTING_HEADERS,
        STATE_GETTING_BODY,
        STATE_GETTING_CHUNKED,
        STATE_GETTING_CHUNKED_BYTES,
        STATE_GETTING_TRAILER,
        STATE_SOCKET_ERROR,
        STATE_CLOSED             ///< connection should be closed now
    };
    
    Client* client;
    Request_ptr activeRequest;
    ConnectionState state;
    string host;
    short port;
    std::string buffer;
    int bodyTransferSize;
    SGTimeStamp idleTime;
    bool chunkedTransfer;
    bool noMessageBody;
    int requestBodyBytesToSend;
  
    z_stream zlib;
    unsigned char* zlibInflateBuffer;
    int zlibInflateBufferSize;
    unsigned char* zlibOutputBuffer;
    bool contentGZip, contentDeflate;
  
    RequestList queuedRequests;
    RequestList sentRequests;
};

Client::Client() :
    d(new ClientPrivate)
{
    d->proxyPort = 0;
    d->maxConnections = 4;
    setUserAgent("SimGear-" SG_STRINGIZE(SIMGEAR_VERSION));
}

Client::~Client()
{
}

void Client::setMaxConnections(unsigned int maxCon)
{
    if (maxCon < 1) {
        throw sg_range_exception("illegal HTTP::Client::setMaxConnections value");
    }
    
    d->maxConnections = maxCon;
}

void Client::update(int waitTimeout)
{
    d->poller.poll(waitTimeout);
    bool waitingRequests = !d->pendingRequests.empty();
    
    ConnectionDict::iterator it = d->connections.begin();
    for (; it != d->connections.end(); ) {
        Connection* con = it->second;
        if (con->hasIdleTimeout() || 
            con->hasError() ||
            con->hasErrorTimeout() ||
            (!con->isActive() && waitingRequests))
        {
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
}

void Client::makeRequest(const Request_ptr& r)
{
    string host = r->host();
    int port = r->port();
    if (!d->proxy.empty()) {
        host = d->proxy;
        port = d->proxyPort;
    }
    
    Connection* con = NULL;
    stringstream ss;
    ss << host << "-" << port;
    string connectionId = ss.str();
    bool havePending = !d->pendingRequests.empty();
    ConnectionDict::iterator consEnd = d->connections.end();
     
    // assign request to an existing Connection.
    // various options exist here, examined in order
    if (d->connections.size() >= d->maxConnections) {
        ConnectionDict::iterator it = d->connections.find(connectionId);
        if (it == consEnd) {
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
        
        if (!con) {
            // we have at least one connection to the host, but they are
            // all active - we need to pick one to queue the request on.
            // we use random but round-robin would also work.
            int index = rand() % count;
            for (it = d->connections.find(connectionId); index > 0; --index) { ; }
            con = it->second;
        }
    } // of at max connections limit
    
    // allocate a new connection object
    if (!con) {
        con = new Connection(this);
        con->setServer(host, port);
        d->poller.addChannel(con);
        d->connections.insert(d->connections.end(), 
            ConnectionDict::value_type(connectionId, con));
    }
    
    con->queueRequest(r);
}

void Client::requestFinished(Connection* con)
{
    
}

void Client::setUserAgent(const string& ua)
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

void Client::setProxy(const string& proxy, int port, const string& auth)
{
    d->proxy = proxy;
    d->proxyPort = port;
    d->proxyAuth = auth;
}

bool Client::hasActiveRequests() const
{
    ConnectionDict::const_iterator it = d->connections.begin();
    for (; it != d->connections.end(); ++it) {
        if (it->second->isActive()) return true;
    }
    
    return false;
}

} // of namespace HTTP

} // of namespace simgear
