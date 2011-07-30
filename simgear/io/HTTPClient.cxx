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
#    define SIMGEAR_VERSION "development " __DATE__
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
    
    void connectToHost(const string& host, short port)
    {
        open();
        connect(host.c_str(), port);
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
        
        stringstream headerData;
        string path = r->path();
        if (!client->proxyHost().empty()) {
            path = "http://" + r->hostAndPort() + path;
        }

        headerData << r->method() << " " << path << " HTTP/1.1 " << client->userAgent() << "\r\n";
        headerData << "Host: " << r->hostAndPort() << "\r\n";

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
        if (state == STATE_GETTING_BODY) {
            activeRequest->gotBodyData(s, n);
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
            state = STATE_IDLE;
            setTerminator("\r\n");
            
            if (!queuedRequests.empty()) {
                Request_ptr next = queuedRequests.front();
                queuedRequests.pop_front();
                startRequest(next);
            } else {
                idleTime.stamp();
            }
            
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
private:
    void processHeader()
    {
        string h = strutils::simplify(buffer);
        if (h.empty()) { // blank line terminates headers
            headersComplete();
            
            if (bodyTransferSize > 0) {
                state = STATE_GETTING_BODY;
                setByteCount(bodyTransferSize);
            } else {
                responseComplete();
                state = STATE_IDLE; // no response body, we're done
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
        
        if (lkey == "content-length" && (bodyTransferSize <= 0)) {
            bodyTransferSize = strutils::to_int(value);
        } else if (lkey == "transfer-length") {
            bodyTransferSize = strutils::to_int(value);
        }
        
        activeRequest->responseHeader(lkey, value);
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
    }
    
    enum ConnectionState {
        STATE_IDLE = 0,
        STATE_GETTING_HEADERS,
        STATE_GETTING_BODY
    };
    
    Client* client;
    Request_ptr activeRequest;
    ConnectionState state;
    std::string buffer;
    int bodyTransferSize;
    SGTimeStamp idleTime;
    
    std::list<Request_ptr> queuedRequests;
};

Client::Client()
{
    setUserAgent("SimGear-" SG_STRINGIZE(SIMGEAR_VERSION));
}

void Client::update()
{
    ConnectionDict::iterator it = _connections.begin();
    for (; it != _connections.end(); ) {
        if (it->second->hasIdleTimeout()) {
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
    string host = r->hostAndPort();
    if (!_proxy.empty()) {
        host = _proxy;
    }
    
    if (_connections.find(host) == _connections.end()) {
        Connection* con = new Connection(this);
        con->connectToHost(r->host(), r->port());
        _connections[host] = con;
    }
    
    _connections[host]->queueRequest(r);
}

void Client::requestFinished(Connection* con)
{
    
}

void Client::setUserAgent(const string& ua)
{
    _userAgent = ua;
}

void Client::setProxy(const string& proxy, const string& auth)
{
    _proxy = proxy;
    _proxyAuth = auth;
}

} // of namespace HTTP

} // of namespace simgear
