// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHStaticTriangle_hxx
#define BVHStaticTriangle_hxx

#include <simgear/math/SGGeometry.hxx>
#include "BVHStaticData.hxx"
#include "BVHStaticLeaf.hxx"

namespace simgear {

class BVHStaticTriangle : public BVHStaticLeaf {
public:
  BVHStaticTriangle(unsigned material, const unsigned indices[3]);
  virtual ~BVHStaticTriangle();

  virtual void accept(BVHVisitor& visitor, const BVHStaticData& data) const;

  virtual SGBoxf computeBoundingBox(const BVHStaticData& data) const;
  virtual SGVec3f computeCenter(const BVHStaticData& data) const;

  SGTrianglef getTriangle(const BVHStaticData& data) const
  {
    return SGTrianglef(data.getVertex(_indices[0]),
                       data.getVertex(_indices[1]),
                       data.getVertex(_indices[2]));
  }

  unsigned getMaterialIndex() const
  { return _material; }

private:
  unsigned _indices[3];
  unsigned _material;
};

}

#endif
