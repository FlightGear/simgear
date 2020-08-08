#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <cerrno>

#include <simgear/simgear_config.h>

#include "HTTPClient.hxx"
#include "HTTPRequest.hxx"

#include "test_HTTP.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/test_macros.hxx>

#include <curl/multi.h>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stringstream;

using namespace simgear;

const char* BODY1 = "The quick brown fox jumps over a lazy dog.";
const char* BODY3 = "Cras ut neque nulla. Duis ut velit neque, sit amet "
"pharetra risus. In est ligula, lacinia vitae congue in, sollicitudin at "
"libero. Mauris pharetra pretium elit, nec placerat dui semper et. Maecenas "
"magna magna, placerat sed luctus ac, commodo et ligula. Mauris at purus et "
"nisl molestie auctor placerat at quam. Donec sapien magna, venenatis sed "
"iaculis id, fringilla vel arcu. Duis sed neque nisi. Cras a arcu sit amet "
"risus ultrices varius. Integer sagittis euismod dui id varius. Cras vel "
"justo gravida metus.";

const unsigned int body2Size = 8 * 1024;
char body2[body2Size];


class TestRequest : public HTTP::Request
{
public:
    bool complete;
    bool failed;
    string bodyData;

    TestRequest(const std::string& url, const std::string method = "GET") :
        HTTP::Request(url, method),
        complete(false),
        failed(false)
    {

    }

    std::map<string, string> headers;
protected:

    void onDone() override
    {
        complete = true;
    }
 
    void onFail() override
    {
        failed = true;
    }

    void gotBodyData(const char* s, int n) override
    {
    //    std::cout << "got body data:'" << string(s, n) << "'" <<std::endl;
        bodyData += string(s, n);
    }

    void responseHeader(const string& header, const string& value) override
    {
        Request::responseHeader(header, value);
        headers[header] =  value;
    }
};

class HTTPTestChannel : public TestServerChannel
{
public:

    virtual void processRequestHeaders()
    {
        if (path == "/test1") {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/testLorem") {
            string contentStr(BODY3);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_zero_length_content") {
            string contentStr;
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_headers") {
            SG_CHECK_EQUAL(requestHeaders["X-Foo"], string("Bar"));
            SG_CHECK_EQUAL(requestHeaders["X-AnotherHeader"],
                           string("A longer value"));

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
                sendErrorResponse(401, false, "bad auth, not empty"); // shouldn't supply auth
            }

            sendBody2();
        } else if (path == "http://www.google.com/test3") {
            // proxy test
            if (requestHeaders["Host"] != "www.google.com") {
                sendErrorResponse(400, true, "bad destination");
            }

            string credentials = requestHeaders["Proxy-Authorization"];
            if (credentials.substr(0, 5) != "Basic") {
              // request basic auth
              stringstream d;
              d << "HTTP/1.1 " << 407 << " " << reasonForCode(407) << "\r\n";
              d << "WWW-Authenticate: Basic real=\"simgear\"\r\n";
              d << "\r\n"; // final CRLF to terminate the headers
              push(d.str().c_str());
              return;
            }

            std::vector<unsigned char> userAndPass;
            strutils::decodeBase64(credentials.substr(6), userAndPass);
            std::string decodedUserPass((char*) userAndPass.data(), userAndPass.size());

            if (decodedUserPass != "johndoe:swordfish") {
                std::map<string, string>::const_iterator it;
                for (it = requestHeaders.begin(); it != requestHeaders.end(); ++it) {
                  cerr << "header:" << it->first << " = " << it->second << endl;
                }

                sendErrorResponse(401, false, "bad auth, not as set"); // forbidden
            }

            sendBody2();
        } else if (strutils::starts_with(path, "/test_1_0")) {
            string contentStr(BODY1);
            if (strutils::ends_with(path, "/B")) {
                contentStr = BODY3;
            }
            stringstream d;
            d << "HTTP/1.0 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
            closeAfterSending();
        } else if (path == "/test_close") {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Connection: close\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
            closeAfterSending();
        } else if (path == "/test_abrupt_close") {
            // simulate server doing socket close before sending any
            // response - this used to cause a TerraSync failure since we
            // would get stuck restarting the request
            closeAfterSending();

        } else if (path == "/test_args") {
            if ((args["foo"] != "abc") || (args["bar"] != "1234") || (args["username"] != "johndoe")) {
                sendErrorResponse(400, true, "bad arguments");
                return;
            }

            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_post") {
            if (requestHeaders["Content-Type"] != "application/x-www-form-urlencoded") {
                cerr << "bad content type: '" << requestHeaders["Content-Type"] << "'" << endl;
                 sendErrorResponse(400, true, "bad content type");
                 return;
            }

            requestContentLength = strutils::to_int(requestHeaders["Content-Length"]);
            setByteCount(requestContentLength);
            state = STATE_REQUEST_BODY;
        } else if ((path == "/test_put") || (path == "/test_create")) {
              if (requestHeaders["Content-Type"] != "x-application/foobar") {
                  cerr << "bad content type: '" << requestHeaders["Content-Type"] << "'" << endl;
                   sendErrorResponse(400, true, "bad content type");
                   return;
              }

              requestContentLength = strutils::to_int(requestHeaders["Content-Length"]);
              setByteCount(requestContentLength);
              state = STATE_REQUEST_BODY;
        } else if (path == "/test_get_during_send") {
            // indicate we will send some number of bytes, but only send
            // some of them now.
            waitingOnNextRequestToContinue = true;

            string contentStr(BODY3, 100); // only send first 100 chars
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << strlen(BODY3) << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_get_during_send_2") {
            stringstream d;

            if (waitingOnNextRequestToContinue) {
                waitingOnNextRequestToContinue = false;
                // push the rest of the first request
                d << string(BODY3).substr(100);
            }


            // push the response to this request
            string contentStr(BODY1);
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/test_redirect") {
            string contentStr("<html>See <a href=\"wibble\">Here</a></html>");
            stringstream d;
            d << "HTTP/1.1 " << 302 << " " << "Found" << "\r\n";
            d << "Location:" << " http://localhost:2000/was_redirected" << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else if (path == "/was_redirected") {
            string contentStr(BODY1);
            stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentStr.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << contentStr;
            push(d.str().c_str());
        } else {
          TestServerChannel::processRequestHeaders();
        }
    }

    void processRequestBody()
    {
      if (path == "/test_post") {
            if ((args["foo"] != "abc") || (args["bar"] != "1234") || (args["username"] != "johndoe")) {
                sendErrorResponse(400, true, "bad arguments");
                return;
            }

            stringstream d;
            d << "HTTP/1.1 " << 204 << " " << reasonForCode(204) << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            push(d.str().c_str());
        } else if (path == "/test_put") {
          std::cerr << "sending PUT response" << std::endl;

          SG_CHECK_EQUAL(buffer, BODY3);
          stringstream d;
          d << "HTTP/1.1 " << 204 << " " << reasonForCode(204) << "\r\n";
          d << "\r\n"; // final CRLF to terminate the headers
          push(d.str().c_str());
        } else if (path == "/test_create") {
          std::cerr << "sending create response" << std::endl;

          std::string entityStr = "http://localhost:2000/something.txt";

          SG_CHECK_EQUAL(buffer, BODY3);
          stringstream d;
          d << "HTTP/1.1 " << 201 << " " << reasonForCode(201) << "\r\n";
          d << "Location:" << entityStr << "\r\n";
          d << "Content-Length:" << entityStr.size() << "\r\n";
          d << "\r\n"; // final CRLF to terminate the headers
          d << entityStr;

          push(d.str().c_str());
        } else {
          TestServerChannel::processRequestBody();
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

    bool waitingOnNextRequestToContinue;
};

TestServer<HTTPTestChannel> testServer;

void waitForComplete(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();
        testServer.poll();

        if (tr->complete) {
            return;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out" << endl;
}

void waitForFailed(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();
        testServer.poll();

        if (tr->failed) {
            return;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out waiting for failure" << endl;
}

using CompletionCheck = std::function<bool()>;

bool waitFor(HTTP::Client* cl, CompletionCheck ccheck)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();
        testServer.poll();

        if (ccheck()) {
            return true;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out" << endl;
    return false;
}


int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_INFO );

    HTTP::Client cl;
    // force all requests to use the same connection for this test
    cl.setMaxConnections(1);

// test URL parsing
    {
        auto tr1 = std::make_unique<TestRequest>("http://localhost.woo.zar:2000/test1?foo=bar");
        SG_CHECK_EQUAL(tr1->scheme(), "http");
        SG_CHECK_EQUAL(tr1->hostAndPort(), "localhost.woo.zar:2000");
        SG_CHECK_EQUAL(tr1->host(), "localhost.woo.zar");
        SG_CHECK_EQUAL(tr1->port(), 2000);
        SG_CHECK_EQUAL(tr1->path(), "/test1");
    }

    {
        auto tr2 = std::make_unique<TestRequest>("http://192.168.1.1/test1/dir/thing/file.png");
        SG_CHECK_EQUAL(tr2->scheme(), "http");
        SG_CHECK_EQUAL(tr2->hostAndPort(), "192.168.1.1");
        SG_CHECK_EQUAL(tr2->host(), "192.168.1.1");
        SG_CHECK_EQUAL(tr2->port(), 80);
        SG_CHECK_EQUAL(tr2->path(), "/test1/dir/thing/file.png");
    }

// basic get request
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

    {
      TestRequest* tr = new TestRequest("http://localhost:2000/testLorem");
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
      SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY3));
      SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY3));
      SG_CHECK_EQUAL(tr->bodyData, string(BODY3));
    }

    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_args?foo=abc&bar=1234&username=johndoe");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
    }

    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_headers");
        HTTP::Request_ptr own(tr);
        tr->requestHeader("X-Foo") = "Bar";
        tr->requestHeader("X-AnotherHeader") = "A longer value";
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

// larger get request
    for (unsigned int i=0; i<body2Size; ++i) {
        // this contains embeded 0s on purpose, i.e it's
        // not text data but binary
        body2[i] = (i << 4) | (i >> 2);
    }

    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test2");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseBytesReceived(), body2Size);
        SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
    }

    cerr << "testing chunked transfer encoding" << endl;
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/testchunked");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), 30);
        SG_CHECK_EQUAL(tr->bodyData, "ABCDEFGHABCDEFABCDSTUVABCDSTUV");
    // check trailers made it too
        SG_CHECK_EQUAL(tr->headers["x-foobar"], string("wibble"));
    }

// test 404
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/not-found");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 404);
        SG_CHECK_EQUAL(tr->responseReason(), string("not found"));
        SG_CHECK_EQUAL(tr->responseLength(), 0);
    }

    cout << "done 404 test" << endl;

    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_args?foo=abc&bar=1234&username=johndoe");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
    }

    cout << "done1" << endl;
// test HTTP/1.0
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_1_0");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

    cout << "done2" << endl;
// test HTTP/1.1 Connection::close
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_close");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }
    cout << "done3" << endl;
// test connectToHost failure

    {
        TestRequest* tr = new TestRequest("http://not.found/something");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForFailed(&cl, tr);



        const int HOST_NOT_FOUND_CODE = CURLE_COULDNT_RESOLVE_HOST;
        SG_CHECK_EQUAL(tr->responseCode(), HOST_NOT_FOUND_CODE);
    }

  cout << "testing abrupt close" << endl;
    // test server-side abrupt close
    {
        TestRequest* tr = new TestRequest("http://localhost:2000/test_abrupt_close");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForFailed(&cl, tr);

        const int SERVER_NO_DATA_CODE = CURLE_GOT_NOTHING;
        SG_CHECK_EQUAL(tr->responseCode(), SERVER_NO_DATA_CODE);
    }

cout << "testing proxy close" << endl;
// test proxy
    {
        cl.setProxy("localhost", 2000);
        TestRequest* tr = new TestRequest("http://www.google.com/test2");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), body2Size);
        SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
    }

    {
        cl.setProxy("localhost", 2000, "johndoe:swordfish");
        TestRequest* tr = new TestRequest("http://www.google.com/test3");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseBytesReceived(), body2Size);
        SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
    }

// pipelining
    cout << "testing HTTP 1.1 pipelining" << endl;

    {
        testServer.disconnectAll();
        cl.clearAllConnections();

        cl.setProxy("", 80);
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);


        TestRequest* tr2 = new TestRequest("http://localhost:2000/testLorem");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);

        TestRequest* tr3 = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own3(tr3);
        cl.makeRequest(tr3);

        SG_VERIFY(waitFor(&cl, [tr, tr2, tr3]() {
            return tr->complete && tr2->complete &&tr3->complete; 
        }));

        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

        SG_CHECK_EQUAL(tr2->responseLength(), strlen(BODY3));
        SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY3));
        SG_CHECK_EQUAL(tr2->bodyData, string(BODY3));

        SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));

        SG_CHECK_EQUAL(testServer.connectCount(), 1);
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

        SG_VERIFY(waitFor(&cl, [tr, tr2, tr3]() {
            return tr->complete && tr2->complete &&tr3->complete; 
        }));

        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

        SG_CHECK_EQUAL(tr2->responseLength(), strlen(BODY3));
        SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY3));
        SG_CHECK_EQUAL(tr2->bodyData, string(BODY3));
        SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
    }

// POST
    {
        cout << "testing POST" << endl;
        TestRequest* tr = new TestRequest("http://localhost:2000/test_post?foo=abc&bar=1234&username=johndoe", "POST");
        tr->setBodyData("", "application/x-www-form-urlencoded");

        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 204);
    }

    // PUT
        {
            cout << "testing PUT" << endl;
            TestRequest* tr = new TestRequest("http://localhost:2000/test_put", "PUT");
            tr->setBodyData(BODY3, "x-application/foobar");

            HTTP::Request_ptr own(tr);
            cl.makeRequest(tr);
            waitForComplete(&cl, tr);
            SG_CHECK_EQUAL(tr->responseCode(), 204);
        }

        {
            cout << "testing PUT create" << endl;
            TestRequest* tr = new TestRequest("http://localhost:2000/test_create", "PUT");
            tr->setBodyData(BODY3, "x-application/foobar");

            HTTP::Request_ptr own(tr);
            cl.makeRequest(tr);
            waitForComplete(&cl, tr);
            SG_CHECK_EQUAL(tr->responseCode(), 201);
        }

    // test_zero_length_content
    {
        cout << "zero-length-content-response" << endl;
        TestRequest* tr = new TestRequest("http://localhost:2000/test_zero_length_content");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->bodyData, string());
        SG_CHECK_EQUAL(tr->responseBytesReceived(), 0);
    }

    // test cancel
    {
        cout <<  "cancel  request" << endl;
        testServer.disconnectAll();
        cl.clearAllConnections();

        cl.setProxy("", 80);
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        TestRequest* tr2 = new TestRequest("http://localhost:2000/testLorem");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);

        TestRequest* tr3 = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own3(tr3);
        cl.makeRequest(tr3);

        cl.cancelRequest(tr, "my reason 1");

        cl.cancelRequest(tr2, "my reason 2");

        waitForComplete(&cl, tr3);

        SG_CHECK_EQUAL(tr->responseCode(), -1);
        SG_CHECK_EQUAL(tr2->responseReason(), "my reason 2");

        SG_CHECK_EQUAL(tr3->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr3->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
    }

    // test cancel
    {
        cout <<  "cancel middle request" << endl;
        testServer.disconnectAll();
        cl.clearAllConnections();

        cl.setProxy("", 80);
        TestRequest* tr = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        TestRequest* tr2 = new TestRequest("http://localhost:2000/testLorem");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);

        TestRequest* tr3 = new TestRequest("http://localhost:2000/test1");
        HTTP::Request_ptr own3(tr3);
        cl.makeRequest(tr3);

        cl.cancelRequest(tr2, "middle request");

        waitForComplete(&cl, tr3);

        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

        SG_CHECK_EQUAL(tr2->responseCode(), -1);

        SG_CHECK_EQUAL(tr3->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr3->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
    }

    // disabling this test for now, since it seems to have changed depending
    // on the libCurl version. (Or some other configuration which is currently
    // not apparent).
    // old behaviour: Curl sends the second request soon after makeRequest
    // new behaviour: Curl waits for the first request to complete, before
    // sending the second request (i.e acts as if HTTP pipelining is disabled)
#if 0
    {
        cout << "get-during-response-send\n\n" << endl;
        cl.clearAllConnections();
        //test_get_during_send

        TestRequest* tr = new TestRequest("http://localhost:2000/test_get_during_send");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        // kick things along
        for (int i=0; i<10; ++i) {
            SGTimeStamp::sleepForMSec(1);
            cl.update();
            testServer.poll();

        }

        TestRequest* tr2 = new TestRequest("http://localhost:2000/test_get_during_send_2");
        HTTP::Request_ptr own2(tr2);
        cl.makeRequest(tr2);

        SG_VERIFY(waitFor(&cl, [tr, tr2]() {
            return tr->isComplete() && tr2->isComplete();
        }));
        
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->bodyData, string(BODY3));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY3));
        SG_CHECK_EQUAL(tr2->responseCode(), 200);
        SG_CHECK_EQUAL(tr2->bodyData, string(BODY1));
        SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY1));
    }
#endif
    
    {
        cout << "redirect test" << endl;
        // redirect test
        testServer.disconnectAll();
        cl.clearAllConnections();
        
        TestRequest* tr = new TestRequest("http://localhost:2000/test_redirect");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
}
