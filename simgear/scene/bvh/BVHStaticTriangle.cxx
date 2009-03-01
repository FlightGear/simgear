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

#include "BVHStaticTriangle.hxx"

#include "BVHVisitor.hxx"

namespace simgear {

BVHStaticTriangle::BVHStaticTriangle(unsigned material,
                                       const unsigned indices[3]) :
  _material(material)
{
    for (unsigned i = 0; i < 3; ++i)
        _indices[i] = indices[i];
}

BVHStaticTriangle::~BVHStaticTriangle()
{
}

void
BVHStaticTriangle::accept(BVHVisitor& visitor, const BVHStaticData& data) const
{
    visitor.apply(*this, data);
}

SGBoxf
BVHStaticTriangle::computeBoundingBox(const BVHStaticData& data) const
{
    SGBoxf box;
    box.expandBy(data.getVertex(_indices[0]));
    box.expandBy(data.getVertex(_indices[1]));
    box.expandBy(data.getVertex(_indices[2]));
    return box;
}

SGVec3f
BVHStaticTriangle::computeCenter(const BVHStaticData& data) const
{
    return getTriangle(data).getCenter();
}

}
