/**
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/props/props.hxx>
#include "persparam.hxx"

template <> double
SGPersonalityParameter<double>::getNodeValue( const SGPropertyNode *props,
                                              const char *name,
                                              double defval ) const
{
  return props->getDoubleValue( name, defval );
}
