/* 2/14/00 fixed help message- dip angle (down positive), variation (E positive) */

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include <simgear/constants.h>

#include "coremag.hxx"


int main(int argc, char *argv[])
{
  /* args are double lat_deg, double lon_deg, double h, 
                   int mm, int dd, int yy,int model */
  /* output N, E, down components of B (nTesla)
     dip angle (down positive), variation (E positive) */
double lat_deg,lon_deg,h,var;
int /* model,*/yy,mm,dd;
double field[6];

if ((argc != 8) && (argc !=7)) {
fprintf(stdout,"Usage: mag lat_deg lon_deg h mm dd yy [model]\n");
fprintf(stdout,"N latitudes, E longitudes positive degrees, h in km, mm dd yy is date\n");
fprintf(stdout,"model 1,2,3,4,5,6,7 <=> IGRF90,WMM85,WMM90,WMM95,IGRF95,WMM2000,IGRF2000\n");
fprintf(stdout,"Default model is IGRF2000, valid 1/1/00 - 12/31/05\n");
fprintf(stdout,"Output Bx (N) By (E) Bz (down) (in nTesla) dip (degrees down positive)\n");
fprintf(stdout,"variation (degrees E positive)\n");
exit(1);
}

lat_deg=strtod(argv[1],NULL);
lon_deg=strtod(argv[2],NULL);
h=      strtod(argv[3],NULL);
mm=     (int)strtol(argv[4],NULL,10);
dd=     (int)strtol(argv[5],NULL,10);
yy=     (int)strtol(argv[6],NULL,10);
if (argc == 8){
//  model=  (int)strtol(argv[7],NULL,10);
}else{
//  model=7;
}


var = calc_magvar( SGD_DEGREES_TO_RADIANS * lat_deg, SGD_DEGREES_TO_RADIANS * lon_deg, h,
		   yymmdd_to_julian_days(yy,mm,dd), field );

fprintf(stdout,"%6.0f %6.0f %6.0f\n", field[0], field[1], field[2] );
fprintf(stdout,"%6.0f %6.0f %6.0f\n", field[3], field[4], field[5] );
fprintf(stdout,"%6.0f %6.0f %6.0f %4.2f %4.2f \n",
  field[3],field[4],field[5],
  SGD_RADIANS_TO_DEGREES * (atan(field[5]/pow(field[3]*field[3]+field[4]*field[4],0.5))),
  SGD_RADIANS_TO_DEGREES * var);
exit(0);
}
  
  

