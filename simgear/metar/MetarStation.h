// Class encapulating the metar station information
//
// METAR station information is kept in this file:
//   http://www.nws.noaa.gov/pub/stninfo/nsd_cccc.gz
//   http://www.nws.noaa.gov/pub/stninfo/nsd_cccc.txt


#ifndef _MetarStation_
#define _MetarStation_

#include <simgear/compiler.h>

#include STL_IOSTREAM
#include STL_STRING
#include <vector>
#include <map>

#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>

FG_USING_STD(string);
FG_USING_STD(vector);
FG_USING_STD(map);

class CMetarStationDB;

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
	
	// Operations
private:
	double decodeDMS( char *b );



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
	unsigned int Altitude() { //Returns the stations height above MSL in M.
			return m_altitude;}
	Point3D &locationPolar() { return m_locationPolar; }
	Point3D &upperLocationPolar() { return m_upperLocationPolar; }
	Point3D &locationCart() { return m_locationCart; }
	Point3D &upperLocationCart() { return m_upperLocationCart; }
	char pFlag() { return m_pFlag; }
			// Get attributes

    friend std::ostream& operator << ( std::ostream&, const CMetarStation& );
	void dump();
	

	

private:
	CMetarStation(
		const CMetarStation &rNewObj );
			// Copy constructor.  Not implemented.

	CMetarStation &operator =(
		const CMetarStation &rObj );
			// Assignment operator.  Not implemented.

	friend CMetarStationDB;
};


class CMetarStationDB 
{

 private:
    std::string databasePath;  //The path of the database file.
    std::map<std::string , CMetarStation *> METAR_Stations;
    CMetarStation * bestStation;

 public:
     CMetarStation *find( std::string stationID );
     CMetarStation * find( Point3D locationCart );
     CMetarStationDB(const char * dbFile);
     ~CMetarStationDB();
};

#endif
