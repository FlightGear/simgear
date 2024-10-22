// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHStaticLeaf_hxx
#define BVHStaticLeaf_hxx

#include <simgear/math/SGGeometry.hxx>
#include "BVHStaticNode.hxx"

namespace simgear {

class BVHStaticLeaf : public BVHStaticNode {
public:
  virtual ~BVHStaticLeaf();

  virtual void accept(BVHVisitor& visitor, const BVHStaticData& data) const = 0;

  virtual SGBoxf computeBoundingBox(const BVHStaticData&) const = 0;
  virtual SGVec3f computeCenter(const BVHStaticData&) const = 0;
};

}

#endif
