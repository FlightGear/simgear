#include <simgear_config.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <signal.h>

#include <iostream>

#include <simgear/io/sg_file.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRepository.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/logstream.hxx>

using namespace simgear;
using std::cout;
using std::endl;
using std::cerr;
using std::string;

int main(int argc, char* argv[])
{
    HTTP::Client cl;
    string proxy, proxyAuth;
    string_list headers;
    string url;

    sglog().setLogLevels( SG_ALL, SG_INFO );

    for (int a=0; a<argc;++a) {
        if (argv[a][0] == '-') {
            if (!strcmp(argv[a], "--proxy")) {
                proxy = argv[++a];
            } else if (!strcmp(argv[a], "--auth")) {
                proxyAuth = argv[++a];
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

    if (url.empty()) {
        cerr << "no URL argument specificed" << endl;
        return EXIT_FAILURE;
    }

    SGPath rootPath = simgear::Dir::current().path();
    auto repo = std::make_unique<HTTPRepository>(rootPath, &cl);
    repo->setBaseUrl(url);
    repo->update();

    while (repo->isDoingSync()) {
        cl.update();
        SGTimeStamp::sleepForMSec(100);
    }

    if (repo->failure() != HTTPRepository::REPO_NO_ERROR) {
        cerr << "got response:" << repo->failure() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
