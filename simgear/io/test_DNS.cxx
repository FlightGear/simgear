// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2024  Torsten Dreyer - torsten@flightgear.org

#include <cstdlib>

#include <atomic>
#include <errno.h>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>
#include <numeric> // for std::accumulate

#include <simgear/simgear_config.h>
#include "DNSClient.hxx"
#include "test_DNS.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/misc/test_macros.hxx>

using std::cout;
using std::cerr;
using std::endl;

using namespace simgear;

#define DNS_MAKE_REQUEST_AND_WAIT(__CLIENT__, __REQ__) \
        __CLIENT__.makeRequest(__REQ__);                     \
        while (!__REQ__->isComplete() && !__REQ__->isTimeout()) { \
            SGTimeStamp::sleepForMSec(200);            \
            __CLIENT__.update(0);                      \
        }                                              \
        SG_VERIFY(!__REQ__->isTimeout());

class Watchdog
{
public:
    Watchdog() : _interval(0), _timer(0), _running(false) {}
    ~Watchdog() {
      stop();
    }

    void start(unsigned int milliseconds)
    {
        _interval = milliseconds;
        _timer = 0;
        _running = true;
        _thread = std::thread(&Watchdog::loop, this);
    }

    void stop()
    {
        _running = false;
        _thread.join();
    }

private:
    unsigned int _interval;
    std::atomic<unsigned int> _timer;
    std::atomic<int> _running;
    std::thread _thread;

    void loop()
    {
        while (_running)
        {
            _timer++;

            if (_timer >= _interval)
            {
                _running = false;
                std::cerr << "Failure: timeout." << endl;
                exit(EXIT_FAILURE);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

void test_polling(DNS::Client& cl, int argc, char** argv)
{
    cout << "test update without prior pending request" << endl;

    cout << "polling.";
    for (int i = 0; i < 20; i++) {
        SGTimeStamp::sleepForMSec(200);
        cl.update(0);
        cout << ".";
        cout.flush();
    }
    cout << "done" << endl;
}

void test_existing_NAPTR( DNS::Client & cl, int argc, char ** argv )
{
    const std::string DN = argc > 1 ? argv[1] : "naptr.test.flightgear.org";
    cout << "test all seven existing NAPTR: " << DN << endl;

    DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest(DN);
    DNS::Request_ptr r(naptrRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    cout << "test for ascending preference/order" << endl;
    int order = -1, preference = -1;
    for (DNS::NAPTRRequest::NAPTR_list::const_iterator it = naptrRequest->entries.begin(); it != naptrRequest->entries.end(); ++it) {
        cout << "NAPTR " << (*it)->order << " " << (*it)->preference << " '" << (*it)->service << "' '" << (*it)->regexp << "' '" << (*it)->replacement << "'" << endl;
        // currently only support "U" which implies empty replacement
        SG_CHECK_EQUAL((*it)->flags, "U");
        SG_CHECK_EQUAL(naptrRequest->entries[0]->replacement, "");

        if ((*it)->order < order) {
            cerr << "NAPTR entries not ascending for field 'order'" << endl;
            exit(1);
        } else if ((*it)->order > order) {
            order = (*it)->order;
            preference = (*it)->preference;
        } else {
            if ((*it)->preference < preference) {
                cerr << "NAPTR entries not ascending for field 'preference', order=" << order << endl;
                exit(1);
            }
            preference = (*it)->preference;
        }

        SG_VERIFY (simgear::strutils::starts_with((*it)->regexp, "!^.*$!"));
        SG_VERIFY (simgear::strutils::ends_with((*it)->regexp, "!"));
    }
    SG_CHECK_EQUAL(naptrRequest->entries.size(), 7);
    cout << "test existing NAPTR: " << DN << " done." << endl;
}

void test_service_NAPTR(DNS::Client& cl, int argc, char** argv)
{
    const std::string DN = argc > 1 ? argv[1] : "naptr.test.flightgear.org";
    const std::string QSERVICE = argc > 2 ? argv[2] : "test";

    cout << "test four existing NAPTR " << DN << " with qservice: " << QSERVICE << endl;

    DNS::NAPTRRequest* naptrRequest = new DNS::NAPTRRequest(DN);
    naptrRequest->qservice = QSERVICE;
    DNS::Request_ptr r(naptrRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    for (DNS::NAPTRRequest::NAPTR_list::const_iterator it = naptrRequest->entries.begin(); it != naptrRequest->entries.end(); ++it) {
        cout << "NAPTR " << (*it)->order << " " << (*it)->preference << " '" << (*it)->service << "' '" << (*it)->regexp << "' '" << (*it)->replacement << "'" << endl;
        SG_CHECK_EQUAL(QSERVICE, (*it)->service);
    }
    SG_CHECK_EQUAL(naptrRequest->entries.size(), 4);
    cout << "test existing NAPTR " << DN << " with qservice: " << QSERVICE << " done." << endl;
}

void test_nonexiting_NAPTR(DNS::Client& cl, int argc, char** argv)
{
    cout << "test non-existing NAPTR" << endl;
    DNS::NAPTRRequest* naptrRequest = new DNS::NAPTRRequest("jurkxkqdiufqzpfvzqok.prozhqrlcaavbxifkkhf");
    DNS::Request_ptr r(naptrRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    SG_CHECK_EQUAL(naptrRequest->entries.size(), 0);
}

void test_existing_SRV(DNS::Client& cl, int argc, char** argv)
{
    const char* DN = "_fgms._udp.flightgear.org";
    cout << "test existing SRV: " << DN << endl;
    DNS::SRVRequest* srvRequest = new DNS::SRVRequest(DN);
    DNS::Request_ptr r(srvRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    SG_VERIFY(!srvRequest->entries.empty());

    for (DNS::SRVRequest::SRV_list::const_iterator it = srvRequest->entries.begin(); it != srvRequest->entries.end(); ++it) {
        cout << "SRV " << (*it)->priority << " " << (*it)->weight << " " << (*it)->port << " '" << (*it)->target << "'" << endl;
    }
}

void test_key_value_TXT(DNS::Client& cl, int argc, char** argv)
{
    const char* DN = "txt-test1.test.flightgear.org";

    cout << "test key-value TXT: " << DN << endl;
    DNS::TXTRequest* txtRequest = new DNS::TXTRequest(DN);
    DNS::Request_ptr r(txtRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    SG_VERIFY(!txtRequest->entries.empty());

    auto entry = txtRequest->entries.at(0);
    SG_CHECK_EQUAL(entry, "key=value");
    SG_CHECK_EQUAL(txtRequest->attributes.size(), 1);
    SG_CHECK_EQUAL (txtRequest->attributes["key"], "value");

    for (DNS::TXTRequest::TXT_list::const_iterator it = txtRequest->entries.begin(); it != txtRequest->entries.end(); ++it) {
        cout << "TXT " << " '" << (*it) << "'" << endl;
    }
}

void test_long_TXT(DNS::Client& cl, int argc, char** argv)
{
    const char* DN = "txt-test2.test.flightgear.org";

    cout << "test long TXT: " << DN << endl;
    DNS::TXTRequest* txtRequest = new DNS::TXTRequest(DN);
    DNS::Request_ptr r(txtRequest);
    DNS_MAKE_REQUEST_AND_WAIT(cl, r);

    // TXT records longer than 255 chars are split over multiple entries. Combine them together again.
    std::string all = std::accumulate(txtRequest->entries.begin(), txtRequest->entries.end(), std::string(""));
    cout << "TXT " << " '" << all << "' len=" << all.length() << endl;

    // Check start and and ending of well-known string.
    SG_VERIFY( simgear::strutils::starts_with( all, "Lorem ipsum" ));
    SG_VERIFY( simgear::strutils::ends_with( all, "est laborum." ));

    // The lorem ipsum in our TXT record is 431 chars long
    SG_CHECK_EQUAL(all.length(), 431);
}

int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_DEBUG );

    Watchdog watchdog;
    watchdog.start(100);

    simgear::Socket::initSockets();


    DNS::Client cl;

    test_polling(cl, argc, argv );

    test_existing_NAPTR( cl, argc, argv );
    test_service_NAPTR( cl, argc, argv );
    test_nonexiting_NAPTR( cl, argc, argv );
    test_existing_SRV( cl, argc, argv );
    test_key_value_TXT( cl, argc, argv );
    test_long_TXT(cl, argc, argv);

    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
    }
