// Class encapulating the metar report information
//
// Individual METAR reports are found in this directory:
//   ftp://weather.noaa.gov/data/observations/metar/stations
//

#ifndef _MetarReport_
#define _MetarReport_

#include <iostream>
#include <string>
#include <vector>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>

class CMetarReport
{
	// Typedefs, enumerations

	// Attributes
private:
	void *m_DecodedReport;
		// A void pointer which is cast to the correct type in the cpp file.
		// I don't want the ugly metar structure and header files dragged into
		// every report user program file.
		// Probably should be a smart pointer if copy constructor and assignment
		// is allowed.

	// Operations

public:
	CMetarReport( 
		char *s );
			// Constructor

	~CMetarReport();
			// Destructor

	char *StationID();

	int Day();    // The day of month on which the report was issued.
	int Hour();   // The hour of the day the report was issued.
	int Minutes();  //Minutes past the hour of the report issue time.

	// Directions in degrees
	// Speed in knots
	// Altitude in meters
	// Temperature in centigrade

	int WindDirection();
	int WindSpeed();
	int WindGustSpeed();

	// Add cloud more cloud info...
	// Cloud code characters...
	char CloudLow();
	char CloudMedium();
	char CloudHigh();

	bool Virga();
	int VirgaDirection();

	int TornadicDirection();	
	int TornadicMovementDirection();

	int ThunderStormDirection();
	int ThunderStormMovementDirection();
	
	bool VolcanicAsh();
	bool Hail();

	int LightningDirection();
	bool OccationalLightning();
	bool FrequentLightning();
	bool ContinuousLightning();
	bool Lightning()
	{ 
		return OccationalLightning() || FrequentLightning() || ContinuousLightning();
	}

	bool CloudToGroundLightning();
	bool InterCloudLightning();
	bool CloudToCloudLightning();
	bool CloudToAirLightning();

	bool DistantLightning();
	bool AirportLightning();
	bool OverheadLightning();
	bool VicinityLightning();

	int Temperature();
	int DewpointTemperature();

	int VerticalVisibility();
	int Ceiling();
	int EstimatedCeiling();
	int VariableSkyLayerHeight();

	int SnowDepthInches();

	double AirPressure();  //Return the air pressure in InchesHg.
	double PrevailVisibility(); // Prevailing Visibility in meters.
 	void dump();

private:
	CMetarReport(
		const CMetarReport &rNewObj );
			// Copy constructor.  Not implemented.

	CMetarReport &operator =(
		const CMetarReport &rObj );
			// Assignment operator.  Not implemented.
};

std::ostream& operator << ( std::ostream&, CMetarReport& );

#endif
