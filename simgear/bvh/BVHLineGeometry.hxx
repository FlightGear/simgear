// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

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
