/**
 * $Id$
 */

#include <simgear/props/props.hxx>
#include "persparam.hxx"

template <> double
SGPersonalityParameter<double>::getNodeValue( SGPropertyNode *props,
                                              const char *name,
                                              double defval ) const
{
  return props->getDoubleValue( name, defval );
}
