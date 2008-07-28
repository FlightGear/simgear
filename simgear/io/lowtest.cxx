#include <simgear/compiler.h>

#include <iostream>
#include "lowlevel.hxx"


static const int sgEndianTest = 1;
#define sgIsLittleEndian (*((char *) &sgEndianTest ) != 0)
#define sgIsBigEndian    (*((char *) &sgEndianTest ) == 0)

using std::cout;
using std::endl;


int main() {
    cout << "This machine is ";
    if ( sgIsLittleEndian ) {
        cout << "little ";
    } else {
        cout << "big ";
    }
    cout << "endian" << endl;

    cout << "sizeof(short) = " << sizeof(short) << endl;

    short s = 1111;
    cout << "short s = " << s << endl;
    sgEndianSwap((uint16_t *)&s);
    cout << "short s = " << s << endl;
    sgEndianSwap((uint16_t *)&s);
    cout << "short s = " << s << endl;

    int i = 1111111;
    cout << "int i = " << i << endl;
    sgEndianSwap((uint32_t *)&i);
    cout << "int i = " << i << endl;
    sgEndianSwap((uint32_t *)&i);
    cout << "int i = " << i << endl;

    double x = 1111111111;
    cout << "double x = " << x << endl;
    sgEndianSwap((uint64_t *)&x);
    cout << "double x = " << x << endl;
    sgEndianSwap((uint64_t *)&x);
    cout << "double x = " << x << endl;
}
