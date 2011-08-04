#include "HTTPClient.hxx"

#include <sstream>
#include <cassert>
#include <list>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/io/sg_netChat.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

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

class Connection : public NetChat
{
public:
    Connection(Client* pr) :
        client(pr),
        state(STATE_IDLE)
    {
        setTerminator("\r\n");
    }
    
    bool connectToHost(const string& host, short port)
    {
        if (!open()) {
            SG_LOG(SG_ALL, SG_WARN, "HTTP::Connection: connectToHost: open() failed");
            return false;
        }
        
        if (connect(host.c_str(), port) != 0) {
            return false;
        }
        
        return true;
    }
    
    // socket-level errors
    virtual void handleError(int error)
    {
        NetChat::handleError(error);
        if (activeRequest) {
            activeRequest->setFailure(error, "socket error");
            activeRequest = NULL;
        }
    
        state = STATE_SOCKET_ERROR;
    }
    
    void queueRequest(const Request_ptr& r)
    {
        if (!activeRequest) {
            startRequest(r);
        } else {
            queuedRequests.push_back(r);
        }
    }
    
    void startRequest(const Request_ptr& r)
    {
        activeRequest = r;
        state = STATE_IDLE;
        bodyTransferSize = 0;
        chunkedTransfer = false;
        
        stringstream headerData;
        string path = r->path();
        if (!client->proxyHost().empty()) {
            path = r->url();
        }

        headerData << r->method() << " " << path << " HTTP/1.1\r\n";
        headerData << "Host: " << r->hostAndPort() << "\r\n";
        headerData << "User-Agent:" << client->userAgent() << "\r\n";
        if (!client->proxyAuth().empty()) {
            headerData << "Proxy-Authorization: " << client->proxyAuth() << "\r\n";
        }

        BOOST_FOREACH(string h, r->requestHeaders()) {
            headerData << h << ": " << r->header(h) << "\r\n";
        }

        headerData << "\r\n"; // final CRLF to terminate the headers

    // TODO - add request body support for PUT, etc operations

        push(headerData.str().c_str());
    }
    
    virtual void collectIncomingData(const char* s, int n)
    {
        if ((state == STATE_GETTING_BODY) || (state == STATE_GETTING_CHUNKED_BYTES)) {
            activeRequest->processBodyBytes(s, n);
        } else {
            buffer += string(s, n);
        }
    }
    
    virtual void foundTerminator(void)
    {
        switch (state) {
        case STATE_IDLE:
            activeRequest->responseStart(buffer);
            state = STATE_GETTING_HEADERS;
            buffer.clear();
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
    
    bool hasError() const
    {
        return (state == STATE_SOCKET_ERROR);
    }
private:
    void processHeader()
    {
        string h = strutils::simplify(buffer);
        if (h.empty()) { // blank line terminates headers
            headersComplete();
            
            if (chunkedTransfer) {
                state = STATE_GETTING_CHUNKED;
            } else if (bodyTransferSize > 0) {
                state = STATE_GETTING_BODY;
                setByteCount(bodyTransferSize);
            } else {
                responseComplete();
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
        activeRequest->responseComplete();
        client->requestFinished(this);
        activeRequest = NULL;
        
        state = STATE_IDLE;
        setTerminator("\r\n");
        
        if (!queuedRequests.empty()) {
            Request_ptr next = queuedRequests.front();
            queuedRequests.pop_front();
            startRequest(next);
        } else {
            idleTime.stamp();
        }
    }
    
    enum ConnectionState {
        STATE_IDLE = 0,
        STATE_GETTING_HEADERS,
        STATE_GETTING_BODY,
        STATE_GETTING_CHUNKED,
        STATE_GETTING_CHUNKED_BYTES,
        STATE_GETTING_TRAILER,
        STATE_SOCKET_ERROR
    };
    
    Client* client;
    Request_ptr activeRequest;
    ConnectionState state;
    std::string buffer;
    int bodyTransferSize;
    SGTimeStamp idleTime;
    bool chunkedTransfer;
    
    std::list<Request_ptr> queuedRequests;
};

Client::Client()
{
    setUserAgent("SimGear-" SG_STRINGIZE(SIMGEAR_VERSION));
}

void Client::update()
{
    NetChannel::poll();
    
    ConnectionDict::iterator it = _connections.begin();
    for (; it != _connections.end(); ) {
        if (it->second->hasIdleTimeout() || it->second->hasError()) {
        // connection has been idle for a while, clean it up
            ConnectionDict::iterator del = it++;
            delete del->second;
            _connections.erase(del);
        } else {
            ++it;
        }
    } // of connecion iteration
}

void Client::makeRequest(const Request_ptr& r)
{
    string host = r->host();
    int port = r->port();
    if (!_proxy.empty()) {
        host = _proxy;
        port = _proxyPort;
    }
    
    stringstream ss;
    ss << host << "-" << port;
    string connectionId = ss.str();
    
    if (_connections.find(connectionId) == _connections.end()) {
        Connection* con = new Connection(this);
        bool ok = con->connectToHost(host, port);
        if (!ok) {
        // since NetChannel connect is non-blocking, this failure
        // path is unlikely, but still checked for.
            SG_LOG(SG_IO, SG_WARN, "unable to connect to host:" 
                << host << " (port:" << port << ")");
            delete con;
            
            r->setFailure(-1, "unable to connect to host");
            return;
        }
        
        _connections[connectionId] = con;
    }
    
    _connections[connectionId]->queueRequest(r);
}

void Client::requestFinished(Connection* con)
{
    
}

void Client::setUserAgent(const string& ua)
{
    _userAgent = ua;
}

void Client::setProxy(const string& proxy, int port, const string& auth)
{
    _proxy = proxy;
    _proxyPort = port;
    _proxyAuth = auth;
}

} // of namespace HTTP

} // of namespace simgear
