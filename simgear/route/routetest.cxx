#include <simgear/compiler.h>
#include <simgear/constants.h>

#include STL_IOSTREAM

#include "route.hxx"
#include "waypoint.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif
 
int main() {
    SGRoute route;

    route.add_waypoint( SGWayPoint(0, 0, 0, SGWayPoint::CARTESIAN, "Start") );
    route.add_waypoint( SGWayPoint(1, 0, 0, SGWayPoint::CARTESIAN, "1") );
    route.add_waypoint( SGWayPoint(2, 0, 0, SGWayPoint::CARTESIAN, "2") );
    route.add_waypoint( SGWayPoint(2, 2, 0, SGWayPoint::CARTESIAN, "3") );
    route.add_waypoint( SGWayPoint(4, 2, 0, SGWayPoint::CARTESIAN, "4") );
   
    route.set_current( 1 );

    cout << "( 0.5, 0 ) = " << route.distance_off_route( 0.5, 0 ) << endl;
    cout << "( 0.5, 1 ) = " << route.distance_off_route( 0.5, 1 ) << endl;
    cout << "( 0.5, -1 ) = " << route.distance_off_route( 0.5, 1 ) << endl;

    route.set_current( 3 );

    cout << "( 2, 4 ) = " << route.distance_off_route( 2, 4 ) << endl;
    cout << "( 2.5, 4 ) = " << route.distance_off_route( 2.5, 4 ) << endl;

    return 0;
}
