
#include <iostream>
#include <simgear/timing/timestamp.hxx>

using namespace std;

int main()
{
    cout << "About to benchmark SGTimeStamp for 5 seconds. Press ENTER when ready." << endl;
    int c = cin.get();
    long nb = 0;
    SGTimeStamp start, now;
    start.stamp();
    do {
        nb += 1;
        now.stamp();
    } while ( ( now - start ).toMicroSeconds() < 5000000 );

    cout << ( nb / 5 ) << " iterations per seconds. Press ENTER to quit." << endl;
    c = cin.get();
}
