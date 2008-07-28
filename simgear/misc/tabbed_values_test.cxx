////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear/compiler.h>

#include <iostream>
#include "tabbed_values.hxx"

using std::cout;
using std::cerr;
using std::endl;


int main (int ac, char ** av)
{
	const char* string1 = "Hello\tWorld\t34\tZ\t\tThere Is No Spoon";
	
	SGTabbedValues tv(string1);
	
	if (tv[0] != "Hello") {
		cerr << "failed to read string at index 0" << endl;
		return 1;
	}
	
	if (tv[1] != "World") {
		cerr << "failed to read string at index 1" << endl;
		return 1;
	}
	
	if (tv[2] != "34") {
		cerr << "failed to read string at index 2" << endl;
		return 1;
	}
	
	double dval = tv.getDoubleAt(2);
	if (dval != 34.0) {
		cerr << "failed to read double at index 2" << endl;
		return 2;
	}
	
	char cval = tv.getCharAt(3);
	if (cval != 'Z') {
		cerr << "failed to read char at index 3" << endl;
		return 1;
	}
	
	cval = tv.getCharAt(0);
	if (cval != 'H') {
		cerr << "failed to read char at index 0" << endl;
		return 1;
	}
	
	if (tv.isValueAt(4)) {
		cerr << "didn't identify skipped value correctly" << endl;
		return 3;
	}
	
	if (!tv.isValueAt(3)) {
		cerr << "didn't identify present value correctly" << endl;
		return 3;
	}
	
	if (tv[5] != "There Is No Spoon") {
		cerr << "failed to read string at index 5 (got [" << tv[5] << "]" << endl;
		return 1;
	}
	
	cout << "all tests passed successfully!" << endl;
	return 0;
}
