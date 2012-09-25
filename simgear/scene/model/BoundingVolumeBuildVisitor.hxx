
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

#ifndef SimGear_BoundingVolumeBuildVisitor_hxx
#define SimGear_BoundingVolumeBuildVisitor_hxx

#include <osg/Camera>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Group>
#include <osg/PagedLOD>
#include <osg/Transform>
#include <osg/TriangleFunctor>

#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/math/SGGeometry.hxx>

#include <simgear/bvh/BVHStaticGeometryBuilder.hxx>

#include "PrimitiveCollector.hxx"

namespace simgear {

class BoundingVolumeBuildVisitor : public osg::NodeVisitor {
public:
    class _PrimitiveCollector : public PrimitiveCollector {
    public:
        _PrimitiveCollector() :
            _geometryBuilder(new BVHStaticGeometryBuilder)
        { }
        virtual ~_PrimitiveCollector()
        { }

        virtual void addPoint(const osg::Vec3d& v1)
        { }
        virtual void addLine(const osg::Vec3d& v1, const osg::Vec3d& v2)
        { }
        virtual void addTriangle(const osg::Vec3d& v1, const osg::Vec3d& v2, const osg::Vec3d& v3)
        {
            _geometryBuilder->addTriangle(toVec3f(toSG(v1)), toVec3f(toSG(v2)), toVec3f(toSG(v3)));
        }

        BVHNode* buildTreeAndClear()
        {
            BVHNode* bvNode = _geometryBuilder->buildTree();
            _geometryBuilder = new BVHStaticGeometryBuilder;
            return bvNode;
        }

        void swap(_PrimitiveCollector& primitiveCollector)
        {
            PrimitiveCollector::swap(primitiveCollector);
            std::swap(_geometryBuilder, primitiveCollector._geometryBuilder);
        }

        void setCurrentMaterial(const BVHMaterial* material)
        {
            _geometryBuilder->setCurrentMaterial(material);
        }
        const BVHMaterial* getCurrentMaterial() const
        {
            return _geometryBuilder->getCurrentMaterial();
        }

        SGSharedPtr<BVHStaticGeometryBuilder> _geometryBuilder;
    };

    BoundingVolumeBuildVisitor(bool dumpIntoLeafs) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _dumpIntoLeafs(dumpIntoLeafs)
    {
        setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    }
    ~BoundingVolumeBuildVisitor()
    {
    }

    const BVHMaterial* pushMaterial(osg::Geode* geode)
    {
        const BVHMaterial* oldMaterial = _primitiveCollector.getCurrentMaterial();
        const BVHMaterial* material = SGMaterialLib::findMaterial(geode);
        if (material)
            _primitiveCollector.setCurrentMaterial(material);
        return oldMaterial;
    }

    void fillWith(osg::Drawable* drawable)
    {
        drawable->accept(_primitiveCollector);
    }

    virtual void apply(osg::Geode& geode)
    {
        if (hasBoundingVolumeTree(geode))
            return;

        const BVHMaterial* oldMaterial = pushMaterial(&geode);

        bool flushHere = getNodePath().size() <= 1 || _dumpIntoLeafs;
        if (flushHere) {
            // push the current active primitive list
            _PrimitiveCollector previousPrimitives;
            _primitiveCollector.swap(previousPrimitives);

            const BVHMaterial* mat = previousPrimitives.getCurrentMaterial();
            _primitiveCollector.setCurrentMaterial(mat);

            // walk the children
            for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
                fillWith(geode.getDrawable(i));

            // Flush the bounding volume tree if we reached the topmost group
            addBoundingVolumeTreeToNode(geode);

            // pop the current active primitive list
            _primitiveCollector.swap(previousPrimitives);
        } else {
            for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
                fillWith(geode.getDrawable(i));
        }

        _primitiveCollector.setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Group& group)
    { traverseAndCollect(group); }

    virtual void apply(osg::Transform& transform)
    { traverseAndDump(transform); }

    virtual void apply(osg::PagedLOD&)
    {
        // Do nothing. In this case we get called by the loading process anyway
    }

    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        traverseAndDump(camera);
    }

    void traverseAndDump(osg::Node& node)
    {
        if (hasBoundingVolumeTree(node))
            return;

        // push the current active primitive list
        _PrimitiveCollector previousPrimitives;
        _primitiveCollector.swap(previousPrimitives);

        const BVHMaterial* mat = previousPrimitives.getCurrentMaterial();
        _primitiveCollector.setCurrentMaterial(mat);

        // walk the children
        traverse(node);

        // We know whenever we see a transform, we need to flush the
        // collected bounding volume tree since these transforms are not
        // handled by the plain leafs.
        addBoundingVolumeTreeToNode(node);

        // pop the current active primitive list
        _primitiveCollector.swap(previousPrimitives);
    }

    void traverseAndCollect(osg::Node& node)
    {
        // Already been here??
        if (hasBoundingVolumeTree(node))
            return;

        // Force a flush of the bvtree if we are in the topmost node.
        if (getNodePath().size() <= 1) {
            traverseAndDump(node);
            return;
        }

        // Note that we do not need to push the already collected list of
        // primitives, since we are now in the topmost node ...

        // walk the children
        traverse(node);
    }

    void addBoundingVolumeTreeToNode(osg::Node& node)
    {
        // Build the flat tree.
        BVHNode* bvNode = _primitiveCollector.buildTreeAndClear();

        // Nothing in there?
        if (!bvNode)
            return;

        SGSceneUserData* userData;
        userData = SGSceneUserData::getOrCreateSceneUserData(&node);
        userData->setBVHNode(bvNode);
    }

    bool hasBoundingVolumeTree(osg::Node& node)
    {
        SGSceneUserData* userData;
        userData = SGSceneUserData::getSceneUserData(&node);
        if (!userData)
            return false;
        if (!userData->getBVHNode())
            return false;
        return true;
    }

private:
    _PrimitiveCollector _primitiveCollector;
    bool _dumpIntoLeafs;
};

}

#endif
