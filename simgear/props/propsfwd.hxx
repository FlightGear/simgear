// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 James Turner <james@flightgear.com>

/**
 * @file
 * @brief Forward declarations for properties (and related structures)
 */

#pragma once

#include <simgear/structure/SGSharedPtr.hxx>
#include <vector>

class SGPropertyNode;

using SGPropertyNode_ptr = SGSharedPtr<SGPropertyNode>;
using SGConstPropertyNode_ptr = SGSharedPtr<const SGPropertyNode>;

namespace simgear
{
    using PropertyList = std::vector<SGPropertyNode_ptr>;
}

class SGCondition; 

using SGConditionRef = SGSharedPtr<SGCondition> ;


