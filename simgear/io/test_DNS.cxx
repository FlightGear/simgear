#include <cstdlib>

#include <iostream>
#include <map>
#include <sstream>
#include <errno.h>
#include <thread>
#include <atomic>

#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/simgear_config.h>

#include "DNSClient.hxx"

#include "test_DNS.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>

using std::cout;
using std::cerr;
using std::endl;

using namespace simgear;

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        cerr << "\tgot:'" << a << "'" << endl; \
        exit(1); \
    }

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
    std::atomic<bool> _running;
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


int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_DEBUG );

    Watchdog watchdog;
    watchdog.start(100);

    simgear::Socket::initSockets();


    DNS::Client cl;

    cout << "test update without prior pending request" << endl;
    {
        cout << "polling.";
        for( int i = 0; i < 20; i++ ) {
            SGTimeStamp::sleepForMSec(200);
            cl.update(0);
            cout << ".";
            cout.flush();
        }
        cout << "done" << endl;
    }

#define EXISTING_RECORD "terrasync.flightgear.org"
    cout << "test existing NAPTR: " EXISTING_RECORD << endl;
    {
        DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest(EXISTING_RECORD);
        DNS::Request_ptr r(naptrRequest);
        cl.makeRequest(r);
        while( !r->isComplete() && !r->isTimeout()) {
            SGTimeStamp::sleepForMSec(200);
            cl.update(0);
        }

        if( r->isTimeout() ) {
            cerr << "timeout testing existing record " EXISTING_RECORD << endl;
            return EXIT_FAILURE;
        }
        if(naptrRequest->entries.empty()) {
            cerr << "no results for " EXISTING_RECORD << endl;
            return EXIT_FAILURE;
        }

        cout << "test for ascending preference/order" << endl;
        int order = -1, preference = -1;
        for( DNS::NAPTRRequest::NAPTR_list::const_iterator it = naptrRequest->entries.begin(); it != naptrRequest->entries.end(); ++it ) {
            // currently only support "U" which implies empty replacement
            COMPARE((*it)->flags, "U" );
            COMPARE(naptrRequest->entries[0]->replacement, "" );

            // currently only support ws20
            COMPARE((*it)->service, "ws20" );

            if( (*it)->order < order ) {
                cerr << "NAPTR entries not ascending for field 'order'" << endl;
                return EXIT_FAILURE;
            } else if( (*it)->order > order ) {
                order = (*it)->order;
                preference = (*it)->preference;
            } else {
                if( (*it)->preference < preference ) {
                    cerr << "NAPTR entries not ascending for field 'preference', order=" << order << endl;
                    return EXIT_FAILURE;
                }
                preference = (*it)->preference;
            }

            if( false == simgear::strutils::starts_with( (*it)->regexp, "!^.*$!" ) ) {
                cerr << "NAPTR entry with unsupported regexp: " << (*it)->regexp << endl;
                return EXIT_FAILURE;
            }

            if( false == simgear::strutils::ends_with( (*it)->regexp, "!" ) ) {
                cerr << "NAPTR entry with unsupported regexp: " << (*it)->regexp << endl;
                return EXIT_FAILURE;
            }

        }
    }

    cout << "test non-existing NAPTR" << endl;
    {
        DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest("jurkxkqdiufqzpfvzqok.prozhqrlcaavbxifkkhf");
        DNS::Request_ptr r(naptrRequest);
        cl.makeRequest(r);
        while( !r->isComplete() && !r->isTimeout()) {
            SGTimeStamp::sleepForMSec(200);
            cl.update(0);
        }

        if( r->isTimeout() ) {
            cerr << "timeout testing non-existing record." << endl;
            return EXIT_FAILURE;
        }
        COMPARE(naptrRequest->entries.size(), 0 );
    }

    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
}
