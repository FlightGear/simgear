#include <cstdlib>

#include <iostream>
#include <map>
#include <sstream>
#include <errno.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/simgear_config.h>

#include "DNSClient.hxx"

#include "test_DNS.hxx"

#include <simgear/debug/logstream.hxx>

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


int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_DEBUG );

    DNS::Client cl;

    // test existing NAPTR
    // fgtest.t3r.de.		600	IN	NAPTR	999 99 "U" "test" "!^.*$!http://dnstest.flightgear.org/!" .
    {
    	DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest("fgtest.t3r.de");
    	DNS::Request_ptr r(naptrRequest);
    	cl.makeRequest(r);
    	while( !r->complete() )
    		cl.update(0);

    	COMPARE(naptrRequest->entries.size(), 1 );
    	COMPARE(naptrRequest->entries[0]->order, 999 );
    	COMPARE(naptrRequest->entries[0]->preference, 99 );
    	COMPARE(naptrRequest->entries[0]->service, "test" );
    	COMPARE(naptrRequest->entries[0]->regexp, "!^.*$!http://dnstest.flightgear.org/!" );
    	COMPARE(naptrRequest->entries[0]->replacement, "" );
    }

    // test non-existing NAPTR
    {
    	DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest("jurkxkqdiufqzpfvzqok.prozhqrlcaavbxifkkhf");
    	DNS::Request_ptr r(naptrRequest);
    	cl.makeRequest(r);
    	while( !r->complete() )
    		cl.update(0);

    	COMPARE(naptrRequest->entries.size(), 0 );
    }

    cout << "all tests passed ok" << endl;
    return EXIT_SUCCESS;
}
