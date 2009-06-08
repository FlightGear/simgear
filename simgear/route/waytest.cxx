#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <iostream>

#include "waypoint.hxx"

using std::cout;
using std::endl;

int main() {
    SGWayPoint a1(-93.216923, 44.880547, 0.0, SGWayPoint::WGS84, "KMSP");
    SGWayPoint a2(-93.216923, 44.880547, 0.0, SGWayPoint::SPHERICAL, "KMSP");

    // KMSN (Madison)
    double cur_lon = -89.336939;
    double cur_lat = 43.139541;
    double cur_alt = 0.0;

    double course, distance;

    a1.CourseAndDistance( cur_lon, cur_lat, cur_alt, &course, &distance );
    cout << "Course to " << a1.get_id() << " is " << course << endl;
    cout << "Distance to " << a1.get_id() << " is " << distance * SG_METER_TO_NM
	 << endl;

    a2.CourseAndDistance( cur_lon, cur_lat, cur_alt, &course, &distance );
    cout << "Course to " << a2.get_id() << " is " << course << endl;
    cout << "Distance to " << a2.get_id() << " is " << distance * SG_METER_TO_NM
	 << endl;
    cout << endl;

    SGWayPoint b1(-88.237037, 43.041038, 0.0, SGWayPoint::WGS84, "KUES");
    SGWayPoint b2(-88.237037, 43.041038, 0.0, SGWayPoint::SPHERICAL, "KUES");

    b1.CourseAndDistance( cur_lon, cur_lat, cur_alt, &course, &distance );
    cout << "Course to " << b1.get_id() << " is " << course << endl;
    cout << "Distance to " << b1.get_id() << " is " << distance * SG_METER_TO_NM
	 << endl;

    b2.CourseAndDistance( cur_lon, cur_lat, cur_alt, &course, &distance );
    cout << "Course to " << b2.get_id() << " is " << course << endl;
    cout << "Distance to " << b2.get_id() << " is " << distance * SG_METER_TO_NM
	 << endl;
    cout << endl;
   
    return 0;
}
