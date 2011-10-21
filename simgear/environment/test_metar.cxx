
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <cstdlib>
#include <cstdio>

#ifdef _MSC_VER
#   define  random  rand
#endif

#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>

#include "metar.hxx"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        cerr << "\tgot:" << a << endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        cerr << "failed:" << #a << endl; \
        exit(1); \
    }
    

void test_basic()
{
    SGMetar m1("2011/10/20 11:25 EHAM 201125Z 27012KT 240V300 9999 VCSH FEW025CB SCT048 10/05 Q1025 TEMPO VRB03KT");
    COMPARE(m1.getYear(), 2011);
    COMPARE(m1.getMonth(), 10);
    COMPARE(m1.getDay(), 20);
    COMPARE(m1.getHour(), 11);
    COMPARE(m1.getMinute(), 25);
    COMPARE(m1.getReportType(), -1); // should default to NIL?
    
    COMPARE(m1.getWindDir(), 270);
    COMPARE(m1.getWindSpeed_kt(), 12);
    
    COMPARE(m1.getTemperature_C(), 10);
    COMPARE(m1.getDewpoint_C(), 5);
    COMPARE(m1.getPressure_hPa(), 1025);
}

int main(int argc, char* argv[])
{
    try {
    test_basic();
    } catch (sg_exception& e) {
        cerr << "got exception:" << e.getMessage() << endl;
        return -1;
    }
    
    return 0;
}
