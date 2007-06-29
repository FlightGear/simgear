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

int main (int ac, const char ** av)
{
  MyVisitor visitor;

  for (int i = 1; i < ac; i++) {
    ifstream input(av[i]);
    cout << "Reading " << av[i] << endl;
    try {
      readXML(input, visitor);

    } catch (const sg_exception& e) {
      cerr << "Error: file '" << av[i] << "' " << e.getFormattedMessage() << endl;
      return -1;

    } catch (...) {
      cerr << "Error reading from " << av[i] << endl;
      return -1;
    }
  }
  return 0;
}
