// SPDX-License-Identifier: LGPL-2.1-or-later

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

    // negative temperature and negative dew point
    // wind provides direction, but not velocity
    SGMetar m2("XXXX 012345Z 150KT 9999 -SN OVC060CB SCT050TCU M20/M30 Q1005");
    SG_CHECK_EQUAL_EP2(m2.getTemperature_C(), -20.0, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m2.getDewpoint_C(), -30.0, TEST_EPSILON);

    // wind contains a space between direction-velocity and gusts
    // temperature is absent
    SGMetar m3("2023/05/08 14:46 KSCH 081446Z 28012 G15KT 15SM CLR A2981 RMK TEMP AND DP MISSING; FIRST");
    SG_CHECK_EQUAL(m3.getWindDir(), 280);
    SG_CHECK_EQUAL_EP2(m3.getWindSpeed_kt(), 12.0, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m3.getGustSpeed_kt(), 15.0, TEST_EPSILON);
}

void test_drizzle()
{
    SGMetar m1("2011/10/20 11:25 EHAM 201125Z 27012KT 9999 DZ FEW025CB 10/05 Q1025");
    SG_CHECK_EQUAL(m1.getRain(), 1);
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
    SG_CHECK_EQUAL(m1.getWindDir(), 0);
    SG_CHECK_EQUAL_EP2(m1.getWindSpeed_kt(), 0.0, TEST_EPSILON);

    SGMetar m2("2020/10/21 16:55 LIVD 211655Z /////KT CAVOK 07/03 Q1023 RMK SKC VIS MIN 9999 BLU");
    SG_CHECK_EQUAL(m2.getWindDir(), 0);
    SG_CHECK_EQUAL_EP2(m2.getWindSpeed_kt(), 0.0, TEST_EPSILON);

    SGMetar m3("2020/11/17 16:00 CYAZ 171600Z 14040G//KT 10SM -RA OVC012 12/11 A2895 RMK NS8 VIA CYXY SLP806 DENSITY ALT 900FT");
    SG_CHECK_EQUAL(m3.getWindDir(), 140);
    SG_CHECK_EQUAL_EP2(m3.getWindSpeed_kt(), 40, TEST_EPSILON);
    SG_CHECK_EQUAL_EP2(m3.getGustSpeed_kt(), SGMetarNaN, TEST_EPSILON);
}

void test_sensor_failure_pressure()
{
    SGMetar m1("2023/05/07 09:00 FOOK 070900Z /////KT 9000 SCT010TCU BKN018 26/22 Q////");
    SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON);
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

void test_LFBT_failure()
{
    // https://sourceforge.net/p/flightgear/codetickets/2765/
    SGMetar m1("2022/09/15 17:00 LFBT 151700Z AUTO 21008KT 9999 -RA FEW060/// SCT076/// OVC088/// ///CB 20/16 Q1014 TEMPO 27015G25KT 4000 -TSRA");
}

void bulk_from_sentry()
{
    { SGMetar m1("2023/03/01 06:00 FNDU 010600Z 00000KT 9999 BKN023 ///// Q////"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("XXXX 012345Z 150KT 9999 -SN OVC060CB SCT050TCU M20/M30 Q1005"); }
    { SGMetar m1("2023/05/08 14:00 FTTC 081400Z 00000KT 9999 SCT040 41/// Q1009"); }
    { SGMetar m1("2023/05/08 14:00 DAAT 081400Z 30013KT 250V320 CAVOK 35/// Q1020"); }
    { SGMetar m1("2023/05/08 14:00 DAUA 081400Z 04016KT CAVOK 38/// Q1014"); }
    { SGMetar m1("2023/05/08 14:00 DAOF 081400Z 11008KT CAVOK 38/// Q1014"); }
    { SGMetar m1("2023/05/08 05:00 MYGF 080500Z AUTO 08008KT //// R06///// // NCD 19/14 A3010"); }
    { SGMetar m1("2023/05/07 14:00 FTTC 071400Z 18004KT 9999 FEW040 43/// Q1008"); }
    { SGMetar m1("2023/05/07 21:00 DAUA 072100Z 27006KT 9000 NSC 34/// Q1010"); }
    { SGMetar m1("2023/05/06 16:30 LFBP 061630Z AUTO 27014KT 9999 FEW027/// ///TCU 21/14 Q1015 TEMPO 29018G30KT 3000 TSRA BKN014 SCT040CB BKN060"); }
    { SGMetar m1("2023/05/01 12:50 EKVJ 011250Z 23016KT CAVOK 13/// Q1013"); }
    { SGMetar m1("2023/04/26 20:00 HESC 262000Z ?30005KT CAVOK 25/04 Q1009 NOSIG"); }
    { SGMetar m1("XXXX 091100Z  25010KT  100SM  15/11 1013"); }
    { SGMetar m1("2023/04/24 18:00 SPJJ 241800Z 35009KT 9999 SCT040 19/// Q1031 RMK PP000"); }
    // { SGMetar m1("2023/04/12 18:20 LSMM 121820Z AUTO 31012KT 260V340 9999NDV RA FEW018 BKN034 OVC0 43 10/07 Q1006 RMK"); }   valid error, note the space in the OVC layer
    { SGMetar m1("2023/04/12 16:30 LFMY 121630Z AUTO 26006KT 9999 // ///TCU ///// Q1007"); }
    { SGMetar m1("2023/04/12 14:00 SBPB 121400Z 14009KT 9999 BKN020 ///// Q1011"); }
    { SGMetar m1("2023/04/01 15:00 FMNN 011500Z 02004KT 9999 BKN020CB 28/// Q1010"); }
    { SGMetar m1("2023/03/25 20:55 TNCE 252055Z AUTO 06012KT 040V100 //// // ///////// 27/22 Q1016 RE//"); }
    { SGMetar m1("2023/03/23 01:20 EDMO 230120Z AUTO 21004KT 180V250 //// R22///// // OVC210/// 07/03 Q1012"); }
    { SGMetar m1("2023/03/22 01:20 EDMO 220120Z AUTO 14002KT //// R22///// // FEW079/// BKN095/// OVC120/// 08/04 Q1015"); }
    { SGMetar m1("2023/03/11 11:51 KSMO 111151Z AUTO 00000KT 4SM HZ OVC003 A3002 RMK AO2 RAE20B35E42 SLPNO P0000 60004 70084 57003 $"); }
    { SGMetar m1("2023/03/10 21:25 EHDV 102125Z AUTO 35024KT 9999 FEW041/// 04/// Q1005"); }
    { SGMetar m1("2023/03/10 08:53 PHJR 100853Z A2999 RMK AO2 SLP163 53010 $"); }
    { SGMetar m1("XXXX 012345Z 3607KT 12SM SCT048 FEW300 20/08 Q1022 NOSIG"); }
    { SGMetar m1("XXXX 012345Z 00000KT SKC Q1013"); }
    { SGMetar m1("2023/02/27 16:00 SBCX 271600Z 18004KT CAVOK 25/// Q1023"); }
    { SGMetar m1("2023/02/25 22:45 KMHR 252245Z E15009KT 10SM BKN075 OVC085 08/04 A2990"); }
    { SGMetar m1("2023/02/25 22:55 KMMT 252255Z AUTO 00000KT 10SM M 13/13 A3013 RMK AO2 SLP206 T01320130 $"); }
    { SGMetar m1("2023/02/23 00:45 KMWC 230045Z 06015G23KT 1 1/4SM SNPL OVC005 M01/M03 A2970 "); }
    { SGMetar m1("2023/02/20 12:30 LFMY 201230Z AUTO VRB03KT 9999 FEW005 18/// Q1026 NOSIG"); }
    { SGMetar m1("2023/02/17 00:05 PHNG 170005Z 07011KT 10SM SCT022 BKN029 A2987 RMK AO2"); }
    { SGMetar m1("2023/02/08 16:48 BIIS 081648Z 18004KT 9999 SCT025 M06/// Q0980"); }
    { SGMetar m1("2023/05/07 09:00 FOOK 070900Z /////KT 9000 SCT010TCU BKN018 26/22 Q////"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("2023/05/08 15:00 FOGO 081500Z /////KT 9999 SCT016 BKN040 29/22 Q////"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("2022/12/14 14:00 HSSJ 141400Z 09006KT 9999 SCT040 32/15 Q 1009"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1009.0, TEST_EPSILON); }
    { SGMetar m1("2023/04/19 15:30 BIKF 191530Z 14017G28KT 9999 BKN005 OVC011 10/09 Q10"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1000.0, TEST_EPSILON); }
    { SGMetar m1("EGKK 251150Z 27018KT 9999 BKN037 12/05 Q1006,"); }
    { SGMetar m1("2023/03/21 17:00 LOAV 211700Z 33005KT 9999 SCT050 15/05"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("2023/02/26 19:30 EPKT 261930Z 28003KT 250V340 9999 -SHSN SCT021CB BKN030 M02/M03"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("2023/02/09 23:00 RPMD 092300Z 35003KT 320V030 9999 FEW015 BKN090 BKN260 24/24"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 1013.0, TEST_EPSILON); }
    { SGMetar m1("XXXX 012345Z 15003KT 19SM FEW072 FEW350 25/07 Q885 NOSIG"); SG_CHECK_EQUAL_EP2(m1.getPressure_hPa(), 885.0, TEST_EPSILON); }
    { SGMetar m1("METAR RPMZ 092300Z 00000KT 9999 FEW017 BKN090 24/23 Q1010 RMK A2983"); }
    // { SGMetar m1("2023/05/10 18:50 LSZC 101850Z AUTO 03005KT 350V050 9999 FEW038 BKN055 BKN0"); }        valid error, BRN0
    { SGMetar m1("2023/05/10 17:51 KHSE 101751Z AUTO A3014 RMK AO2 SLP208 6//// 10283 20172 51003 PWINO FZRANO PNO $"); }
    { SGMetar m1("2023/05/11 03:43 MMTM 110343Z E06010KT 4SM SCT010TCU BKN070 27/25 A2979 RMK 8/270 VC TS LTGIC OCNLS W NWOPS OCNLS HZ"); }
    { SGMetar m1("2023/05/11 03:00 FOOL 110300Z 060/2KT 9999 FEW012 25/25 Q1009 NOSIG"); }
    { SGMetar m1("2023/05/11 00:00 CYTR 110000Z CCA 25013KT 15SM BKN220 17/02 A3006 RMK CI7 DENSITY ALT 511FT SLP181"); }
    {
        SGMetar m1("2023/05/10 23:00 OMAD 102300Z 09001KT CAVOK 2/09 Q1007 NOSIG");
        SG_CHECK_EQUAL_EP2(m1.getTemperature_C(), 2.0, TEST_EPSILON);
        SG_CHECK_EQUAL_EP2(m1.getDewpoint_C(), 9.0, TEST_EPSILON);
    }
    // { SGMetar m1("2023/05/10 20:15 KSTF 102015Z AUTO RMK AO2 PNO PWINO TSNO"); }     valid error, nothing of value
    { SGMetar m1("2023/05/10 17:51 KHSE 101751Z AUTO A3014 RMK AO2 SLP208 6//// 10283 20172 51003 PWINO FZRANO PNO $"); }
    { SGMetar m1("2023/05/07 12:30 SAAV 071230 05004KT 9000 11/11 Q1015 RMK PP///"); }
    { SGMetar m1("2023/05/11 19:53 KETH 111953Z AUTO 14014KT 10SM SCT049 120 24/15 A2995 RMK AO2"); }
}

void bulk_from_tickets()
{
    { SGMetar m1("EGPF 111420Z AUTO 24013KT 9000 -RA SCT019/// BKN023/// BKN030TCU/// 13/11 Q1011 RERA"); }
    { SGMetar m1("EGPF 111420Z AUTO 24013KT 9000 -RA SCT019/// BKN023/// BKN030/// //////TCU 13/11 Q1011 RERA"); }
    { SGMetar m1("LYBE 131800Z 26008KT 3500 -RA BR OVC003 04/04 Q1019 TEMPO OVC002"); }
    { SGMetar m1("LFMO 031330Z AUTO VRB02KT 9000 -RA SCT015/// OVC024/// ///CB 13/13 Q1012 TEMPO VRB20G30KT 2000 TSRA BKN010 BKN025CB"); }
}

void bulk_stress()
{
    // { SGMetar m1("2023/05/11 19:50 KJAU 111950Z AUTO 11.96000012KT 10SM CLR 25/14 A3017 RMK A01"); }                                 valid error, 11.96000012KT
    // { SGMetar m1("2023/05/11 20:00 MGQZ 112000Z 14010KT 8000 TS BKN018 FEW025CB 21/13 QFE 772.3 HZ CB/TS N"); }                      valid error, QFE
    // { SGMetar m1("2023/05/11 20:00 MGCB 112000Z 07004KT 9999 SCT018 SCT200 31/18 QFE 868.7"); }                                      valid error, QFE
    // { SGMetar m1("2023/05/11 20:00 MGZA 112000Z 30008KT 9999 FEW025 38/19 QFE 981.8 CB DIST WSW/NNW"); }                             valid error, QFE
    // { SGMetar m1("2023/05/11 20:00 MGES 112000Z 00000KT 7000 SCT022TCU FEW025CB 31/17 QFE 907.4 HZ FEW090 CB SE/NW TCU NE/E"); }     valid error, QFE
    // { SGMetar m1("2023/05/11 20:00 MGQZ 112000Z 14010KT 8000 TS BKN018 FEW025CB 21/13 QFE 772/3 HZ CB/TS N"); }                      valid error, QFE
    // { SGMetar m1("2023/05/11 20:00 MGZA 112000Z 30008KT 9999 FEW025 38/19 QFE 981/8 CB DIST WSW/NNW"); }                             valid error, QFE
    // { SGMetar m1("2023/05/11 19:55 LIMH 111955Z AUTO /////KT //// ///// Q////"); }                                                   nothing useful
    // { SGMetar m1("2023/05/11 20:00 SKMR 112000Z 00000KT 9999 FEW020 35/23 Q10006 RMK A2973"); }                                      valid error, Q10006
}

int main(int argc, char* argv[])
{
    try {
        test_basic();
        test_sensor_failure_weather();
        test_sensor_failure_cloud();
        test_sensor_failure_wind();
        test_sensor_failure_pressure();
        test_wind_unit_not_specified();
        test_drizzle();
        test_clouds_without_height();
        test_GLRB_failure();
        test_LOWK_failure();
        test_LFBT_failure();
        bulk_from_sentry();
        bulk_from_tickets();
        bulk_stress();
    } catch (sg_exception& e) {
        std::cerr << "Exception: " << e.getMessage() << std::endl;
        return -1;
    }
    
    return 0;
}
