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

typedef SGSharedPtr<SGPropertyNode> SGPropertyNode_ptr;
typedef SGSharedPtr<const SGPropertyNode> SGConstPropertyNode_ptr;

namespace simgear
{
    using PropertyList = std::vector<SGPropertyNode_ptr>;
}

class SGCondition; 

typedef SGSharedPtr<SGCondition> SGConditionRef;


