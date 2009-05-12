#include <string>
#include <simgear/debug/logstream.hxx>

class Test {

	public:
	Test() {
	  //cout << "Cout seems to work" << endl;
	  //cerr << "Cerr seems to work" << endl;
	  
	  sglog().setLogLevels( SG_ALL, SG_INFO );
	  
	  SG_LOG(SG_EVENT, SG_INFO, "Test::Test" << "foo");
	}

};

Test test;

int
main( int argc, char* argv[] )
{
    sglog().setLogLevels( SG_ALL, SG_INFO );

    //SG_LOG( FG_TERRAIN, FG_BULK,  "terrain::bulk" ); // shouldnt appear
    //SG_LOG( FG_TERRAIN, SG_DEBUG, "terrain::debug" ); // shouldnt appear
    //SG_LOG( FG_TERRAIN, SG_INFO,  "terrain::info" );
    //SG_LOG( FG_TERRAIN, FG_WARN,  "terrain::warn" );
    //SG_LOG( FG_TERRAIN, SG_ALERT, "terrain::alert" );

    int i = 12345;
    long l = 54321L;
    double d = 3.14159;
    string s = "Hello world!";

    SG_LOG( SG_EVENT, SG_INFO, "event::info "
				 << "i=" << i
				 << ", l=" << l
				 << ", d=" << d
	                         << ", d*l=" << d*l
				 << ", s=\"" << s << "\"" );

    // This shouldn't appear in log output:
    SG_LOG( SG_EVENT, SG_DEBUG, "event::debug "
	    << "- this should be seen - "
	    << "d=" << d
	    << ", s=\"" << s << "\"" );

    return 0;
}
