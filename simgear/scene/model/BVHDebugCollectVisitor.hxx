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

#ifndef BVHDebugCollectVisitor_hxx
#define BVHDebugCollectVisitor_hxx

#include <osg/ref_ptr>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PolygonOffset>
#include <osg/PrimitiveSet>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/StateSet>

#include <simgear/math/SGGeometry.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include <simgear/bvh/BVHVisitor.hxx>
#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHGroup.hxx>
#include <simgear/bvh/BVHTransform.hxx>
#include <simgear/bvh/BVHMotionTransform.hxx>
#include <simgear/bvh/BVHStaticGeometry.hxx>

#include <simgear/bvh/BVHStaticData.hxx>

#include <simgear/bvh/BVHStaticNode.hxx>
#include <simgear/bvh/BVHStaticTriangle.hxx>
#include <simgear/bvh/BVHStaticBinary.hxx>

#include <simgear/bvh/BVHBoundingBoxVisitor.hxx>

namespace simgear {

class BVHNode;
class BVHStaticNode;

class BVHDebugCollectVisitor : public BVHVisitor {
public:
    BVHDebugCollectVisitor(const double& time, unsigned level = ~0u) :
        _group(new osg::Group),
        _time(time),
        _level(level),
        _currentLevel(0)
    {
        osg::StateSet* stateSet = _group->getOrCreateStateSet();
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->setAttribute(new osg::Depth(osg::Depth::LESS, 0, 1, false));
        osg::BlendFunc *blendFunc;
        blendFunc = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA,
                                       osg::BlendFunc::DST_ALPHA);
        stateSet->setAttributeAndModes(blendFunc);
        osg::PolygonOffset* polygonOffset = new osg::PolygonOffset(-1, -1);
        stateSet->setAttributeAndModes(polygonOffset);
    }
    virtual ~BVHDebugCollectVisitor()
    { }

    virtual void apply(BVHGroup& node)
    {
        addNodeSphere(node);
        ++_currentLevel;
        node.traverse(*this);
        --_currentLevel;
    }
    virtual void apply(BVHTransform& node)
    {
        addNodeSphere(node);
        osg::ref_ptr<osg::Group> oldGroup = _group;
        osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
        transform->setMatrix(osg::Matrix(node.getToWorldTransform().data()));
        _group = transform;
        ++_currentLevel;
        node.traverse(*this);
        --_currentLevel;
        _group = oldGroup;
        if (transform->getNumChildren())
            _group->addChild(transform.get());
    }
    virtual void apply(BVHMotionTransform& node)
    {
        addNodeSphere(node);
        osg::ref_ptr<osg::Group> oldGroup = _group;
        osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
        transform->setMatrix(osg::Matrix(node.getToWorldTransform(_time).data()));
        _group = transform;
        ++_currentLevel;
        node.traverse(*this);
        --_currentLevel;
        _group = oldGroup;
        if (transform->getNumChildren())
            _group->addChild(transform.get());
    }
    virtual void apply(BVHLineGeometry&)
    {
    }
    virtual void apply(BVHStaticGeometry& node)
    {
        addNodeSphere(node);
        ++_currentLevel;
        node.traverse(*this);
        --_currentLevel;
    }

    virtual void apply(const BVHStaticBinary& node, const BVHStaticData& data)
    {
        addNodeBox(node, data);
        ++_currentLevel;
        node.traverse(*this, data);
        --_currentLevel;
    }
    virtual void apply(const BVHStaticTriangle& node, const BVHStaticData& data)
    {
        addNodeBox(node, data);
        addTriangle(node.getTriangle(data), osg::Vec4(0.5, 0, 0.5, 0.2));
    }

    osg::Node* getNode() const { return _group.get(); }

    static unsigned allLevels() { return ~0u; }
    static unsigned triangles() { return ~0u - 1; }

private:
    void addTriangle(const SGTrianglef& triangle, const osg::Vec4& color)
    {
        if (_level != triangles())
            return;
        
        osg::Geometry* geometry = new osg::Geometry;
        
        osg::Vec3Array* vertices = new osg::Vec3Array;
        vertices->push_back(toOsg(triangle.getVertex(0)));
        vertices->push_back(toOsg(triangle.getVertex(1)));
        vertices->push_back(toOsg(triangle.getVertex(2)));
        
        osg::Vec4Array* colors = new osg::Vec4Array;
        colors->push_back(color);
        
        geometry->setVertexArray(vertices);
        geometry->setColorArray(colors);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(geometry);
        _group->addChild(geode);
    }
    
    void addNodeSphere(const BVHNode& node)
    {
        if (_level != ~0u && _level != _currentLevel)
            return;
        SGSphered sphere = node.getBoundingSphere();
        osg::Sphere* shape = new osg::Sphere;
        shape->setCenter(toOsg(sphere.getCenter()));
        shape->setRadius(sphere.getRadius());
        addShape(shape, osg::Vec4(0.5f, 0.5f, 0.5f, 0.1f));
    }
    
    void addNodeBox(const BVHStaticNode& node, const BVHStaticData& data)
    {
        if (_level != ~0u && _level != _currentLevel)
            return;
        BVHBoundingBoxVisitor bbv;
        node.accept(bbv, data);
        osg::Box* shape = new osg::Box;
        shape->setCenter(toOsg(bbv.getBox().getCenter()));
        shape->setHalfLengths(toOsg((0.5*bbv.getBox().getSize())));
        addShape(shape, osg::Vec4(0.5f, 0, 0, 0.1f));
    }
    
    void addShape(osg::Shape* shape, const osg::Vec4& color)
    {
        osg::ShapeDrawable* shapeDrawable = new osg::ShapeDrawable;
        shapeDrawable->setColor(color);
        shapeDrawable->setShape(shape);
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(shapeDrawable);
        _group->addChild(geode);
    }
    
    osg::ref_ptr<osg::Group> _group;
    const double _time;
    const unsigned _level;
    unsigned _currentLevel;
};

}

#endif
