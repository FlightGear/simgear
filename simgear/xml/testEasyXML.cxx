#include <string>
#include <iostream>
#include <fstream>
#include "easyxml.hxx"

using std::string;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;

class MyVisitor : public XMLVisitor
{
public:
  virtual void startXML () {
    cout << "Start XML" << endl;
  }
  virtual void endXML () {
    cout << "End XML" << endl;
  }
  virtual void startElement (const char * name, const XMLAttributes &atts) {
    cout << "Start element " << name << endl;
    for (int i = 0; i < atts.size(); i++)
      cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl;
  }
  virtual void endElement (const char * name) {
    cout << "End element " << name << endl;
  }
  virtual void data (const char * s, int len) {
    cout << "Character data " << string(s,len) << endl;
  }
  virtual void pi (const char * target, const char * data) {
    cout << "Processing instruction " << target << ' ' << data << endl;
  }
  virtual void warning (const char * message, int line, int column) {
    cout << "Warning: " << message << " (" << line << ',' << column << ')'
	 << endl;
  }
  virtual void error (const char * message, int line, int column) {
    cout << "Error: " << message << " (" << line << ',' << column << ')'
	 << endl;
  }
};

main (int ac, const char ** av)
{
  MyVisitor visitor;

  for (int i = 1; i < ac; i++) {
    ifstream input(av[i]);
    cout << "Reading " << av[i] << endl;
    if (!readXML(input, visitor)) {
      cerr << "Error reading from " << av[i] << endl;
    }
  }
}
