#include <simgear/compiler.h>

#include <unistd.h>
#include STL_IOSTREAM

#include "sg_socket.hxx"
#include "lowlevel.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif

static const int sgEndianTest = 1;
#define sgIsLittleEndian (*((char *) &sgEndianTest ) != 0)
#define sgIsBigEndian    (*((char *) &sgEndianTest ) == 0)

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
#ifdef __MINGW32__
	Sleep(100);
#else
	sleep(1);
#endif
    }
}
