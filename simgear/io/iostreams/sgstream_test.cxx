#include <simgear_config.h>

#include <iostream>
#include <cstdlib> // for EXIT_SUCCESS

using std::string;
using std::cout;
using std::endl;

#include <simgear/misc/test_macros.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>

int main()
{
    const string fileName = "testfile";
    simgear::Dir tmpDir = simgear::Dir::tempDir("FlightGear");
    tmpDir.setRemoveOnDestroy();
    SGPath p(tmpDir.path() / fileName);

    {
       sg_ofstream f(p, std::ios::binary | std::ios::trunc | std::ios::out);
       f.write("first line ends with line-feed\n"
           "second line ends with just a cr\r"
           "third line ends with both\r\n"
           "fourth line as well\r\n"
           "fifth line is another CR/LF line\r\n"
           "end of test\r\n", 158);
    }

   sg_gzifstream sg(p);
   std::string stuff;
   sg >> skipeol;
   sg >> stuff;
   SG_CHECK_EQUAL(stuff, "second");
   cout << "Detection of LF works." << endl;

   sg >> skipeol;
   sg >> stuff;
   SG_CHECK_EQUAL(stuff, "third");
   cout << "Detection of CR works." << endl;

   sg >> skipeol;
   sg >> stuff;
   SG_CHECK_EQUAL(stuff, "fourth");
   cout << "Detection of CR/LF works." << endl;

   sg >> skipeol;
   sg >> skipeol;
   sg >> stuff;
   SG_CHECK_EQUAL(stuff, "end");
   cout << "Detection of 2 following CR/LF lines works." << endl;

   return EXIT_SUCCESS;
}
