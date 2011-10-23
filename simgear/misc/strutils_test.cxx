////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear/compiler.h>

#include <iostream>
#include "strutils.hxx"

using std::cout;
using std::cerr;
using std::endl;

using namespace simgear::strutils;

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        cerr << "failed:" << #a << endl; \
        exit(1); \
    }
    
int main (int ac, char ** av)
{
    std::string a("abcd");
    COMPARE(strip(a), a);
    COMPARE(strip(" a "), "a");
    COMPARE(lstrip(" a  "), "a  ");
    COMPARE(rstrip("\ta "), "\ta");
    // check internal spacing is preserved
    COMPARE(strip("\t \na \t b\r \n "), "a \t b");
    
    
    VERIFY(starts_with("banana", "ban"));
    VERIFY(!starts_with("abanana", "ban"));
    VERIFY(starts_with("banana", "banana")); // pass - string starts with itself
    VERIFY(!starts_with("ban", "banana")); // fail - original string is prefix of 
    
    VERIFY(ends_with("banana", "ana"));
    VERIFY(ends_with("foo.text", ".text"));
    VERIFY(!ends_with("foo.text", ".html"));
    
    COMPARE(simplify("\ta\t b  \nc\n\r \r\n"), "a b c");
    COMPARE(simplify("The quick  - brown dog!"), "The quick - brown dog!");
    COMPARE(simplify("\r\n  \r\n   \t  \r"), "");
    
    COMPARE(to_int("999"), 999);
    COMPARE(to_int("0000000"), 0);
    COMPARE(to_int("-10000"), -10000);
    
    VERIFY(compare_versions("1.0.12", "1.1") < 0);
    VERIFY(compare_versions("1.1", "1.0.12") > 0);
    VERIFY(compare_versions("10.6.7", "10.6.7") == 0);
    VERIFY(compare_versions("2.0", "2.0.99") < 0);
    VERIFY(compare_versions("99", "99") == 0);
    VERIFY(compare_versions("99", "98") > 0);
    
// since we compare numerically, leasing zeros shouldn't matter
    VERIFY(compare_versions("0.06.7", "0.6.07") == 0);
    
    string_list la = split("zero one two three four five");
    COMPARE(la[2], "two");
    COMPARE(la[5], "five");
    COMPARE(la.size(), 6);
    
    string_list lb = split("alpha:beta:gamma:delta", ":", 2);
    COMPARE(lb.size(), 3);
    COMPARE(lb[0], "alpha");
    COMPARE(lb[1], "beta");
    COMPARE(lb[2], "gamma:delta");
    
    std::string j = join(la, "&");
    COMPARE(j, "zero&one&two&three&four&five");
    
    cout << "all tests passed successfully!" << endl;
    return 0;
}
