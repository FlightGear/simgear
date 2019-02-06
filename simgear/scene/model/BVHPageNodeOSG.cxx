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

#include <osg/Version>
#include <osg/io_utils>
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
    struct _PrimitiveCollector : public PrimitiveCollector {
        _PrimitiveCollector(_NodeVisitor& nodeVisitor) :
            _nodeVisitor(nodeVisitor)
        { }
        virtual ~_PrimitiveCollector()
        { }
        virtual void addPoint(const osg::Vec3d& v1)
        { }
        virtual void addLine(const osg::Vec3d& v1, const osg::Vec3d& v2)
        { }
        virtual void addTriangle(const osg::Vec3d& v1, const osg::Vec3d& v2, const osg::Vec3d& v3)
        { _nodeVisitor.addTriangle(v1, v2, v3); }
    private:
        _NodeVisitor& _nodeVisitor;
    };

    struct _NodeBin {
        SGSharedPtr<BVHNode> getNode(const osg::Matrix& matrix)
        {
            if (_nodeVector.empty())
                return SGSharedPtr<BVHNode>();
            
            if (!matrix.isIdentity()) {
                // If we have a non trivial matrix we need a
                // transform node in any case.
                SGSharedPtr<BVHTransform> transform = new BVHTransform;
                transform->setToWorldTransform(SGMatrixd(matrix.ptr()));
                for (_NodeVector::iterator i = _nodeVector.begin();
                     i != _nodeVector.end(); ++i)
                    transform->addChild(i->get());
                return transform;
            } else {
                // If the matrix is an identity, return the
                // smallest possible subtree.
                if (_nodeVector.size() == 1)
                    return _nodeVector.front();
                SGSharedPtr<BVHGroup> group = new BVHGroup;
                for (_NodeVector::iterator i = _nodeVector.begin();
                     i != _nodeVector.end(); ++i)
                    group->addChild(i->get());
                return group;
            }
        }
        
        void addNode(const SGSharedPtr<BVHNode>& node)
        {
            if (!node.valid())
                return;
            if (node->getBoundingSphere().empty())
                return;
            _nodeVector.push_back(node);
        }
        
    private:
        typedef std::vector<SGSharedPtr<BVHNode> > _NodeVector;
        
        // The current pending node vector.
        _NodeVector _nodeVector;
    };
    
    _NodeVisitor(bool flatten, const osg::Matrix& localToWorldMatrix = osg::Matrix()) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _localToWorldMatrix(localToWorldMatrix),
        _geometryBuilder(new BVHStaticGeometryBuilder),
        _flatten(flatten)
    {
        setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    }
    virtual ~_NodeVisitor()
    {
    }

    void addTriangle(const osg::Vec3d& v1, const osg::Vec3d& v2, const osg::Vec3d& v3)
    {
        _geometryBuilder->addTriangle(toVec3f(toSG(_localToWorldMatrix.preMult(v1))),
                                      toVec3f(toSG(_localToWorldMatrix.preMult(v2))),
                                      toVec3f(toSG(_localToWorldMatrix.preMult(v3))));
    }

    void setCenter(const osg::Vec3& center)
    {
        _centerMatrix.preMultTranslate(center);
        _localToWorldMatrix.postMultTranslate(-center);
        if (1e6 < center.length()) {
            SGGeod geod = SGGeod::fromCart(toVec3d(toSG(center)));
            SGQuatd orientation = SGQuatd::fromLonLat(geod);
            _centerMatrix.preMultRotate(toOsg(orientation));
            _localToWorldMatrix.postMultRotate(toOsg(inverse(orientation)));
        }
    }

    virtual void apply(osg::Geode& geode)
    {
        const BVHMaterial* oldMaterial = _geometryBuilder->getCurrentMaterial();
        if (const BVHMaterial* material = SGMaterialLib::findMaterial(&geode))
            _geometryBuilder->setCurrentMaterial(material);

        _PrimitiveCollector primitiveCollector(*this);
        for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
            geode.getDrawable(i)->accept(primitiveCollector);

        _geometryBuilder->setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Node& node)
    {
        if (_flatten) {
            traverse(node);
        } else {
            _NodeVisitor nodeVisitor(_flatten, _localToWorldMatrix);
            nodeVisitor.traverse(node);
            _nodeBin.addNode(nodeVisitor.getNode(osg::Matrix::identity()));
        }
    }

    virtual void apply(osg::Transform& transform)
    {
        if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
            return;

        // FIXME identify and handle dynamic transforms

        if (_flatten) {
            // propagate the matrix further down into the nodes and
            // build a flat leaf tree as far as possible

            // save away and accumulate the localToWorldMatrix
            osg::Matrix localToWorldMatrix = _localToWorldMatrix;
            if (!transform.computeLocalToWorldMatrix(_localToWorldMatrix, this))
                return;

            traverse(transform);

            _localToWorldMatrix = localToWorldMatrix;
        } else {
            // accumulate the localToWorldMatrix
            osg::Matrix localToWorldMatrix = _localToWorldMatrix;
            if (!transform.computeLocalToWorldMatrix(localToWorldMatrix, this))
                return;

            // evaluate the loca to world matrix here in this group node.
            _NodeVisitor nodeVisitor(_flatten);
            nodeVisitor.traverse(transform);
            _nodeBin.addNode(nodeVisitor.getNode(localToWorldMatrix));
        }
    }

    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        apply(static_cast<osg::Transform&>(camera));
    }

    virtual void apply(osg::PagedLOD& pagedLOD)
    {
        unsigned numFileNames = pagedLOD.getNumFileNames();
        if (_flatten) {
            // In flattening mode treat lod nodes as proxy nodes
            for (unsigned i = 0; i < numFileNames; ++i) {
                if (i < pagedLOD.getNumChildren() && pagedLOD.getChild(i))
                    continue;
                osg::ref_ptr<osg::Node> node;
                if (pagedLOD.getMinRange(i) <= 0) {
                    osg::ref_ptr<const osgDB::Options> options;
                    options = getOptions(pagedLOD.getDatabaseOptions(), pagedLOD.getDatabasePath());
#if OSG_VERSION_LESS_THAN(3,4,0)
                    node = osgDB::readNodeFile(pagedLOD.getFileName(i), options.get());
#else
                    node = osgDB::readRefNodeFile(pagedLOD.getFileName(i), options.get());
#endif
                }
                if (!node.valid())
                    node = new osg::Group;
                if (i < pagedLOD.getNumChildren())
                    pagedLOD.setChild(i, node);
                else
                    pagedLOD.addChild(node);
            }
        } else {
            // in non flattening mode translate to bvh page nodes
            std::vector<std::string> nameList;
            for (unsigned i = pagedLOD.getNumChildren(); i < numFileNames; ++i) {
                if (0 < pagedLOD.getMinRange(i))
                    continue;
                nameList.push_back(pagedLOD.getFileName(i));
            }

            _NodeBin nodeBin;
            if (!nameList.empty()) {
                osg::ref_ptr<const osgDB::Options> options;
                options = getOptions(pagedLOD.getDatabaseOptions(), pagedLOD.getDatabasePath());
                SGSphered boundingSphere(toVec3d(toSG(pagedLOD.getCenter())), pagedLOD.getRadius());
                nodeBin.addNode(new BVHPageNodeOSG(nameList, boundingSphere, options.get()));
            }
            _nodeBin.addNode(nodeBin.getNode(_localToWorldMatrix));
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
            osg::ref_ptr<const osgDB::Options> options;
            options = getOptions(proxyNode.getDatabaseOptions(), proxyNode.getDatabasePath());
            osg::ref_ptr<osg::Node> node;
#if OSG_VERSION_LESS_THAN(3,4,0)
            node = osgDB::readNodeFile(proxyNode.getFileName(i), options.get());
#else
            node = osgDB::readRefNodeFile(proxyNode.getFileName(i), options.get());
#endif
            if (!node.valid())
                node = new osg::Group;
            if (i < proxyNode.getNumChildren())
                proxyNode.setChild(i, node);
            else
                proxyNode.addChild(node);
        }

        apply(static_cast<osg::Group&>(proxyNode));
    }

    static osg::ref_ptr<const osgDB::Options>
    getOptions(const osg::Referenced* referenced, const std::string& databasePath)
    {
        osg::ref_ptr<const osgDB::Options> options = dynamic_cast<const osgDB::Options*>(referenced);
        if (!options.valid())
            options = osgDB::Registry::instance()->getOptions();
        if (databasePath.empty())
            return options;
        osg::ref_ptr<osgDB::Options> writable;
        if (options.valid())
            writable = static_cast<osgDB::Options*>(options->clone(osg::CopyOp()));
        else
            writable = new osgDB::Options;
        writable->getDatabasePathList().push_front(databasePath);
        return writable;
    }

    SGSharedPtr<BVHNode> getNode(const osg::Matrix& matrix = osg::Matrix())
    {
        // Flush any pendig leaf nodes
        if (_geometryBuilder.valid()) {
            _nodeBin.addNode(_geometryBuilder->buildTree());
            _geometryBuilder.clear();
        }

        return _nodeBin.getNode(matrix*_centerMatrix);
    }

private:
    // The part of the accumulated model view matrix that
    // is put into a BVHTransform node.
    osg::Matrix _localToWorldMatrix;
    // The matrix that centers and aligns the leaf.
    osg::Matrix _centerMatrix;

    // The current pending nodes.
    _NodeBin _nodeBin;

    SGSharedPtr<BVHStaticGeometryBuilder> _geometryBuilder;

    bool _flatten;
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
BVHPageNodeOSG::load(const std::string& name, const osg::ref_ptr<const osg::Referenced>& options)
{
    osg::ref_ptr<osg::Node> node;
#if OSG_VERSION_LESS_THAN(3,4,0)
    node = osgDB::readNodeFile(name, dynamic_cast<const osgDB::Options*>(options.get()));
#else
    node = osgDB::readRefNodeFile(name, dynamic_cast<const osgDB::Options*>(options.get()));
#endif
    if (!node.valid())
        return SGSharedPtr<BVHNode>();

    bool flatten = (node->getBound()._radius < 30000);
    _NodeVisitor nodeVisitor(flatten);
    if (flatten)
        nodeVisitor.setCenter(node->getBound()._center);
    node->accept(nodeVisitor);
    return nodeVisitor.getNode();
}

BVHPageNodeOSG::BVHPageNodeOSG(const std::string& name,
                               const SGSphered& boundingSphere,
                               const osg::ref_ptr<const osg::Referenced>& options) :
    _boundingSphere(boundingSphere),
    _options(options)
{
    _modelList.push_back(name);
}

BVHPageNodeOSG::BVHPageNodeOSG(const std::vector<std::string>& nameList,
                               const SGSphered& boundingSphere,
                               const osg::ref_ptr<const osg::Referenced>& options) :
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
