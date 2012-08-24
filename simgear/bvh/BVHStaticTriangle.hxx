// Copyright (C) 2008 - 2009  Mathias Froehlich - Mathias.Froehlich@web.de
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
