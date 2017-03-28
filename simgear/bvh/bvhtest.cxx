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

#include <simgear_config.h>
#include <iostream>
#include <simgear/structure/SGSharedPtr.hxx>

#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHTransform.hxx"

#include "BVHStaticData.hxx"

#include "BVHStaticNode.hxx"
#include "BVHStaticLeaf.hxx"
#include "BVHStaticTriangle.hxx"
#include "BVHStaticBinary.hxx"
#include "BVHStaticGeometry.hxx"

#include "BVHBoundingBoxVisitor.hxx"
#include "BVHSubTreeCollector.hxx"
#include "BVHLineSegmentVisitor.hxx"
#include "BVHNearestPointVisitor.hxx"

using namespace simgear;

BVHNode*
buildSingleTriangle(const SGVec3f& v1, const SGVec3f& v2, const SGVec3f& v3)
{
    BVHStaticData* staticData = new BVHStaticData;
    unsigned indices[3] = {
        staticData->addVertex(v1),
        staticData->addVertex(v2),
        staticData->addVertex(v3)
    };
    BVHStaticTriangle* staticTriangle = new BVHStaticTriangle(~0u, indices);
    return new BVHStaticGeometry(staticTriangle, staticData);
}

bool
testLineIntersections()
{
    SGVec3f v1(-1, -1, 0);
    SGVec3f v2(1, -1, 0);
    SGVec3f v3(-1, 1, 0);
    SGSharedPtr<BVHNode> node = buildSingleTriangle(v1, v2, v3);

    SGLineSegmentd lineSegment(SGVec3d(0, 0, -1), SGVec3d(0, 0, 1));
    {
        BVHLineSegmentVisitor lineSegmentVisitor(lineSegment);
        node->accept(lineSegmentVisitor);
        if (lineSegmentVisitor.empty())
            return false;
        if (!equivalent(lineSegmentVisitor.getPoint(), SGVec3d(0, 0, 0)))
            return false;
    }

    SGVec3d position(1000, 1000, 1000);
    SGMatrixd matrix(position);
    SGSharedPtr<BVHTransform> transform1 = new BVHTransform;
    transform1->setToWorldTransform(matrix);
    transform1->addChild(node);

    SGSharedPtr<BVHTransform> transform2 = new BVHTransform;
    transform2->setToLocalTransform(matrix);
    transform2->addChild(transform1);

    {
        BVHLineSegmentVisitor lineSegmentVisitor(lineSegment);
        transform2->accept(lineSegmentVisitor);
        if (lineSegmentVisitor.empty())
            return false;
        if (!equivalent(lineSegmentVisitor.getPoint(), SGVec3d(0, 0, 0)))
            return false;
    }

    SGSharedPtr<BVHMotionTransform> transform3 = new BVHMotionTransform;
    transform3->setLinearVelocity(SGVec3d(0, 0, 1));
    transform3->setAngularVelocity(SGVec3d(1, 0, 0));
    transform3->addChild(node);

    {
        BVHLineSegmentVisitor lineSegmentVisitor(lineSegment, 0);
        transform3->accept(lineSegmentVisitor);
        if (lineSegmentVisitor.empty())
            return false;
        if (!equivalent(lineSegmentVisitor.getPoint(), SGVec3d(0, 0, 0)))
            return false;
        if (!equivalent(lineSegmentVisitor.getLinearVelocity(),
                        SGVec3d(0, 1, 1)))
            return false;
        if (!equivalent(lineSegmentVisitor.getAngularVelocity(),
                        SGVec3d(1, 0, 0)))
            return false;
    }

    return true;
}

bool
testNearestPoint()
{
    SGVec3f v1(-1, -1, 0);
    SGVec3f v2(1, -1, 0);
    SGVec3f v3(-1, 1, 0);
    SGSharedPtr<BVHNode> node = buildSingleTriangle(v1, v2, v3);

    SGSphered sphere(SGVec3d(0, 0, -1), 2);
    {
      BVHNearestPointVisitor nearestPointVisitor(sphere, 0);
        node->accept(nearestPointVisitor);
        if (nearestPointVisitor.empty())
            return false;
        if (!equivalent(nearestPointVisitor.getPoint(), SGVec3d(0, 0, 0)))
            return false;
    }

    return true;
}

int
main(int argc, char** argv)
{
    if (!testLineIntersections())
        return EXIT_FAILURE;
    if (!testNearestPoint())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
