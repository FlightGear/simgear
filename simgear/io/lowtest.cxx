#include <simgear/compiler.h>

#include STL_IOSTREAM
#include "lowlevel.hxx"

static const int sgEndianTest = 1;
#define sgIsLittleEndian (*((char *) &sgEndianTest ) != 0)
#define sgIsBigEndian    (*((char *) &sgEndianTest ) == 0)


int main() {
    cout << "This machine is ";
    if ( sgIsLittleEndian ) {
        cout << "little ";
    } else {
        cout << "big ";
    }
    cout << "endian" << endl;

    short s = 1111;
    cout << "short s = " << s << endl;
    sgEndianSwap((unsigned short *)&s);
    cout << "short s = " << s << endl;
    sgEndianSwap((unsigned short *)&s);
    cout << "short s = " << s << endl;

    int i = 1111111;
    cout << "int i = " << i << endl;
    sgEndianSwap((unsigned int *)&i);
    cout << "int i = " << i << endl;
    sgEndianSwap((unsigned int *)&i);
    cout << "int i = " << i << endl;

    double x = 1111111111;
    cout << "double x = " << x << endl;
    sgEndianSwap((unsigned long long *)&x);
    cout << "double x = " << x << endl;
    sgEndianSwap((unsigned long long *)&x);
    cout << "double x = " << x << endl;
}
