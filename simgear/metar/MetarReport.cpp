// Metar report implementation class code

#include <simgear/compiler.h>

#include STL_IOSTREAM

#include "MetarReport.h"
#include "Metar.h"

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(endl);
SG_USING_STD(ostream);
#endif

CMetarReport::CMetarReport(
   char *s ) :
		m_DecodedReport( 0 )
{
	m_DecodedReport = new Decoded_METAR;
	DcdMETAR( s, (Decoded_METAR *)m_DecodedReport );
}


CMetarReport::~CMetarReport()
{
}

static int DecodeDirChars( char* c )
{
	int r = 0;

	if ( c[0] )
	{
		if ( c[0] == 'E' ) r = 90;
		else if ( c[0] == 'S' ) r = 180;
		else if ( c[0] == 'W' ) r = 270;

		if ( r == 0 )
		{
			if ( c[1] == 'E' ) r = 45;
			else if ( c[1] == 'W' ) r = 315;
		}
		else if ( r = 180 )
		{
			if ( c[1] == 'E' ) r = 135;
			else if ( c[1] == 'W' ) r = 225;
		}
	}
	return r;
}

char *CMetarReport::StationID()
{
	return ((Decoded_METAR *)m_DecodedReport)->stnid;
}

int CMetarReport::Day() 
{
  return ((Decoded_METAR*)m_DecodedReport)->ob_date;
}

int CMetarReport::Hour() 
{
  return ((Decoded_METAR*)m_DecodedReport)->ob_hour;
}

int CMetarReport::Minutes() 
{
  return ((Decoded_METAR*)m_DecodedReport)->ob_minute;
}

int CMetarReport::WindDirection()
{
	return ((Decoded_METAR *)m_DecodedReport)->winData.windDir;
}

int CMetarReport::WindSpeed()
{
	return ((Decoded_METAR *)m_DecodedReport)->winData.windSpeed;
}

int CMetarReport::WindGustSpeed()
{
	return ((Decoded_METAR *)m_DecodedReport)->winData.windGust;
}

int CMetarReport::LightningDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->LTG_DIR );
}

char CMetarReport::CloudLow()
{
	return ((Decoded_METAR *)m_DecodedReport)->CloudLow;
}

char CMetarReport::CloudMedium()
{
	return ((Decoded_METAR *)m_DecodedReport)->CloudMedium;
}

char CMetarReport::CloudHigh()
{
	return ((Decoded_METAR *)m_DecodedReport)->CloudHigh;
}

int CMetarReport::VirgaDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->VIRGA_DIR );
}

int CMetarReport::TornadicDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->TornadicDIR );
}

int CMetarReport::TornadicMovementDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->TornadicMovDir );
}

int CMetarReport::ThunderStormDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->TS_LOC );
}

int CMetarReport::ThunderStormMovementDirection()
{
	return DecodeDirChars( ((Decoded_METAR *)m_DecodedReport)->TS_MOVMNT );
}

bool CMetarReport::Virga()
{
	return ((Decoded_METAR *)m_DecodedReport)->VIRGA;
}

bool CMetarReport::VolcanicAsh()
{
	return ((Decoded_METAR *)m_DecodedReport)->VOLCASH;
}

bool CMetarReport::Hail()
{
	return ((Decoded_METAR *)m_DecodedReport)->GR;
}

bool CMetarReport::OccationalLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->OCNL_LTG;
}

bool CMetarReport::FrequentLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->FRQ_LTG;
}

bool CMetarReport::ContinuousLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->CNS_LTG;
}

bool CMetarReport::CloudToGroundLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->CG_LTG;
}

bool CMetarReport::InterCloudLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->IC_LTG;
}

bool CMetarReport::CloudToCloudLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->CC_LTG;
}

bool CMetarReport::CloudToAirLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->CA_LTG;
}

bool CMetarReport::DistantLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->DSNT_LTG;
}

bool CMetarReport::AirportLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->AP_LTG;
}

bool CMetarReport::VicinityLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->VcyStn_LTG;
}

bool CMetarReport::OverheadLightning()
{
	return ((Decoded_METAR *)m_DecodedReport)->OVHD_LTG;
}

int CMetarReport::Temperature()
{
	return ((Decoded_METAR *)m_DecodedReport)->temp;
}

int CMetarReport::DewpointTemperature()
{
	return ((Decoded_METAR *)m_DecodedReport)->dew_pt_temp;
}

int CMetarReport::VerticalVisibility() // Meters
{
	return ((Decoded_METAR *)m_DecodedReport)->VertVsby;
}

int CMetarReport::Ceiling()
{
	return SG_FEET_TO_METER * ((Decoded_METAR *)m_DecodedReport)->Ceiling;
}

int CMetarReport::EstimatedCeiling()
{
	return SG_FEET_TO_METER * ((Decoded_METAR *)m_DecodedReport)->Estimated_Ceiling;
}

int CMetarReport::VariableSkyLayerHeight()
{
	return SG_FEET_TO_METER * ((Decoded_METAR *)m_DecodedReport)->VrbSkyLayerHgt;
}

int CMetarReport::SnowDepthInches()
{
	return ((Decoded_METAR *)m_DecodedReport)->snow_depth;
}


ostream&
operator << ( ostream& out, CMetarReport& p )
{
    return out 
	<< "StationID " << p.StationID()
	<< " WindDirection " << p.WindDirection()
	<< " WindSpeed " << p.WindSpeed()
	<< " WindGustSpeed " << p.WindGustSpeed() << endl
	<< "CloudLow " << p.CloudLow()
	<< " CloudMedium " << p.CloudMedium()
	<< " CloudHigh " << p.CloudHigh() << endl
	<< "TornadicDirection " << p.TornadicDirection()
	<< " TornadicMovementDirection " << p.TornadicMovementDirection() << endl
	<< "ThunderStormDirection " << p.ThunderStormDirection()
	<< " ThunderStormMovementDirection " << p.ThunderStormMovementDirection() << endl
	<< "Virga " << p.Virga()
	<< " VirgaDirection " << p.VirgaDirection() << endl
	<< "VolcanicAsh " << p.VolcanicAsh() << endl
	<< "Hail " << p.Hail() << endl
	<< "LightningDirection " << p.LightningDirection()
	<< " OccationalLightning " << p.OccationalLightning()
	<< " FrequentLightning " << p.FrequentLightning()
	<< " ContinuousLightning " << p.ContinuousLightning() << endl
	<< "CloudToGroundLightning " << p.CloudToGroundLightning()
	<< " InterCloudLightning " << p.InterCloudLightning()
	<< " CloudToCloudLightning " << p.CloudToCloudLightning()
	<< " CloudToAirLightning " << p.CloudToAirLightning() << endl
	<< "DistantLightning " << p.DistantLightning()
	<< " AirportLightning " << p.AirportLightning()
	<< " VicinityLightning " << p.VicinityLightning()
	<< " OverheadLightning " << p.OverheadLightning() << endl
	<< "VerticalVisibility " << p.VerticalVisibility() << endl // Meters
	<< "Temperature " << p.Temperature() 
	<< " DewpointTemperature " << p.DewpointTemperature() << endl
	<< "Ceiling " << p.Ceiling()
	<< " EstimatedCeiling " << p.EstimatedCeiling()
	<< " VariableSkyLayerHeight " << p.VariableSkyLayerHeight() << endl
	<< "SnowDepthInches " << p.SnowDepthInches() << endl
	;
}


double CMetarReport::AirPressure() 
{
  return ((Decoded_METAR *)m_DecodedReport)->inches_altstng;
}

void CMetarReport::dump()
{
	prtDMETR( (Decoded_METAR *)m_DecodedReport );
}

double CMetarReport::PrevailVisibility() {
  //Values from each visibility field converted to meters.
  double smiles;
  double km;
  double meters;
  smiles = ((Decoded_METAR*) m_DecodedReport)->prevail_vsbySM  * 621 ;
  km =  ((Decoded_METAR*) m_DecodedReport)->prevail_vsbyKM * 1000;
  meters =  ((Decoded_METAR*) m_DecodedReport)->prevail_vsbyM;
  
  /* Return the smallest one. If the field is specified it should have been
     set to MAX_INT */
  if(smiles < km && smiles < meters) 
    return smiles;
  else 
    return km < meters ? km : meters; 
}
