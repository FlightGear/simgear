#include <unistd.h>

#include "sg_socket.hxx"

int main() {

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
