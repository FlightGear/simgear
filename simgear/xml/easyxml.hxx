#ifndef __EASYXML_HXX
#define __EASYXML_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_IOSTREAM
#include STL_STRING
#include <vector>

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istream);
#endif
SG_USING_STD(string);
SG_USING_STD(vector);


/**
 * Interface for XML attributes.
 */
class XMLAttributes
{
public:
  XMLAttributes ();
  virtual ~ XMLAttributes ();

  virtual int size () const = 0;
  virtual const char * getName (int i) const = 0;
  virtual const char * getValue (int i) const = 0;

  virtual int findAttribute (const char * name) const;
  virtual bool hasAttribute (const char * name) const;
  virtual const char * getValue (const char * name) const;
};


/**
 * Default mutable attributes implementation.
 */
class XMLAttributesDefault : public XMLAttributes
{
public:
  XMLAttributesDefault ();
  XMLAttributesDefault (const XMLAttributes & atts);
  virtual ~XMLAttributesDefault ();

  virtual int size () const;
  virtual const char * getName (int i) const;
  virtual const char * getValue (int i) const;

  virtual void addAttribute (const char * name, const char * value);
  virtual void setName (int i, const char * name);
  virtual void setValue (int i, const char * value);
  virtual void setValue (const char * name, const char * value);

private:
  vector<string> _atts;
};


/**
 * Visitor class for an XML document.
 *
 * To read an XML document, a module must subclass the visitor to do
 * something useful for the different XML events.
 */
class XMLVisitor
{
public:
				// start an XML document
  virtual void startXML () {}
				// end an XML document
  virtual void endXML () {}
				// start an element
  virtual void startElement (const char * name, const XMLAttributes &atts) {}
				// end an element
  virtual void endElement (const char * name) {}
				// character data
  virtual void data (const char * s, int length) {}
				// processing instruction
  virtual void pi (const char * target, const char * data) {}
				// non-fatal warning
  virtual void warning (const char * message, int line, int column) {}
				// fatal error
  virtual void error (const char * message, int line, int column) = 0;
};


/**
 * Read an XML document.
 */
extern bool readXML (istream &input, XMLVisitor &visitor);


#endif // __EASYXML_HXX

