// Metar station implementation code

#include <stdio.h>

#include "MetarStation.h"
#include <algorithm>

#include <simgear/misc/fgpath.hxx>


double CMetarStation::decodeDMS( char *b )
{
	double r = 0;
	double m = 0;
	double s = 0;
	if ( *b )
	{
		// Degrees
		r = (*b - '0') * 10.0; b++;
		r += (*b - '0'); b++;
		if ( *b != '-' )
		{
			r *= 10;
			r += (*b - '0'); b++;
		}
		b++;
		// Minutes
		m = (*b - '0') * 10.0; b++;
		m += (*b - '0'); b++;
		r += m/60.0;
		if ( *b == '-' )
		{
			// Seconds
			b++;
			s = (*b - '0') * 10.0; b++;
			s += (*b - '0'); b++;
		}
		r += s/3600.0;
		// Direction (E W N S)
		if ( *b == 'W' || *b == 'S' ) r = -r;
	}
	return r * DEG_TO_RAD;
}

// Constructor
// Decodes METAR station string of this format:
// KPUB;72;464;Pueblo, Pueblo Memorial Airport;CO;United States;4;38-17-24N;104-29-54W;38-17-03N;104-29-43W;1440;1420;P

CMetarStation::CMetarStation( 
	char *s )
{
	char *t;
	t = strchr( s, ';' ); *t = 0; t++;
	m_ID = s;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_number = atoi( s ) * 1000;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_number += atoi( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_name = s;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_state = s;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_country = s;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	m_region = atoi( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double latitude = decodeDMS( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double longitude = decodeDMS( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double ulatitude = decodeDMS( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double ulongitude = decodeDMS( s );
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double altitude = atoi( s ) * FEET_TO_METER;
	m_altitude = altitude;
	s = t; t = strchr( s, ';' ); *t = 0; t++;
	double ualtitude = atoi( s ) * FEET_TO_METER;
	Point3D p( longitude, latitude, altitude+EQUATORIAL_RADIUS_M );
	m_locationPolar = p;
	m_locationCart = sgPolarToCart3d( p );
	Point3D up( ulongitude, ulatitude, ualtitude+EQUATORIAL_RADIUS_M );
	m_upperLocationPolar = up;
	m_upperLocationCart = sgPolarToCart3d( up );
	s = t;
	m_pFlag = s[0];
}
		


void CMetarStation::dump()
{
	std::cout << "ID:" << ID();
	std::cout << std::endl;
	std::cout << "number:" << number();
	std::cout << std::endl;
	std::cout << "name:" << name();
	std::cout << std::endl;
	std::cout << "state:" << state();
	std::cout << std::endl;
	std::cout << "country:" << country();
	std::cout << std::endl;
	std::cout << "region:" << region();
	std::cout << std::endl;
	std::cout << "Location (cart):" << locationCart();
	std::cout << std::endl;
	std::cout << "Location (polar):" << locationPolar();
	std::cout << std::endl;
	std::cout << "Upper Location (cart):" << upperLocationCart();
	std::cout << std::endl;
	std::cout << "Upper Location (polar):" << upperLocationPolar();
	std::cout << std::endl;
	std::cout << "P flag:" << pFlag();
	std::cout << std::endl;
}



CMetarStationDB::CMetarStationDB(const char * dbPath) 
{
    // Read the list of metar stations, decoding and adding to global list.

    CMetarStation *m;
    char buf[256];


    // Open the metar station list
    FILE *f = fopen( dbPath, "r" );
	

    if ( f != NULL ) {
	// Read each line, create an instance of a station, and add it to the vector
	while ( fgets( buf, 256, f) != NULL && feof( f ) == 0 ) {
	    //std::cout << buf << std::endl;
	    m = new CMetarStation( buf );
	    //m->dump();
	    METAR_Stations[m->ID()]=( m );
	}
	
	// Close the list
	fclose( f );
	// std::cout << METAR_Stations.size() << " Metar stations" << std::endl;
	
    } else {
	// std::cout << "Could not open MetarStations file " << std::endl;
	
    }
}



CMetarStation * CMetarStationDB::find( std::string stationID )
{
    std::map<std::string,CMetarStation*>::iterator target;
    target = METAR_Stations.find(stationID);
  if(target!= METAR_Stations.end() )
      return target->second;
  else 
      return NULL;
}



CMetarStation * CMetarStationDB::find( Point3D locationCart )
{
    std::map<std::string,CMetarStation*>::iterator itr;
    double bestDist = 99999999;
    CMetarStation * bestStation;
    Point3D curLocation = locationCart;
    itr = METAR_Stations.begin(); 
    while(itr != METAR_Stations.end()) 
      {
	double dist = itr->second->locationCart().distance3Dsquared( curLocation );
	if (dist < bestDist )
	  {
	    bestDist = dist;
	    bestStation = itr->second;
	  }
	itr++;
      }
    
    return bestStation;
}


CMetarStationDB::~CMetarStationDB() {
    std::map<std::string,CMetarStation*>::iterator itr;
    for(itr = METAR_Stations.begin(); itr != METAR_Stations.end(); itr++) 
      {
	delete itr->second;
    }
}

std::ostream&
operator << ( ostream& out, const CMetarStation& p )
{
    return out 
		<< "ID:" << p.m_ID << std::endl
		<< "number:" << p.m_number << std::endl
		<< "name:" << p.m_name << std::endl
		<< "state:" << p.m_state << std::endl
		<< "country:" << p.m_country << std::endl
		<< "region:" << p.m_region << std::endl
		<< "Location (cart):" << p.m_locationCart << std::endl
		<< "Location (polar):" << p.m_locationCart << std::endl
		<< "Upper Location (cart):" << p.m_upperLocationCart << std::endl
		<< "Upper Location (polar):" << p.m_upperLocationPolar << std::endl
		<< "P flag:" << p.m_pFlag << std::endl;
}

