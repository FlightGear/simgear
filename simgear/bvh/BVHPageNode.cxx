// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2012 Mathias Froehlich <mathias.froehlich@web.de>

#include <simgear_config.h>

#include "BVHPageNode.hxx"

#include "BVHPager.hxx"

namespace simgear {

BVHPageNode::BVHPageNode() :
    _useStamp(0),
    _requested(false)
{
}

BVHPageNode::~BVHPageNode()
{
}

void
BVHPageNode::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

}
