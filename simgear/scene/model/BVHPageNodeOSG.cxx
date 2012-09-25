// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "BVHPageNodeOSG.hxx"

#include "../../bvh/BVHPageRequest.hxx"
#include "../../bvh/BVHPager.hxx"

#include <osg/Camera>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Group>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/Transform>
#include <osgDB/ReadFile>

#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/math/SGGeometry.hxx>

#include <simgear/bvh/BVHStaticGeometryBuilder.hxx>

#include "PrimitiveCollector.hxx"

namespace simgear {

class BVHPageNodeOSG::_NodeVisitor : public osg::NodeVisitor {
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

    _NodeVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
    {
        setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    }
    virtual ~_NodeVisitor()
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

    virtual void apply(osg::Geode& geode)
    {
        const BVHMaterial* oldMaterial = pushMaterial(&geode);

        for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
            geode.getDrawable(i)->accept(_primitiveCollector);

        _primitiveCollector.setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Group& group)
    {
        // FIXME optimize this to collapse more leafs

        // push the current active primitive list
        _PrimitiveCollector previousPrimitives;
        _primitiveCollector.swap(previousPrimitives);

        const BVHMaterial* mat = previousPrimitives.getCurrentMaterial();
        _primitiveCollector.setCurrentMaterial(mat);

        NodeVector nodeVector;
        _nodeVector.swap(nodeVector);

        // walk the children
        traverse(group);

        // We know whenever we see a transform, we need to flush the
        // collected bounding volume tree since these transforms are not
        // handled by the plain leafs.
        addBoundingVolumeTreeToNode();

        _nodeVector.swap(nodeVector);

        if (!nodeVector.empty()) {
            if (nodeVector.size() == 1) {
                _nodeVector.push_back(nodeVector.front());
            } else {
                SGSharedPtr<BVHGroup> group = new BVHGroup;
                for (NodeVector::iterator i = nodeVector.begin();
                     i != nodeVector.end(); ++i)
                    group->addChild(i->get());
                _nodeVector.push_back(group);
            }
        }

        // pop the current active primitive list
        _primitiveCollector.swap(previousPrimitives);
    }

    virtual void apply(osg::Transform& transform)
    {
        if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
            return;

        osg::Matrix matrix;
        if (!transform.computeLocalToWorldMatrix(matrix, this))
            return;

        // push the current active primitive list
        _PrimitiveCollector previousPrimitives;
        _primitiveCollector.swap(previousPrimitives);

        const BVHMaterial* mat = previousPrimitives.getCurrentMaterial();
        _primitiveCollector.setCurrentMaterial(mat);

        NodeVector nodeVector;
        _nodeVector.swap(nodeVector);

        // walk the children
        traverse(transform);

        // We know whenever we see a transform, we need to flush the
        // collected bounding volume tree since these transforms are not
        // handled by the plain leafs.
        addBoundingVolumeTreeToNode();

        _nodeVector.swap(nodeVector);

        // pop the current active primitive list
        _primitiveCollector.swap(previousPrimitives);

        if (!nodeVector.empty()) {
            SGSharedPtr<BVHTransform> bvhTransform = new BVHTransform;
            bvhTransform->setToWorldTransform(SGMatrixd(matrix.ptr()));

            for (NodeVector::iterator i = nodeVector.begin();
                 i != nodeVector.end(); ++i)
                bvhTransform->addChild(i->get());

            _nodeVector.push_back(bvhTransform);
        }
    }

    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        apply(static_cast<osg::Transform&>(camera));
    }

    void addBoundingVolumeTreeToNode()
    {
        // Build the flat tree.
        BVHNode* bvNode = _primitiveCollector.buildTreeAndClear();

        // Nothing in there?
        if (!bvNode)
            return;
        if (bvNode->getBoundingSphere().empty())
            return;

        _nodeVector.push_back(bvNode);
    }

    virtual void apply(osg::PagedLOD& pagedLOD)
    {
        float range = std::numeric_limits<float>::max();
        unsigned numFileNames = pagedLOD.getNumFileNames();
        for (unsigned i = 0; i < numFileNames; ++i) {
            if (range < pagedLOD.getMaxRange(i))
                continue;
            range = pagedLOD.getMaxRange(i);
        }

        std::vector<std::string> nameList;
        for (unsigned i = pagedLOD.getNumChildren(); i < pagedLOD.getNumFileNames(); ++i) {
            if (pagedLOD.getMaxRange(i) <= range) {
                nameList.push_back(pagedLOD.getFileName(i));
            }
        }

        if (!nameList.empty()) {
            SGSphered boundingSphere(toVec3d(toSG(pagedLOD.getCenter())), pagedLOD.getRadius());
            _nodeVector.push_back(new BVHPageNodeOSG(nameList, boundingSphere, pagedLOD.getDatabaseOptions()));
        }

        // For the rest that might be already there, traverse this as lod
        apply(static_cast<osg::LOD&>(pagedLOD));
    }

    virtual void apply(osg::ProxyNode& proxyNode)
    {
        unsigned numFileNames = proxyNode.getNumFileNames();
        for (unsigned i = 0; i < numFileNames; ++i) {
            if (i < proxyNode.getNumChildren() && proxyNode.getChild(i))
                continue;
            // FIXME evaluate proxyNode.getDatabasePath()
            osg::ref_ptr<osg::Node> node;
            node = osgDB::readNodeFile(proxyNode.getFileName(i),
              dynamic_cast<const osgDB::Options*>(proxyNode.getDatabaseOptions()));
            if (!node.valid())
                node = new osg::Group;
            if (i < proxyNode.getNumChildren())
                proxyNode.setChild(i, node);
            else
                proxyNode.addChild(node);
        }

        apply(static_cast<osg::Group&>(proxyNode));
    }

    SGSharedPtr<BVHNode> getBVHNode()
    {
        addBoundingVolumeTreeToNode();

        if (_nodeVector.empty())
            return SGSharedPtr<BVHNode>();
        if (_nodeVector.size() == 1)
            return _nodeVector.front();
        SGSharedPtr<BVHGroup> group = new BVHGroup;
        for (NodeVector::iterator i = _nodeVector.begin();
             i != _nodeVector.end(); ++i)
            group->addChild(i->get());
        return group;
    }

private:
    _PrimitiveCollector _primitiveCollector;
    typedef std::vector<SGSharedPtr<BVHNode> > NodeVector;
    NodeVector _nodeVector;
};

class BVHPageNodeOSG::_Request : public BVHPageRequest {
public:
    _Request(BVHPageNodeOSG* pageNode) :
        _pageNode(pageNode)
    {
    }
    virtual ~_Request()
    {
    }
    virtual void load()
    {
        if (!_pageNode.valid())
            return;
        for (std::vector<std::string>::const_iterator i = _pageNode->_modelList.begin();
             i != _pageNode->_modelList.end(); ++i) {
            SGSharedPtr<BVHNode> node = BVHPageNodeOSG::load(*i, _pageNode->_options);
            if (!node.valid())
                continue;
            _nodeVector.push_back(node);
        }
    }
    virtual void insert()
    {
        if (!_pageNode.valid())
            return;
        for (unsigned i = 0; i < _nodeVector.size(); ++i)
            _pageNode->addChild(_nodeVector[i].get());
    }
    virtual BVHPageNode* getPageNode()
    {
        return _pageNode.get();
    }

private:
    SGSharedPtr<BVHPageNodeOSG> _pageNode;
    std::vector<SGSharedPtr<BVHNode> > _nodeVector;
};

SGSharedPtr<BVHNode>
BVHPageNodeOSG::load(const std::string& name, const osg::ref_ptr<osg::Referenced>& options)
{
    osg::ref_ptr<osg::Node> node;
    node = osgDB::readNodeFile(name, dynamic_cast<const osgDB::Options*>(options.get()));
    if (!node.valid())
        return SGSharedPtr<BVHNode>();

    _NodeVisitor nodeVisitor;
    node->accept(nodeVisitor);
    return nodeVisitor.getBVHNode();
}

BVHPageNodeOSG::BVHPageNodeOSG(const std::string& name,
                               const SGSphered& boundingSphere,
                               const osg::ref_ptr<osg::Referenced>& options) :
    _boundingSphere(boundingSphere),
    _options(options)
{
    _modelList.push_back(name);
}

BVHPageNodeOSG::BVHPageNodeOSG(const std::vector<std::string>& nameList,
                               const SGSphered& boundingSphere,
                               const osg::ref_ptr<osg::Referenced>& options) :
    _modelList(nameList),
    _boundingSphere(boundingSphere),
    _options(options)
{
}

BVHPageNodeOSG::~BVHPageNodeOSG()
{
}
    
BVHPageRequest*
BVHPageNodeOSG::newRequest()
{
    return new _Request(this);
}

void
BVHPageNodeOSG::setBoundingSphere(const SGSphered& sphere)
{
    _boundingSphere = sphere;
    invalidateParentBound();
}

SGSphered
BVHPageNodeOSG::computeBoundingSphere() const
{
    return _boundingSphere;
}

void
BVHPageNodeOSG::invalidateBound()
{
    // Don't propagate invalidate bound to its parent
    // Just do this once we get a bounding sphere set
}

}
