#include <simgear_config.h>

#include <cstdio>
#include <cstring>
#include <signal.h>

#include <iostream>

#include <simgear/io/sg_file.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/sg_netChannel.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>

using namespace simgear;
using std::cout;
using std::endl;
using std::cerr;
using std::string;

class ARequest : public HTTP::Request
{
public:
    ARequest(string& url) :
        Request(url),
        _complete(false),
        _file(NULL)
    {

    }

    void setFile(SGFile* f)
    {
        _file = f;
    }

    bool complete() const
        { return _complete; }

    void addHeader(const string& h)
    {
        int colonPos = h.find(':');
        if (colonPos < 0) {
            cerr << "malformed header: " << h << endl;
            return;
        }

        string key = h.substr(0, colonPos);
        requestHeader(key) = h.substr(colonPos + 1);
    }

protected:
    virtual void onDone()
    {
        _complete = true;
    }

    virtual void gotBodyData(const char* s, int n)
    {
        _file->write(s, n);
    }
private:
    bool _complete;
    SGFile* _file;
};

int main(int argc, char* argv[])
{
    HTTP::Client cl;
    SGFile* outFile = 0;
    string proxy, proxyAuth;
    string_list headers;
    string url;

    for (int a=0; a<argc;++a) {
        if (argv[a][0] == '-') {
            if (!strcmp(argv[a], "--user-agent")) {
                cl.setUserAgent(argv[++a]);
            } else if (!strcmp(argv[a], "--proxy")) {
                proxy = argv[++a];
            } else if (!strcmp(argv[a], "--auth")) {
                proxyAuth = argv[++a];
            } else if (!strcmp(argv[a], "-f") || !strcmp(argv[a], "--file")) {
                outFile = new SGFile(SGPath::fromLocal8Bit(argv[++a]));
                if (!outFile->open(SG_IO_OUT)) {
                    cerr << "failed to open output for writing:" << outFile->get_file_name() << endl;
                    return EXIT_FAILURE;
                }
            } else if (!strcmp(argv[a], "--header")) {
                headers.push_back(argv[++a]);
            }
        } else { // of argument starts with a hyphen
            url = argv[a];
        }
    } // of arguments iteration

    if (!proxy.empty()) {
        int colonPos = proxy.find(':');
        string proxyHost = proxy;
        int proxyPort = 8800;
        if (colonPos >= 0) {
            proxyHost = proxy.substr(0, colonPos);
            proxyPort = strutils::to_int(proxy.substr(colonPos + 1));
        }

        cl.setProxy(proxyHost, proxyPort, proxyAuth);
    }

#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    if (!outFile) {
        outFile = new SGFile(fileno(stdout));
    }

    if (url.empty()) {
        cerr << "no URL argument specificed" << endl;
        return EXIT_FAILURE;
    }

    ARequest* req = new ARequest(url);
    for (const auto& h : headers) {
        req->addHeader(h);
    }

    req->setFile(outFile);
    cl.makeRequest(req);

    while (!req->complete()) {
        cl.update();
        SGTimeStamp::sleepForMSec(100);
    }

    if (req->responseCode() != 200) {
        cerr << "got response:" << req->responseCode() << endl;
        cerr << "\treason:" << req->responseReason() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
