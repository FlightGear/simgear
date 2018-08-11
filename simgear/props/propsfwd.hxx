// Copyright (C) 2018  James Turner - <james@flightgear>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

/** \file
 *
 * Forward declarations for properties (and related structures)
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
