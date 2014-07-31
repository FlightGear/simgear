/** \file
 *
 * Forward declarations for properties (and related structures)
 */
 
#ifndef SG_PROPS_FWD_HXX
#define SG_PROPS_FWD_HXX

#include <simgear/structure/SGSharedPtr.hxx>

class SGPropertyNode;

typedef SGSharedPtr<SGPropertyNode> SGPropertyNode_ptr;
typedef SGSharedPtr<const SGPropertyNode> SGConstPropertyNode_ptr;

class SGCondition; 

#endif // of SG_PROPS_FWD_HXX
