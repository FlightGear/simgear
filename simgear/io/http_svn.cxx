#include <cstdio>
#include <cstring>
#include <signal.h>

#include <iostream>
#include <boost/foreach.hpp>


#include <simgear/io/sg_file.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/sg_netChannel.hxx>
#include <simgear/io/DAVMultiStatus.hxx>
#include <simgear/io/SVNRepository.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>

using namespace simgear;
using std::cout;
using std::endl;
using std::cerr;
using std::string;

HTTP::Client* httpClient;

int main(int argc, char* argv[])
{
  sglog().setLogLevels( SG_ALL, SG_INFO );
  HTTP::Client cl;
  httpClient = &cl;

  
  SGPath p("/home/jmt/Desktop/scenemodels");
  SVNRepository airports(p, &cl);
 //  airports.setBaseUrl("http://svn.goneabitbursar.com/testproject1");
  airports.setBaseUrl("http://terrascenery.googlecode.com/svn/trunk/data/Scenery/Models");
  
//  airports.setBaseUrl("http://terrascenery.googlecode.com/svn/trunk/data/Scenery/Airports");
  airports.update();
  
  while (airports.isDoingSync()) {
    cl.update(100);
  }
  
  cout << "all done!" << endl;
  return EXIT_SUCCESS;
}