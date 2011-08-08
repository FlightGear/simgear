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
#include <simgear/misc/sg_sleep.hxx>

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
    bool failed;
    string bodyData;
    
    TestRequest(const std::string& url) : 
        HTTP::Request(url),
        complete(false)
    {
    }
    
    std::map<string, string> sendHeaders;
    std::map<string, string> headers;
protected:
    string_list requestHeaders() const
    {
        string_list r;
        std::map<string, string>::const_iterator it;
        for (it = sendHeaders.begin(); it != sendHeaders.end(); ++it) {
            r.push_back(it->first);
        }
        return r;
    }
    
    string header(const string& name) const
    {
        std::map<string, string>::const_iterator it = sendHeaders.find(name);
        if (it == sendHeaders.end()) {
            return string();
        }
        
        return it->second;
    }
    
    virtual void responseHeadersComplete()
    {
    }
    
    virtual void responseComplete()
    {
        complete = true;
    }  
    
    virtual void failure()
    {
        failed = true;
    }
    
    virtual void gotBodyData(const char* s, int n)
    {
        bodyData += string(s, n);
    }
    
    virtual void responseHeader(const string& header, const string& value)
    {
        headers[header] =  value;
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
            if (line.size() < 3) {
                cerr << "malformed request:" << buffer << endl;
                exit(-1);
            }
            
            method = line[0];
            path = line[1];
            httpVersion = line[2];
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
            string value = strutils::strip(buffer.substr(colonPos + 1));
            requestHeaders[key] = value;
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
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_headers") {
            COMPARE(requestHeaders["X-Foo"], string("Bar"));
            COMPARE(requestHeaders["X-AnotherHeader"], string("A longer value"));
            
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test2") {
            sendBody2();
        } else if (path == "/testchunked") {
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Transfer-Encoding:chunked\r\n";
            d << "\r\n";
            d << "8\r\n"; // first chunk
            d << "ABCDEFGH\r\n";
            d << "6\r\n"; // second chunk
            d << "ABCDEF\r\n";
            d << "10\r\n"; // third chunk
            d << "ABCDSTUVABCDSTUV\r\n";
            d << "0\r\n"; // start of trailer
            d << "X-Foobar: wibble\r\n"; // trailer data
            d << "\r\n";
            push(d.str().c_str());
        } else if (path == "http://www.google.com/test2") {
            // proxy test
            if (requestHeaders["Host"] != "www.google.com") {
                sendErrorResponse(400, true, "bad destination");
            }
            
            if (requestHeaders["Proxy-Authorization"] != string()) {
                sendErrorResponse(401, false, "bad auth"); // shouldn't supply auth
            }
            
            sendBody2();
        } else if (path == "http://www.google.com/test3") {
            // proxy test
            if (requestHeaders["Host"] != "www.google.com") {
                sendErrorResponse(400, true, "bad destination");
            }

            if (requestHeaders["Proxy-Authorization"] != "ABCDEF") {
                sendErrorResponse(401, false, "bad auth"); // forbidden
            }

            sendBody2();
        } else if (strutils::starts_with(path, "/test_1_0")) {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.0 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
            closeWhenDone();
        } else if (path == "/test_close") {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Connection: close\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
            closeWhenDone();
        } else {
            sendErrorResponse(404, true, "");
        }
    }
    
    void sendBody2()
    {
        stringstream d;
        d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
        d << "Content-Length:" << body2Size << "\r\n";
        d << "\r\n"; // final CRLF to terminate the headers
        push(d.str().c_str());
        bufferSend(body2, body2Size);
    }
    
    void sendErrorResponse(int code, bool close, string content)
    {
        cerr << "sending error " << code << " for " << path << endl;
        stringstream headerData;
        headerData << "HTTP/1.1 " << code << " " << reasonForCode(code) << "\r\n";
        headerData << "Content-Length:" << content.size() << "\r\n";
        headerData << "\r\n"; // final CRLF to terminate the headers
        push(headerData.str().c_str());
        push(content.c_str());
        
        if (close) {
            closeWhenDone();
        }
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
        //cout << "did accept from " << addr.getHost() << ":" << addr.getPort() << endl;
        TestServerChannel* chan = new TestServerChannel();
        chan->setHandle(handle);
    }
};

void waitForComplete(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  1000) {
        cl->update();
        if (tr->complete) {
            return;
        }
        sleepForMSec(1);
    }
    
    cerr << "timed out" << endl;
}

void waitForFailed(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  1000) {
        cl->update();
        if (tr->failed) {
            return;
        }
        sleepForMSec(1);
    }
    
    cerr << "timed out waiting for failure" << endl;
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

        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseReason(), string("OK"));
        COMPARE(tr->responseLength(), strlen(BODY1));
        COMPARE(tr->responseBytesReceived(), strlen(BODY1));
        COMPARE(tr->bodyData, string(BODY1));
    }

    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_headers");
        HTTP::Request_ptr own(tr);
        tr->sendHeaders["X-Foo"] = "Bar";
        tr->sendHeaders["X-AnotherHeader"] = "A longer value";
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseReason(), string("OK"));
        COMPARE(tr->responseLength(), strlen(BODY1));
        COMPARE(tr->responseBytesReceived(), strlen(BODY1));
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
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseBytesReceived(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/testchunked");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseReason(), string("OK"));
        COMPARE(tr->responseBytesReceived(), 30);
        COMPARE(tr->bodyData, "ABCDEFGHABCDEFABCDSTUVABCDSTUV");
    // check trailers made it too
        COMPARE(tr->headers["x-foobar"], string("wibble"));
    }
    
// test 404
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/not-found");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 404);
        COMPARE(tr->responseReason(), string("not found"));
        COMPARE(tr->responseLength(), 0);
    }

    cout << "done1" << endl;
// test HTTP/1.0
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_1_0");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseLength(), strlen(BODY1));
        COMPARE(tr->bodyData, string(BODY1));
    }

    cout << "done2" << endl;
// test HTTP/1.1 Connection::close
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_close");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseLength(), strlen(BODY1));
        COMPARE(tr->bodyData, string(BODY1));
    }
    cout << "done3" << endl;
// test connectToHost failure
/*
    {
        TestRequest* tr = new TestRequest("http://not.found/something");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForFailed(tr);
        COMPARE(tr->responseCode(), -1);
    }
    */
// test proxy
    {
        cl.setProxy("localhost", 2000);
        TestRequest* tr = new TestRequest("http://www.google.com/test2");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseLength(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
    {
        cl.setProxy("localhost", 2000, "ABCDEF");
        TestRequest* tr = new TestRequest("http://www.google.com/test3");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        COMPARE(tr->responseCode(), 200);
        COMPARE(tr->responseBytesReceived(), body2Size);
        COMPARE(tr->bodyData, string(body2, body2Size));
    }
    
// pipelining
    {
        cl.setProxy("", 80);
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        
        
        TestRequest* tr2 = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);
        
        TestRequest* tr3 = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own3(tr3);
        cl.makeRequest(tr3);
        
        waitForComplete(&cl, tr3);
        VERIFY(tr->complete);
        VERIFY(tr2->complete);
        COMPARE(tr->bodyData, string(BODY1));
        COMPARE(tr2->bodyData, string(BODY1));
        COMPARE(tr3->bodyData, string(BODY1));
    }
    
// multiple requests with an HTTP 1.0 server
    {
        cout << "http 1.0 multiple requests" << endl;
        
        cl.setProxy("", 80);
        TestRequest* tr = new TestRequest("http://localhost:2000/test_1_0/A");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        
        TestRequest* tr2 = new TestRequest("http://localhost:2000/test_1_0/B");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);
        
        TestRequest* tr3 = new TestRequest("http://localhost:2000/test_1_0/C");
        HTTP::Request_ptr own3(tr3);
        cl.makeRequest(tr3);
        
        waitForComplete(&cl, tr3);
        VERIFY(tr->complete);
        VERIFY(tr2->complete);
        COMPARE(tr->bodyData, string(BODY1));
        COMPARE(tr2->bodyData, string(BODY1));
        COMPARE(tr3->bodyData, string(BODY1));
    }
    
    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
}
