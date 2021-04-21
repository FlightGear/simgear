#ifndef SIMGEAR_IO_TEST_HTTP_HXX
#define SIMGEAR_IO_TEST_HTTP_HXX

#include <algorithm>
#include <sstream>
#include <vector>

#include <simgear/io/sg_netChat.hxx>
#include <simgear/misc/strutils.hxx>

namespace simgear
{

class TestServerChannel : public NetChat
{
public:
    enum State
    {
        STATE_IDLE = 0,
        STATE_HEADERS,
        STATE_CLOSING,
        STATE_REQUEST_BODY
    };

    TestServerChannel()
    {
        state = STATE_IDLE;
        setTerminator("\r\n");

    }

    virtual ~TestServerChannel()
    {
    }

    virtual void collectIncomingData(const char* s, int n)
    {
        buffer += std::string(s, n);
    }

    virtual void foundTerminator(void)
    {
        if (state == STATE_IDLE) {
            state = STATE_HEADERS;
            string_list line = strutils::split(buffer, NULL, 3);
            if (line.size() < 3) {
                std::cerr << "malformed request:" << buffer << std::endl;
                exit(-1);
            }

            method = line[0];
            path = line[1];

            std::string::size_type queryPos = path.find('?');
            if (queryPos != std::string::npos) {
                parseArgs(path.substr(queryPos + 1));
                path = path.substr(0, queryPos);
            }

            httpVersion = line[2];
            requestHeaders.clear();
            buffer.clear();
        } else if (state == STATE_HEADERS) {
            std::string s = strutils::simplify(buffer);
            if (s.empty()) {
                buffer.clear();
                receivedRequestHeaders();
                return;
            }

            std::string::size_type colonPos = buffer.find(':');
            if (colonPos == std::string::npos) {
                std::cerr << "test malformed HTTP response header:" << buffer << std::endl;
                buffer.clear();
                return;
            }

            std::string key = strutils::simplify(buffer.substr(0, colonPos));
            std::string value = strutils::strip(buffer.substr(colonPos + 1));
            requestHeaders[key] = value;
            buffer.clear();
        } else if (state == STATE_REQUEST_BODY) {
            receivedBody();
            setTerminator("\r\n");
        } else if (state == STATE_CLOSING) {
          // ignore!
        }
    }

    void parseArgs(const std::string& argData)
    {
        string_list argv = strutils::split(argData, "&");
        for (unsigned int a=0; a<argv.size(); ++a) {
            std::string::size_type eqPos = argv[a].find('=');
            if (eqPos == std::string::npos) {
                std::cerr << "malformed HTTP argument:" << argv[a] << std::endl;
                continue;
            }

            std::string key = argv[a].substr(0, eqPos);
            std::string value = argv[a].substr(eqPos + 1);
            args[key] = value;
        }
    }

    void receivedRequestHeaders()
    {
        state = STATE_IDLE;
        processRequestHeaders();
    }

    virtual void processRequestHeaders()
    {
      sendErrorResponse(404, false, "");
    }

    void closeAfterSending()
    {
      state = STATE_CLOSING;
      closeWhenDone();
    }

    void receivedBody()
    {
        state = STATE_IDLE;
        if (method == "POST") {
            parseArgs(buffer);
        }

        processRequestBody();

        buffer.clear();
    }

    virtual void processRequestBody()
    {
      sendErrorResponse(404, false, "");
    }

    void sendErrorResponse(int code, bool close, std::string content)
    {
     //   std::cerr << "sending error " << code << " for " << path << std::endl;
       // std::cerr << "\tcontent:" << content << std::endl;

        std::stringstream headerData;
        headerData << "HTTP/1.1 " << code << " " << reasonForCode(code) << "\r\n";
        headerData << "Content-Length:" << content.size() << "\r\n";
        headerData << "\r\n"; // final CRLF to terminate the headers
        push(headerData.str().c_str());
        push(content.c_str());

        if (close) {
            closeWhenDone();
        }
    }

    std::string reasonForCode(int code)
    {
        switch (code) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "no content";
            case 404: return "not found";
            case 407: return "proxy authentication required";
            default: return "unknown code";
        }
    }

    virtual void handleClose (void)
    {
        NetBufferChannel::handleClose();
    }


    State state;
    std::string buffer;
    std::string method;
    std::string path;
    std::string httpVersion;
    std::map<std::string, std::string> requestHeaders;
    std::map<std::string, std::string> args;
    int requestContentLength;
};

template <class T>
class TestServer : public NetChannel
{
    simgear::NetChannelPoller _poller;
    std::vector<T*> _channels;
public:
    TestServer()
    {
        Socket::initSockets();

        open();
        bind(NULL, 2000); // localhost, any port
        listen(16);

        _poller.addChannel(this);
    }

    virtual ~TestServer()
    {
        disconnectAll();
        _poller.removeChannel(this);
    }

    virtual bool writable (void) { return false ; }

    virtual void handleAccept (void)
    {
        simgear::IPAddress addr ;
        int handle = accept ( &addr ) ;
        T* chan = new T();
        chan->setHandle(handle);

        _channels.push_back(chan);
        _poller.addChannel(chan);
    }

    void poll()
    {
        _poller.poll();

        auto it = std::remove_if(_channels.begin(), _channels.end(), [&](T* channel) {
            if (channel->isClosed()) {
                _poller.removeChannel(channel);
                delete channel;
                return true;
            }
            return false;
        });

        _channels.erase(it, _channels.end());
    }

    int connectCount()
    {
        return _channels.size();
    }

    void disconnectAll()
    {
        typename std::vector<T*>::iterator it;
        for (it = _channels.begin(); it != _channels.end(); ++it) {
            _poller.removeChannel(*it);
            delete *it;
        }

        _channels.clear();
    }
};

} // of namespace simgear

#endif // of SIMGEAR_IO_TEST_HTTP_HXX
