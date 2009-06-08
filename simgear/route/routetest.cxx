#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <iostream>

#include "route.hxx"
#include "waypoint.hxx"

using std::cout;
using std::endl;

void dump_route(const SGRoute& route, const char* message)
{
    cout << "Route dump: " << message << endl;
    for (int i = 0; i < route.size(); i++) {
        const SGWayPoint wp = route.get_waypoint(i);
        cout << "\t#" << i << " " << wp.get_id() << " (" << wp.get_target_lat()
                << ", " << wp.get_target_lon() << ") @" << wp.get_target_alt()
                << " dist: " << wp.get_distance() << endl;
    }
}

int main()
{
    SGRoute route;
/*
    route.add_waypoint( SGWayPoint(0, 0, 0, SGWayPoint::CARTESIAN, "Start") );
    route.add_waypoint( SGWayPoint(1, 0, 0, SGWayPoint::CARTESIAN, "1") );
    route.add_waypoint( SGWayPoint(2, 0, 0, SGWayPoint::CARTESIAN, "2") );
    route.add_waypoint( SGWayPoint(2, 2, 0, SGWayPoint::CARTESIAN, "3") );
    route.add_waypoint( SGWayPoint(4, 2, 0, SGWayPoint::CARTESIAN, "4") );

    dump_route(route, "Init");
    route.set_current( 1 );

    cout << "( 0.5, 0 ) = " << route.distance_off_route( 0.5, 0 ) << endl;
    cout << "( 0.5, 1 ) = " << route.distance_off_route( 0.5, 1 ) << endl;
    cout << "( 0.5, -1 ) = " << route.distance_off_route( 0.5, 1 ) << endl;

    route.set_current( 3 );

    cout << "( 2, 4 ) = " << route.distance_off_route( 2, 4 ) << endl;
    cout << "( 2.5, 4 ) = " << route.distance_off_route( 2.5, 4 ) << endl;

    SGWayPoint wp2 = route.get_waypoint(2);
    route.delete_waypoint(2);
    dump_route(route, "removed WP2");

    route.add_waypoint(wp2, 3);
    dump_route(route, "added back WP2 after WP3");
*/
    return 0;
}
