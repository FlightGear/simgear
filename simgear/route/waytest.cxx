#include <simgear/constants.h>
#include "waypoint.hxx"

int main() {
    SGWayPoint a1(-93.216923, 44.880547, SGWayPoint::WGS84, "KMSP");
    SGWayPoint a2(-93.216923, 44.880547, SGWayPoint::SPHERICAL, "KMSP");

    // KMSN (Madison)
    double cur_lon = -89.336939;
    double cur_lat = 43.139541;

    double course, distance;

    a1.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << a1.get_id() << " is " << course << endl;
    cout << "Distance to " << a1.get_id() << " is " << distance * METER_TO_NM
	 << endl;

    a2.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << a2.get_id() << " is " << course << endl;
    cout << "Distance to " << a2.get_id() << " is " << distance * METER_TO_NM
	 << endl;
    cout << endl;

    SGWayPoint b1(-88.237037, 43.041038, SGWayPoint::WGS84, "KUES");
    SGWayPoint b2(-88.237037, 43.041038, SGWayPoint::SPHERICAL, "KUES");

    b1.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << b1.get_id() << " is " << course << endl;
    cout << "Distance to " << b1.get_id() << " is " << distance * METER_TO_NM
	 << endl;

    b2.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << b2.get_id() << " is " << course << endl;
    cout << "Distance to " << b2.get_id() << " is " << distance * METER_TO_NM
	 << endl;
    cout << endl;

    cur_lon = 10;
    cur_lat = 10;

    SGWayPoint c1(-20, 10, SGWayPoint::CARTESIAN, "Due East");
    c1.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << c1.get_id() << " is " << course << endl;
    cout << "Distance to " << c1.get_id() << " is " << distance << endl;
   
    SGWayPoint c2(20, 20, SGWayPoint::CARTESIAN, "Due SW");
    c2.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << c2.get_id() << " is " << course << endl;
    cout << "Distance to " << c2.get_id() << " is " << distance << endl;
   
    SGWayPoint c3(20, 0, SGWayPoint::CARTESIAN, "Due NW");
    c3.CourseAndDistance( cur_lon, cur_lat, &course, &distance );
    cout << "Course to " << c3.get_id() << " is " << course << endl;
    cout << "Distance to " << c3.get_id() << " is " << distance << endl;
   
    return 0;
}
