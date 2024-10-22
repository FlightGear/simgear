// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 James Turner <james@flightgear.com>

/**
 * @file
 * @brief Forward declarations for properties (and related structures)
 */

#ifndef SG_PROPS_FWD_HXX
#define SG_PROPS_FWD_HXX

#include <simgear/structure/SGSharedPtr.hxx>
#include <vector>

class SGPropertyNode;

typedef SGSharedPtr<SGPropertyNode> SGPropertyNode_ptr;
typedef SGSharedPtr<const SGPropertyNode> SGConstPropertyNode_ptr;

namespace simgear
{
    using PropertyList = std::vector<SGPropertyNode_ptr>;
}

class SGCondition; 

#endif // of SG_PROPS_FWD_HXX
