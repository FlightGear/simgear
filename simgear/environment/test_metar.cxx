
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/test_macros.hxx>

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


const double TEST_EPSILON = 1e-9;

void test_basic()
{
    SGMetar m1("2011/10/20 11:25 EHAM 201125Z 27012KT 240V300 9999 VCSH FEW025CB SCT048 10/05 Q1025 TEMPO VRB03KT");
    SG_CHECK_EQUAL(m1.getYear(), 2011);
    SG_CHECK_EQUAL(m1.getMonth(), 10);
    SG_CHECK_EQUAL(m1.getDay(), 20);
    SG_CHECK_EQUAL(m1.getHour(), 11);
    SG_CHECK_EQUAL(m1.getMinute(), 25);
    SG_CHECK_EQUAL(m1.getReportType(), -1); // should default to NIL?

    SG_CHECK_EQUAL(m1.getWindDir(), 270);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), 12, TEST_EPSILON);

    SG_CHECK_EQUAL(m1.getWeather().size(), 1);
    SG_CHECK_EQUAL(m1.getClouds().size(), 2);

    SG_CHECK_EQUAL_EP2(m1.getTemperature_C(), 10, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getDewpoint_C(), 5, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1025, TEST_EPSILON);
}

void test_sensor_failure_weather()
{
    SGMetar m1("2011/10/20 11:25 EHAM 201125Z 27012KT 240V300 9999 // FEW025CB SCT048 10/05 Q1025");
    SG_CHECK_EQUAL(m1.getWindDir(), 270);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), 12, TEST_EPSILON);

    SG_CHECK_EQUAL(m1.getWeather().size(), 0);
    SG_CHECK_EQUAL(m1.getClouds().size(), 2);

    SG_CHECK_EQUAL_EP2(m1.getTemperature_C(), 10, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getDewpoint_C(), 5, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1025, TEST_EPSILON);
}

void test_sensor_failure_cloud()
{
    SGMetar m1("2011/10/20 11:25 EHAM 201125Z 27012KT 240V300 9999 FEW025CB/// SCT048/// 10/05 Q1025");
    SG_CHECK_EQUAL(m1.getWindDir(), 270);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), 12, TEST_EPSILON);

    SG_CHECK_EQUAL(m1.getWeather().size(), 0);
    SG_CHECK_EQUAL(m1.getClouds().size(), 2);

    SG_CHECK_EQUAL_EP2(m1.getTemperature_C(), 10, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getDewpoint_C(), 5, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1025, TEST_EPSILON);
}

void test_sensor_failure_wind()
{
    SGMetar m1("2020/10/23 16:55 LIVD 231655Z /////KT 9999 OVC025 10/08 Q1020 RMK OVC VIS MIN 9999 BLU");
    SG_CHECK_EQUAL(m1.getWindDir(), -1);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), -1, TEST_EPSILON);

    SGMetar m2("2020/10/21 16:55 LIVD 211655Z /////KT CAVOK 07/03 Q1023 RMK SKC VIS MIN 9999 BLU");
    SG_CHECK_EQUAL(m2.getWindDir(), -1);
    SG_CHECK_EQUAL_EP2(m2.getWindSpeed_kt(), -1, TEST_EPSILON);

    SGMetar m3("2020/11/17 16:00 CYAZ 171600Z 14040G//KT 10SM -RA OVC012 12/11 A2895 RMK NS8 VIA CYXY SLP806 DENSITY ALT 900FT");
    SG_CHECK_EQUAL(m3.getWindDir(), 140);
    SG_CHECK_EQUAL_EP2(m3.getWindSpeed_kt(), 40, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m3.getGustSpeed_kt(), SGMetarNaN, TEST_EPSILON);
}

void test_wind_unit_not_specified()
{
    SGMetar m1("2020/10/23 11:58 KLSV 231158Z 05010G14 10SM CLR 16/M04 A2992 RMK SLPNO WND DATA ESTMD ALSTG/SLP ESTMD 10320 20124 5//// $");
    SG_CHECK_EQUAL(m1.getWindDir(), 50);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), 10.0, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m1.getGustSpeed_kt(), 14.0, TEST_EPSILON);
}

void test_clouds_without_height()
{
    // apparently EGPF has a faulty automated cloud sensor? :)
    SGMetar m1("EGPF 111420Z AUTO 24013KT 9000 -RA SCT019/// BKN023/// BKN030/// //////TCU 13/11 Q1011 RERA");
}

void test_GLRB_failure()
{
    // Haze entered backwards, genuinely invalid
#if 0
    SGMetar m1("2022/04/04 11:00 GLRB 041100Z 29012KT 8000 HZVC NSC 29/21 Q1012 NOSIG");
#endif
}

void test_LOWK_failure()
{
    // surface winds specification won't parse, looks invalid 
#if 0
    SGMetar m3("LOWK 051526Z AUTO 217G33KT 10SM VCTS FEW017 BKN023 OVC041 23/21 A2970 "
    "RMK AO2 PK WND 21037/1458 LTG DSNT ALQDS RAE09 P0000 T02330206");
#endif
}

int main(int argc, char* argv[])
{
    try {
        test_basic();
        test_sensor_failure_weather();
        test_sensor_failure_cloud();
        test_sensor_failure_wind();
        test_wind_unit_not_specified();
        test_clouds_without_height();
        test_GLRB_failure();
        test_LOWK_failure();
    } catch (sg_exception& e) {
        cerr << "got exception:" << e.getMessage() << endl;
        return -1;
    }
    
    return 0;
}
