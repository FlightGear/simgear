#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#endif
#include <iostream>

#include "sg_socket.hxx"
#include "lowlevel.hxx"

static const int sgEndianTest = 1;
#define sgIsLittleEndian (*((char *) &sgEndianTest ) != 0)
#define sgIsBigEndian    (*((char *) &sgEndianTest ) == 0)

using std::cout;
using std::endl;


int main() {

    if ( sgIsLittleEndian ) {
       cout << "this machine is little endian\n";
    }

    if ( sgIsBigEndian ) {
       cout << "this machine is big endian\n";
    }

    cout << "short = " << sizeof(unsigned short) << endl;
    cout << "int = " << sizeof(unsigned int) << endl;
    cout << "long long = " << sizeof(long long) << endl;

    SGSocket s( "", "5500", "tcp" );

    if ( !s.open( SG_IO_BI ) ) {
	cout << "error can't open socket\n";
    }

    char buf[256];

    while ( true ) {
	if ( s.readline( buf, 256 ) > 0 ) {
	    cout << "result = " << buf;
	}
	sleep(1);
    }
}
