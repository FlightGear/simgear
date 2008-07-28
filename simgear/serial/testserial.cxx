#include <simgear/compiler.h>

#include <string>
#include <iostream>

#include <simgear/debug/logstream.hxx>

#include "serial.hxx"

using std::cout;
using std::endl;

int main () {
    SGSerialPort port;
    string value;
    bool result;

    sglog().setLogLevels( SG_ALL, SG_INFO );

    cout << "start of main" << endl;

    result = port.open_port("/dev/ttyS1");
    cout << "opened port, result = " << result << endl;

    result = port.set_baud(4800);
    cout << "set baud, result = " << result << endl;

    port.write_port("ATDT 626-9800\n");

    while ( true ) {
	value = port.read_port();
	if ( value.length() ) {
	    cout << "-> " << value << endl;
	}
    }

    return 0;
}
