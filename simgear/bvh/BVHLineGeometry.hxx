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

#ifndef BVHLineGeometry_hxx
#define BVHLineGeometry_hxx

#include <simgear/math/SGGeometry.hxx>
#include "BVHNode.hxx"

namespace simgear {

class BVHLineGeometry : public BVHNode {
public:
    enum Type {
        CarrierCatapult,
        CarrierWire
    };

    BVHLineGeometry(const SGLineSegmentf& lineSegment, Type type);
    virtual ~BVHLineGeometry();

    virtual void accept(BVHVisitor& visitor);

    const SGLineSegmentf& getLineSegment() const
    { return _lineSegment; }

    Type getType() const
    { return _type; }

    virtual SGSphered computeBoundingSphere() const;

private:
    SGLineSegmentf _lineSegment;
    Type _type;
};

}

#endif
