// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#include "BVHStaticBinary.hxx"

#include "BVHVisitor.hxx"

namespace simgear {

BVHStaticBinary::BVHStaticBinary(unsigned splitAxis,
                                   const BVHStaticNode* leftChild,
                                   const BVHStaticNode* rightChild,
                                   const SGBoxf& boundingBox) :
  _splitAxis(splitAxis),
  _leftChild(leftChild),
  _rightChild(rightChild),
  _boundingBox(boundingBox)
{
}

BVHStaticBinary::~BVHStaticBinary()
{
}

void
BVHStaticBinary::accept(BVHVisitor& visitor, const BVHStaticData& data) const
{
  visitor.apply(*this, data);
}

}
