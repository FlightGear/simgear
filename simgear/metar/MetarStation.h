// Class encapulating the metar station information
//
// METAR station information is kept in this file:
//   http://www.nws.noaa.gov/pub/stninfo/nsd_cccc.gz
//   http://www.nws.noaa.gov/pub/stninfo/nsd_cccc.txt
// This class looks for the file FG_ROOT/Weather/MetarStations instread of nsd_cccc.

#ifndef _MetarStation_
#define _MetarStation_

#include <iostream>
#include <string>
#include <vector>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
//using namespace std;

class CMetarStation
{
	// Attributes
private:
	std::string m_ID;
	unsigned long m_number;
	std::string m_name;
	std::string m_state;
	std::string m_country;
	int m_region;
	Point3D m_locationPolar;
	Point3D m_upperLocationPolar;
	Point3D m_locationCart;
	Point3D m_upperLocationCart;
	int m_altitude;
	int m_upperAltitude;
	char m_pFlag;

	static int initialized;
	static std::string tempName;

	// Operations
private:
	double decodeDMS( char *b );
	static int sameName( CMetarStation *m );

	CMetarStation( 
		char *s );
			// Constructor

	~CMetarStation()
	{
	}
			// Destructor

public:
	std::string &ID() { return m_ID; }
	unsigned long number() { return m_number; }
	std::string &name() { return m_name; }
	std::string &state() { return m_state; }
	std::string &country() { return m_country; }
	int region() { return m_region; }
	Point3D &locationPolar() { return m_locationPolar; }
	Point3D &upperLocationPolar() { return m_upperLocationPolar; }
	Point3D &locationCart() { return m_locationCart; }
	Point3D &upperLocationCart() { return m_upperLocationCart; }
	char pFlag() { return m_pFlag; }
			// Get attributes

    friend std::ostream& operator << ( std::ostream&, const CMetarStation& );
	void dump();
	
	static CMetarStation *find( std::string stationID );
	static CMetarStation *find( Point3D locationCart );
	static void for_each( void f( CMetarStation *s ) );

private:
	CMetarStation(
		const CMetarStation &rNewObj );
			// Copy constructor.  Not implemented.

	CMetarStation &operator =(
		const CMetarStation &rObj );
			// Assignment operator.  Not implemented.

	static int initialize();
};


#endif
