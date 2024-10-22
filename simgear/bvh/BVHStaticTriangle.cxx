// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#include "BVHStaticTriangle.hxx"

#include "BVHVisitor.hxx"

namespace simgear {

BVHStaticTriangle::BVHStaticTriangle(unsigned material, const unsigned indices[3]) :
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
