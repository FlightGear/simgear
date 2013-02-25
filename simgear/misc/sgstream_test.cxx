#include <iostream>
#include <fstream>

#include <cstdlib> // for EXIT_FAILURE

using std::ofstream;
using std::cout;
using std::endl;

#include <simgear/misc/sgstream.hxx>

int main()
{
    const char* fileName = "testfile";
    {
       ofstream f;
       f.open(fileName, std::ios::binary | std::ios::trunc | std::ios::out);
       f.write("first line ends with line-feed\n"
           "second line ends with just a cr\r"
           "third line ends with both\r\n"
           "fourth line as well\r\n"
           "fifth line is another CR/LF line\r\n"
           "end of test\r\n", 1024);
       f.close();
   }

   sg_gzifstream sg(fileName);
   std::string stuff;
   sg >> skipeol;
   sg >> stuff;
   if (stuff != "second") return EXIT_FAILURE;
   cout << "Detection of LF works." << endl;

   sg >> skipeol;
   sg >> stuff;
   if (stuff != "third") return EXIT_FAILURE;
   cout << "Detection of CR works." << endl;

   sg >> skipeol;
   sg >> stuff;
   if (stuff != "fourth") return EXIT_FAILURE;
   cout << "Detection of CR/LF works." << endl;

   sg >> skipeol;
   sg >> skipeol;
   sg >> stuff;
   if (stuff != "end") return EXIT_FAILURE;
   cout << "Detection of 2 following CR/LF lines works." << endl;

   return EXIT_SUCCESS;
}
