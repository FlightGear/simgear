#include "Local.h"     /* standard header file */
#include "Metar.h"
#pragma page(1)
#pragma subtitle("subtitle - description                       ")
/********************************************************************/
/*                                                                  */
/*  Title:         prtDMETR                                         */
/*  Organization:  W/OSO242 - GRAPHICS AND DISPLAY SECTION          */
/*  Date:          15 Sep 1994                                      */
/*  Programmer:    CARL MCCALLA                                     */
/*  Language:      C/370                                            */
/*                                                                  */
/*  Abstract:  prtDMETR    prints, in order of the ASOS METAR       */
/*             format, all non-initialized members of the structure */
/*             addressed by the Decoded_METAR pointer.              */
/*                                                                  */
/*  External Functions Called:                                      */
/*                 None.                                            */
/*                                                                  */
/*  Input:         Mptr - ptr to a decoded_METAR structure.         */
/*                                                                  */
/*  Output:        NONE                                             */
/*                                                                  */
/*  Modification History:                                           */
/*                 None.                                            */
/*                                                                  */
/********************************************************************/
#pragma page(1)
void prtDMETR( Decoded_METAR *Mptr )
{
 
   /***************************/
   /* DECLARE LOCAL VARIABLES */
   /***************************/
 
   int i;
 
   /*************************/
   /* START BODY OF ROUTINE */
   /*************************/
 
   printf("\n\n\n/*******************************************/\n");
   printf("/*    THE DECODED METAR REPORT FOLLOWS     */\n");
   printf("/*******************************************/\n\n");
 
   if( Mptr->codeName[ 0 ] != '\0' )
      printf("REPORT CODE NAME    : %s\n",Mptr->codeName);
 
   if( Mptr->stnid[ 0 ] != '\0' )
      printf("STATION ID          : %s\n",Mptr->stnid);
 
   if( Mptr->ob_date != MAXINT )
      printf("OBSERVATION DAY     : %d\n",Mptr->ob_date);
 
   if( Mptr->ob_hour != MAXINT )
      printf("OBSERVATION HOUR    : %d\n",Mptr->ob_hour);
 
   if( Mptr->ob_minute != MAXINT )
      printf("OBSERVATION MINUTE  : %d\n",Mptr->ob_minute);
 
   if( Mptr->NIL_rpt )
      printf("NIL REPORT          : TRUE\n");
 
   if( Mptr->AUTO )
      printf("AUTO REPORT         : TRUE\n");
 
   if( Mptr->COR )
      printf("CORRECTED REPORT    : TRUE\n");
 
   if( Mptr->winData.windVRB )
      printf("WIND DIRECTION VRB  : TRUE\n");
 
   if( Mptr->winData.windDir != MAXINT )
      printf("WIND DIRECTION      : %d\n",Mptr->winData.windDir);
 
   if( Mptr->winData.windSpeed != MAXINT )
      printf("WIND SPEED          : %d\n",Mptr->winData.windSpeed);
 
   if( Mptr->winData.windGust != MAXINT )
      printf("WIND GUST           : %d\n",Mptr->winData.windGust);
 
   if( Mptr->winData.windUnits[ 0 ] != '\0' )
      printf("WIND UNITS          : %s\n",Mptr->winData.windUnits);
 
   if( Mptr->minWnDir != MAXINT )
      printf("MIN WIND DIRECTION  : %d\n",Mptr->minWnDir);
 
   if( Mptr->maxWnDir != MAXINT )
      printf("MAX WIND DIRECTION  : %d\n",Mptr->maxWnDir);
 
   if( Mptr->prevail_vsbyM != (float) MAXINT )
      printf("PREVAIL VSBY (M)    : %f\n",Mptr->prevail_vsbyM);
 
   if( Mptr->prevail_vsbyKM != (float) MAXINT )
      printf("PREVAIL VSBY (KM)   : %f\n",Mptr->prevail_vsbyKM);
 
   if( Mptr->prevail_vsbySM != (float) MAXINT )
      printf("PREVAIL VSBY (SM)   : %.3f\n",Mptr->prevail_vsbySM);
 
   if( Mptr->charPrevailVsby[0] != '\0' )
      printf("PREVAIL VSBY (CHAR) : %s\n",Mptr->charPrevailVsby);
 
   if( Mptr->vsby_Dir[ 0 ] != '\0' )
      printf("VISIBILITY DIRECTION: %s\n",Mptr->vsby_Dir);
 
   if( Mptr->RVRNO )
      printf("RVRNO               : TRUE\n");
 
   for ( i = 0; i < 12; i++ )
   {
      if( Mptr->RRVR[i].runway_designator[0] != '\0' )
         printf("RUNWAY DESIGNATOR   : %s\n",
                 Mptr->RRVR[i].runway_designator);
 
      if( Mptr->RRVR[i].visRange != MAXINT )
         printf("R_WAY VIS RANGE (FT): %d\n",
                 Mptr->RRVR[i].visRange);
 
      if( Mptr->RRVR[i].vrbl_visRange )
         printf("VRBL VISUAL RANGE   : TRUE\n");
 
      if( Mptr->RRVR[i].below_min_RVR )
         printf("BELOW MIN RVR       : TRUE\n");
 
      if( Mptr->RRVR[i].above_max_RVR )
         printf("ABOVE MAX RVR       : TRUE\n");
 
      if( Mptr->RRVR[i].Max_visRange != MAXINT )
         printf("MX R_WAY VISRNG (FT): %d\n",
                 Mptr->RRVR[i].Max_visRange);
 
      if( Mptr->RRVR[i].Min_visRange != MAXINT )
         printf("MN R_WAY VISRNG (FT): %d\n",
                 Mptr->RRVR[i].Min_visRange);
 
   }
 
 
   if( Mptr->DVR.visRange != MAXINT )
      printf("DISPATCH VIS RANGE  : %d\n",
              Mptr->DVR.visRange);
 
   if( Mptr->DVR.vrbl_visRange )
      printf("VRBL DISPATCH VISRNG: TRUE\n");
 
   if( Mptr->DVR.below_min_DVR )
      printf("BELOW MIN DVR       : TRUE\n");
 
   if( Mptr->DVR.above_max_DVR )
      printf("ABOVE MAX DVR       : TRUE\n");
 
   if( Mptr->DVR.Max_visRange != MAXINT )
      printf("MX DSPAT VISRNG (FT): %d\n",
              Mptr->DVR.Max_visRange);
 
   if( Mptr->DVR.Min_visRange != MAXINT )
      printf("MN DSPAT VISRNG (FT): %d\n",
              Mptr->DVR.Min_visRange);
 
 
   i = 0;
   while ( Mptr->WxObstruct[i][0] != '\0' && i < MAXWXSYMBOLS )
   {
      printf("WX/OBSTRUCT VISION  : %s\n",
         Mptr->WxObstruct[i] );
      i++;
   }
 
   if( Mptr->PartialObscurationAmt[0][0] != '\0' )
      printf("OBSCURATION AMOUNT  : %s\n",
            &(Mptr->PartialObscurationAmt[0][0]));
 
   if( Mptr->PartialObscurationPhenom[0][0] != '\0' )
      printf("OBSCURATION PHENOM  : %s\n",
            &(Mptr->PartialObscurationPhenom[0][0]));
 
 
   if( Mptr->PartialObscurationAmt[1][0] != '\0' )
      printf("OBSCURATION AMOUNT  : %s\n",
            &(Mptr->PartialObscurationAmt[1][0]));
 
   if( Mptr->PartialObscurationPhenom[1][0] != '\0' )
      printf("OBSCURATION PHENOM  : %s\n",
            &(Mptr->PartialObscurationPhenom[1][0]));
 
   i = 0;
   while ( Mptr->cldTypHgt[ i ].cloud_type[0] != '\0' &&
                     i < 6 )
   {
      if( Mptr->cldTypHgt[ i ].cloud_type[0] != '\0' )
         printf("CLOUD COVER         : %s\n",
            Mptr->cldTypHgt[ i ].cloud_type);
 
      if( Mptr->cldTypHgt[ i ].cloud_hgt_char[0] != '\0' )
         printf("CLOUD HGT (CHARAC.) : %s\n",
            Mptr->cldTypHgt[ i ].cloud_hgt_char);
 
      if( Mptr->cldTypHgt[ i ].cloud_hgt_meters != MAXINT)
         printf("CLOUD HGT (METERS)  : %d\n",
            Mptr->cldTypHgt[ i ].cloud_hgt_meters);
 
      if( Mptr->cldTypHgt[ i ].other_cld_phenom[0] != '\0' )
         printf("OTHER CLOUD PHENOM  : %s\n",
            Mptr->cldTypHgt[ i ].other_cld_phenom);
 
      i++;
 
   }
 
   if( Mptr->temp != MAXINT )
      printf("TEMP. (CELSIUS)     : %d\n", Mptr->temp);
 
   if( Mptr->dew_pt_temp != MAXINT )
      printf("D.P. TEMP. (CELSIUS): %d\n", Mptr->dew_pt_temp);
 
   if( Mptr->A_altstng )
      printf("ALTIMETER (INCHES)  : %.2f\n",
         Mptr->inches_altstng );
 
   if( Mptr->Q_altstng )
      printf("ALTIMETER (PASCALS) : %d\n",
         Mptr->hectoPasc_altstng );
 
   if( Mptr->TornadicType[0] != '\0' )
      printf("TORNADIC ACTVTY TYPE: %s\n",
         Mptr->TornadicType );
 
   if( Mptr->BTornadicHour != MAXINT )
      printf("TORN. ACTVTY BEGHOUR: %d\n",
         Mptr->BTornadicHour );
 
   if( Mptr->BTornadicMinute != MAXINT )
      printf("TORN. ACTVTY BEGMIN : %d\n",
         Mptr->BTornadicMinute );
 
   if( Mptr->ETornadicHour != MAXINT )
      printf("TORN. ACTVTY ENDHOUR: %d\n",
         Mptr->ETornadicHour );
 
   if( Mptr->ETornadicMinute != MAXINT )
      printf("TORN. ACTVTY ENDMIN : %d\n",
         Mptr->ETornadicMinute );
 
   if( Mptr->TornadicDistance != MAXINT )
      printf("TORN. DIST. FROM STN: %d\n",
         Mptr->TornadicDistance );
 
   if( Mptr->TornadicLOC[0] != '\0' )
      printf("TORNADIC LOCATION   : %s\n",
         Mptr->TornadicLOC );
 
   if( Mptr->TornadicDIR[0] != '\0' )
      printf("TORNAD. DIR FROM STN: %s\n",
         Mptr->TornadicDIR );
 
   if( Mptr->TornadicMovDir[0] != '\0' )
      printf("TORNADO DIR OF MOVM.: %s\n",
         Mptr->TornadicMovDir );
 
 
   if( Mptr->autoIndicator[0] != '\0' )
         printf("AUTO INDICATOR      : %s\n",
                          Mptr->autoIndicator);
 
   if( Mptr->PKWND_dir !=  MAXINT )
      printf("PEAK WIND DIRECTION : %d\n",Mptr->PKWND_dir);
   if( Mptr->PKWND_speed !=  MAXINT )
      printf("PEAK WIND SPEED     : %d\n",Mptr->PKWND_speed);
   if( Mptr->PKWND_hour !=  MAXINT )
      printf("PEAK WIND HOUR      : %d\n",Mptr->PKWND_hour);
   if( Mptr->PKWND_minute !=  MAXINT )
      printf("PEAK WIND MINUTE    : %d\n",Mptr->PKWND_minute);
 
   if( Mptr->WshfTime_hour != MAXINT )
      printf("HOUR OF WIND SHIFT  : %d\n",Mptr->WshfTime_hour);
   if( Mptr->WshfTime_minute != MAXINT )
      printf("MINUTE OF WIND SHIFT: %d\n",Mptr->WshfTime_minute);
   if( Mptr->Wshft_FROPA != FALSE )
      printf("FROPA ASSOC. W/WSHFT: TRUE\n");
 
   if( Mptr->TWR_VSBY != (float) MAXINT )
      printf("TOWER VISIBILITY    : %.2f\n",Mptr->TWR_VSBY);
   if( Mptr->SFC_VSBY != (float) MAXINT )
      printf("SURFACE VISIBILITY  : %.2f\n",Mptr->SFC_VSBY);
 
   if( Mptr->minVsby != (float) MAXINT )
      printf("MIN VRBL_VIS (SM)   : %.4f\n",Mptr->minVsby);
   if( Mptr->maxVsby != (float) MAXINT )
      printf("MAX VRBL_VIS (SM)   : %.4f\n",Mptr->maxVsby);
 
   if( Mptr->VSBY_2ndSite != (float) MAXINT )
      printf("VSBY_2ndSite (SM)   : %.4f\n",Mptr->VSBY_2ndSite);
   if( Mptr->VSBY_2ndSite_LOC[0] != '\0' )
      printf("VSBY_2ndSite LOC.   : %s\n",
                   Mptr->VSBY_2ndSite_LOC);
 
 
   if( Mptr->OCNL_LTG )
      printf("OCCASSIONAL LTG     : TRUE\n");
 
   if( Mptr->FRQ_LTG )
      printf("FREQUENT LIGHTNING  : TRUE\n");
 
   if( Mptr->CNS_LTG )
      printf("CONTINUOUS LTG      : TRUE\n");
 
   if( Mptr->CG_LTG )
      printf("CLOUD-GROUND LTG    : TRUE\n");
 
   if( Mptr->IC_LTG )
      printf("IN-CLOUD LIGHTNING  : TRUE\n");
 
   if( Mptr->CC_LTG )
      printf("CLD-CLD LIGHTNING   : TRUE\n");
 
   if( Mptr->CA_LTG )
      printf("CLOUD-AIR LIGHTNING : TRUE\n");
 
   if( Mptr->AP_LTG )
      printf("LIGHTNING AT AIRPORT: TRUE\n");
 
   if( Mptr->OVHD_LTG )
      printf("LIGHTNING OVERHEAD  : TRUE\n");
 
   if( Mptr->DSNT_LTG )
      printf("DISTANT LIGHTNING   : TRUE\n");
 
   if( Mptr->LightningVCTS )
      printf("L'NING W/I 5-10(ALP): TRUE\n");
 
   if( Mptr->LightningTS )
      printf("L'NING W/I 5 (ALP)  : TRUE\n");
 
   if( Mptr->VcyStn_LTG )
      printf("VCY STN LIGHTNING   : TRUE\n");
 
   if( Mptr->LTG_DIR[0] != '\0' )
      printf("DIREC. OF LIGHTNING : %s\n", Mptr->LTG_DIR);
 
 
 
   i = 0;
   while( i < 3 && Mptr->ReWx[ i ].Recent_weather[0] != '\0' )
   {
      printf("RECENT WEATHER      : %s",
                  Mptr->ReWx[i].Recent_weather);
 
      if( Mptr->ReWx[i].Bhh != MAXINT )
         printf(" BEG_hh = %d",Mptr->ReWx[i].Bhh);
      if( Mptr->ReWx[i].Bmm != MAXINT )
         printf(" BEG_mm = %d",Mptr->ReWx[i].Bmm);
 
      if( Mptr->ReWx[i].Ehh != MAXINT )
         printf(" END_hh = %d",Mptr->ReWx[i].Ehh);
      if( Mptr->ReWx[i].Emm != MAXINT )
         printf(" END_mm = %d",Mptr->ReWx[i].Emm);
 
      printf("\n");
 
      i++;
   }
 
   if( Mptr->minCeiling != MAXINT )
      printf("MIN VRBL_CIG (FT)   : %d\n",Mptr->minCeiling);
   if( Mptr->maxCeiling != MAXINT )
      printf("MAX VRBL_CIG (FT))  : %d\n",Mptr->maxCeiling);
 
   if( Mptr->CIG_2ndSite_Meters != MAXINT )
      printf("CIG2ndSite (FT)     : %d\n",Mptr->CIG_2ndSite_Meters);
   if( Mptr->CIG_2ndSite_LOC[0] != '\0' )
      printf("CIG @ 2nd Site LOC. : %s\n",Mptr->CIG_2ndSite_LOC);
 
   if( Mptr->PRESFR )
      printf("PRESFR              : TRUE\n");
   if( Mptr->PRESRR )
      printf("PRESRR              : TRUE\n");
 
   if( Mptr->SLPNO )
      printf("SLPNO               : TRUE\n");
 
   if( Mptr->SLP != (float) MAXINT )
      printf("SLP (hPa)           : %.1f\n", Mptr->SLP);
 
   if( Mptr->SectorVsby != (float) MAXINT )
      printf("SECTOR VSBY (MILES) : %.2f\n", Mptr->SectorVsby );
 
   if( Mptr->SectorVsby_Dir[ 0 ] != '\0' )
      printf("SECTOR VSBY OCTANT  : %s\n", Mptr->SectorVsby_Dir );
 
   if( Mptr->TS_LOC[ 0 ] != '\0' )
      printf("THUNDERSTORM LOCAT. : %s\n", Mptr->TS_LOC );
 
   if( Mptr->TS_MOVMNT[ 0 ] != '\0' )
      printf("THUNDERSTORM MOVMNT.: %s\n", Mptr->TS_MOVMNT);
 
   if( Mptr->GR )
      printf("GR (HAILSTONES)     : TRUE\n");
 
   if( Mptr->GR_Size != (float) MAXINT )
      printf("HLSTO SIZE (INCHES) : %.3f\n",Mptr->GR_Size);
 
   if( Mptr->VIRGA )
      printf("VIRGA               : TRUE\n");
 
   if( Mptr->VIRGA_DIR[0] != '\0' )
      printf("DIR OF VIRGA FRM STN: %s\n", Mptr->VIRGA_DIR);
 
   for( i = 0; i < 6; i++ ) {
      if( Mptr->SfcObscuration[i][0] != '\0' )
         printf("SfcObscuration      : %s\n",
                   &(Mptr->SfcObscuration[i][0]) );
   }
 
   if( Mptr->Num8thsSkyObscured != MAXINT )
      printf("8ths of SkyObscured : %d\n",Mptr->Num8thsSkyObscured);
 
   if( Mptr->CIGNO )
      printf("CIGNO               : TRUE\n");
 
   if( Mptr->Ceiling != MAXINT )
      printf("Ceiling (ft)        : %d\n",Mptr->Ceiling);
 
   if( Mptr->Estimated_Ceiling != MAXINT )
      printf("Estimated CIG (ft)  : %d\n",Mptr->Estimated_Ceiling);
 
   if( Mptr->VrbSkyBelow[0] != '\0' )
      printf("VRB SKY COND BELOW  : %s\n",Mptr->VrbSkyBelow);
 
   if( Mptr->VrbSkyAbove[0] != '\0' )
      printf("VRB SKY COND ABOVE  : %s\n",Mptr->VrbSkyAbove);
 
   if( Mptr->VrbSkyLayerHgt != MAXINT )
      printf("VRBSKY COND HGT (FT): %d\n",Mptr->VrbSkyLayerHgt);
 
   if( Mptr->ObscurAloftHgt != MAXINT )
      printf("Hgt Obscur Aloft(ft): %d\n",Mptr->ObscurAloftHgt);
 
   if( Mptr->ObscurAloft[0] != '\0' )
      printf("Obscur Phenom Aloft : %s\n",Mptr->ObscurAloft);
 
   if( Mptr->ObscurAloftSkyCond[0] != '\0' )
      printf("Obscur ALOFT SKYCOND: %s\n",Mptr->ObscurAloftSkyCond);
 
 
   if( Mptr->NOSPECI )
      printf("NOSPECI             : TRUE\n");
 
   if( Mptr->LAST )
      printf("LAST                : TRUE\n");
 
   if( Mptr->synoptic_cloud_type[ 0 ] != '\0' )
      printf("SYNOPTIC CLOUD GROUP: %s\n",Mptr->synoptic_cloud_type);
 
   if( Mptr->CloudLow != '\0' )
      printf("LOW CLOUD CODE      : %c\n",Mptr->CloudLow);
 
   if( Mptr->CloudMedium != '\0' )
      printf("MEDIUM CLOUD CODE   : %c\n",Mptr->CloudMedium);
 
   if( Mptr->CloudHigh != '\0' )
      printf("HIGH CLOUD CODE     : %c\n",Mptr->CloudHigh);
 
   if( Mptr->SNINCR != MAXINT )
      printf("SNINCR (INCHES)     : %d\n",Mptr->SNINCR);
 
   if( Mptr->SNINCR_TotalDepth != MAXINT )
      printf("SNINCR(TOT. INCHES) : %d\n",Mptr->SNINCR_TotalDepth);
 
   if( Mptr->snow_depth_group[ 0 ] != '\0' )
      printf("SNOW DEPTH GROUP    : %s\n",Mptr->snow_depth_group);
 
   if( Mptr->snow_depth != MAXINT )
      printf("SNOW DEPTH (INCHES) : %d\n",Mptr->snow_depth);
 
   if( Mptr->WaterEquivSnow != (float) MAXINT )
      printf("H2O EquivSno(inches): %.2f\n",Mptr->WaterEquivSnow);
 
   if( Mptr->SunshineDur != MAXINT )
      printf("SUNSHINE (MINUTES)  : %d\n",Mptr->SunshineDur);
 
   if( Mptr->SunSensorOut )
      printf("SUN SENSOR OUT      : TRUE\n");
 
   if( Mptr->hourlyPrecip != (float) MAXINT )
      printf("HRLY PRECIP (INCHES): %.2f\n",Mptr->hourlyPrecip);
 
   if( Mptr->precip_amt != (float) MAXINT)
      printf("3/6HR PRCIP (INCHES): %.2f\n",
         Mptr->precip_amt);
 
   if( Mptr->Indeterminant3_6HrPrecip )
      printf("INDTRMN 3/6HR PRECIP: TRUE\n");
 
   if( Mptr->precip_24_amt !=  (float) MAXINT)
      printf("24HR PRECIP (INCHES): %.2f\n",
         Mptr->precip_24_amt);
 
   if( Mptr->Temp_2_tenths != (float) MAXINT )
      printf("TMP2TENTHS (CELSIUS): %.1f\n",Mptr->Temp_2_tenths);
 
   if( Mptr->DP_Temp_2_tenths != (float) MAXINT )
      printf("DPT2TENTHS (CELSIUS): %.1f\n",Mptr->DP_Temp_2_tenths);
 
   if( Mptr->maxtemp !=  (float) MAXINT)
      printf("MAX TEMP (CELSIUS)  : %.1f\n",
         Mptr->maxtemp);
 
   if( Mptr->mintemp !=  (float) MAXINT)
      printf("MIN TEMP (CELSIUS)  : %.1f\n",
         Mptr->mintemp);
 
   if( Mptr->max24temp !=  (float) MAXINT)
      printf("24HrMAXTMP (CELSIUS): %.1f\n",
         Mptr->max24temp);
 
   if( Mptr->min24temp !=  (float) MAXINT)
      printf("24HrMINTMP (CELSIUS): %.1f\n",
         Mptr->min24temp);
 
   if( Mptr->char_prestndcy != MAXINT)
      printf("CHAR PRESS TENDENCY : %d\n",
         Mptr->char_prestndcy );
 
   if( Mptr->prestndcy != (float) MAXINT)
      printf("PRES. TENDENCY (hPa): %.1f\n",
         Mptr->prestndcy );
 
   if( Mptr->PWINO )
      printf("PWINO               : TRUE\n");
 
   if( Mptr->PNO )
      printf("PNO                 : TRUE\n");
 
   if( Mptr->CHINO )
      printf("CHINO               : TRUE\n");
 
   if( Mptr->CHINO_LOC[0] != '\0' )
      printf("CHINO_LOC           : %s\n",Mptr->CHINO_LOC);
 
   if( Mptr->VISNO )
      printf("VISNO               : TRUE\n");
 
   if( Mptr->VISNO_LOC[0] != '\0' )
      printf("VISNO_LOC           : %s\n",Mptr->VISNO_LOC);
 
   if( Mptr->FZRANO )
      printf("FZRANO              : TRUE\n");
 
   if( Mptr->TSNO )
      printf("TSNO                : TRUE\n");
 
   if( Mptr->DollarSign)
      printf("DOLLAR $IGN INDCATR : TRUE\n");
 
   if( Mptr->horiz_vsby[ 0 ] != '\0' )
      printf("HORIZ VISIBILITY    : %s\n",Mptr->horiz_vsby);
 
   if( Mptr->dir_min_horiz_vsby[ 0 ] != '\0' )
      printf("DIR MIN HORIZ VSBY  : %s\n",Mptr->dir_min_horiz_vsby);
 
   if( Mptr->CAVOK )
      printf("CAVOK               : TRUE\n");
 
 
   if( Mptr->VertVsby != MAXINT )
      printf("Vert. Vsby (meters) : %d\n",
                  Mptr->VertVsby );
 
   if( Mptr->charVertVsby[0] != '\0' )
      printf("Vert. Vsby (CHAR)   : %s\n",
                  Mptr->charVertVsby );
 
   if( Mptr->QFE != MAXINT )
      printf("QFE                 : %d\n", Mptr->QFE);
 
   if( Mptr->VOLCASH )
      printf("VOLCANIC ASH        : TRUE\n");
 
   if( Mptr->min_vrbl_wind_dir != MAXINT )
      printf("MIN VRBL WIND DIR   : %d\n",Mptr->min_vrbl_wind_dir);
   if( Mptr->max_vrbl_wind_dir != MAXINT )
      printf("MAX VRBL WIND DIR   : %d\n",Mptr->max_vrbl_wind_dir);
 
 
   printf("\n\n\n");
 
 
   return;
 
}
