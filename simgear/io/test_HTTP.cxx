#include <cstdlib>

#include <iostream>
#include <map>
#include <sstream>

#include <boost/algorithm/string/case_conv.hpp>

#include "HTTPClient.hxx"
#include "HTTPRequest.hxx"

#include <simgear/io/sg_netChat.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stringstream;

using namespace simgear;

const char* BODY1 = "The quick brown fox jumps over a lazy dog.";

const unsigned int body2Size = 8 * 1024;
char body2[body2Size];

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        cerr << "\tgot:" << a << endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        cerr << "failed:" << #a << endl; \
        exit(1); \
    }
    
class TestRequest : public HTTP::Request
{
public:
    bool complete;
    string bodyData;
    
    TestRequest(const std::string& url) : 
        HTTP::Request(url),
        complete(false)
    {
        
    }
    
protected:
    virtual void responseHeadersComplete()
    {
    }
    
    virtual void responseComplete()
    {
        complete = true;
    }  
    
    virtual void gotBodyData(const char* s, int n)
    {
        bodyData += string(s, n);
    }
};

class TestServerChannel : public NetChat
{
public:  
    enum State
    {
        STATE_IDLE = 0,
        STATE_HEADERS,
        STATE_REQUEST_BODY
    };
    
    TestServerChannel()
    {
        state = STATE_IDLE;
        setTerminator("\r\n");
    }
    
    virtual void collectIncomingData(const char* s, int n)
    {
        buffer += string(s, n);
    }
    
    virtual void foundTerminator(void)
    {
        if (state == STATE_IDLE) {
            state = STATE_HEADERS;
            string_list line = strutils::split(buffer, NULL, 3);
            if (line.size() < 4) {
                cerr << "malformed request:" << buffer << endl;
                exit(-1);
            }
            
            method = line[0];
            path = line[1];
            httpVersion = line[2];
            userAgent = line[3];
            requestHeaders.clear();
            buffer.clear();
        } else if (state == STATE_HEADERS) {
            string s = strutils::simplify(buffer);
            if (s.empty()) {
                buffer.clear();
                receivedRequestHeaders();
                return;
            }
            
            int colonPos = buffer.find(':');
            if (colonPos < 0) {
                cerr << "malformed HTTP response header:" << buffer << endl;
                buffer.clear();
                return;
            }

            string key = strutils::simplify(buffer.substr(0, colonPos));
            string lkey = boost::to_lower_copy(key);
            string value = strutils::strip(buffer.substr(colonPos + 1));
            requestHeaders[lkey] = value;
            buffer.clear();
        } else if (state == STATE_REQUEST_BODY) {
            
        }
    }  
    
    void receivedRequestHeaders()
    {
        state = STATE_IDLE;
        if (path == "/test1") {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test2") {
            sendBody2();
        } else if (path == "http://www.google.com/test2") {
            // proxy test
            if (requestHeaders["host"] != "www.google.com") {
                sendErrorResponse(400);
            }
            
            if (requestHeaders["proxy-authorization"] != string()) {
                sendErrorResponse(401); // shouldn't supply auth
            }
            
            sendBody2();
        } else if (path == "http://www.google.com/test3") {
            // proxy test
            if (requestHeaders["host"] != "www.google.com") {
                sendErrorResponse(400);
            }

            if (requestHeaders["proxy-authorization"] != "ABCDEF") {
                sendErrorResponse(401); // forbidden
            }

            sendBody2();
        } else {
            sendErrorResponse(404);
        }
    }
    
    void sendBody2()
    {
        stringstream d;
        d << "HTTP1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
        d << "Content-Length:" << body2Size << "\r\n";
        d << "\r\n"; // final CRLF to terminate the headers
        push(d.str().c_str());
        bufferSend(body2, body2Size);
    }
    
    void sendErrorResponse(int code)
    {
        cerr << "sending error " << code << " for " << path << endl;
        stringstream headerData;
        headerData << "HTTP1.1 " << code << " " << reasonForCode(code) << "\r\n";
        headerData << "\r\n"; // final CRLF to terminate the headers
        push(headerData.str().c_str());
    }
    
    string reasonForCode(int code) 
    {
        switch (code) {
            case 200: return "OK";
            case 404: return "not found";
            default: return "unknown code";
        }
    }
    
    State state;
    string buffer;
    string method;
    string path;
    string httpVersion;
    string userAgent;
    std::map<string, string> requestHeaders;
};

class TestServer : public NetChannel
{
public:   
    TestServer()
    {
        open();
        bind(NULL, 2000); // localhost, any port
        listen(5);
    }
    
    virtual ~TestServer()
    {    
    }
    
    virtual bool writable (void) { return false ; }

    virtual void handleAccept (void)
    {
        simgear::IPAddress addr ;
        int handle = accept ( &addr ) ;

        TestServerChannel* chan = new TestServerChannel();
        chan->setHandle(handle);
    }
};

void waitForComplete(TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  1000) {
        NetChannel::poll(10);
        if (tr->complete) {
            return;
        }
    }
    
    cerr << "timed out" << endl;
}

int main(int argc, char* argv[])
{
    TestServer s;
    
    HTTP::Client cl;

// test URL parsing
    TestRequest* tr1 = new TestRequest("http://localhost.woo.zar:2000/test1?foo=bar");
    COMPARE(tr1->scheme(), "http");
    COMPARE(tr1->hostAndPort(), "localhost.woo.zar:2000");
    COMPARE(tr1->host(), "localhost.woo.zar");
    COMPARE(tr1->port(), 2000);
    COMPARE(tr1->path(), "/test1");
    
    TestRequest* tr2 = new TestRequest("http://192.168.1.1/test1/dir/thing/file.png");
    COMPARE(tr2->scheme(), "http");
    COMPARE(tr2->hostAndPort(), "192.168.1.1");
    COMPARE(tr2->host(), "192.168.1.1");
    COMPARE(tr2->port(), 80);
    COMPARE(tr2->path(), "/test1/dir/thing/file.png");
    
// basic get request
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseReason(), string("OK"));
        COMPARE(tr->contentLength(), strlen(BODY1));
        COMPARE(tr->bodyData, string(BODY1));
    }

// larger get request
    for (unsigned int i=0; i<body2Size; ++i) {
        body2[i] = (i << 4) | (i >> 2);
    }
    
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test2");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->contentLength(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
// test 404
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/not-found");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(tr);
        COMPARE(tr->responseCode(), 404);
        COMPARE(tr->responseReason(), string("not found"));
        COMPARE(tr->contentLength(), 0);
    }
    
// test proxy
    {
        cl.setProxy("localhost:2000");
        TestRequest* tr = new TestRequest("http://www.google.com/test2");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->contentLength(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
    {
        cl.setProxy("localhost:2000", "ABCDEF");
        TestRequest* tr = new TestRequest("http://www.google.com/test3");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->contentLength(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
}
