
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
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/Transform>
#include <osg/TriangleFunctor>

#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/math/SGGeometry.hxx>

#include <simgear/scene/bvh/BVHStaticGeometryBuilder.hxx>

namespace simgear {

class BoundingVolumeBuildVisitor : public osg::NodeVisitor {
public:
    class PFunctor : public osg::PrimitiveFunctor {
    public:
        PFunctor() :
            _modeCache(0)
        {
            _geometryBuilder = new BVHStaticGeometryBuilder;
        }
        virtual ~PFunctor()
        { }

        virtual void setVertexArray(unsigned int count, const osg::Vec2* vertices)
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0], vertices[i][1], 0);
        }

        virtual void setVertexArray(unsigned int count, const osg::Vec3* vertices)
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0], vertices[i][1], vertices[i][2]);
        }

        virtual void setVertexArray(unsigned int count, const osg::Vec4* vertices)
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0]/vertices[i][3],
                                       vertices[i][1]/vertices[i][3],
                                       vertices[i][2]/vertices[i][3]);
        }

        virtual void setVertexArray(unsigned int count, const osg::Vec2d* vertices)
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0], vertices[i][1], 0);
        }

        virtual void setVertexArray(unsigned int count, const osg::Vec3d* vertices)
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0], vertices[i][1], vertices[i][2]);
        }

        virtual void setVertexArray(unsigned int count, const osg::Vec4d* vertices) 
        {
            _vertices.resize(count);
            for (unsigned i = 0; i < count; ++i)
                _vertices[i] = SGVec3f(vertices[i][0]/vertices[i][3],
                                       vertices[i][1]/vertices[i][3],
                                       vertices[i][2]/vertices[i][3]);
        }

        virtual void drawArrays(GLenum mode, GLint first, GLsizei count)
        {
            if (_vertices.empty() || count==0)
                return;

            switch(mode) {
            case (GL_TRIANGLES):
                for (GLsizei i = first; i < first + count; i += 3) {
                    addTriangle(i, i + 1, i + 2);
                }
                break;

            case (GL_TRIANGLE_STRIP):
                for (GLsizei i = 0; i < count; i += 3) {
                    if (i%2)
                        addTriangle(first + i, first + i + 2, first + i + 1);
                    else
                        addTriangle(first + i, first + i + 1, first + i + 2);
                }
                break;

            case (GL_QUADS):
                for (GLsizei i = first; i < first + count; i += 4) {
                    addQuad(i, i + 1, i + 2, i + 3);
                }
                break;

            case (GL_QUAD_STRIP):
                for (GLsizei i = 0; i < count - 2; i += 2) {
                    if (i%4)
                        addQuad(first + i + 1, first + i, i + 3, first + i + 2);
                    else
                        addQuad(first + i, first + i + 1, first + i + 2, first + i + 3);
                }
                break;

            case (GL_POLYGON): // treat polygons as GL_TRIANGLE_FAN
            case (GL_TRIANGLE_FAN):
                for (GLsizei i = first + 2; i < first + count; ++i) {
                    addTriangle(first, i - 1, i);
                }
                break;

            case (GL_POINTS):
                for (GLsizei i = 0; i < count; ++i) {
                    addPoint(first + i);
                }
                break;

            case (GL_LINES):
                for (GLsizei i = first; i < first + count; i += 2) {
                    addLine(i, i + 1);
                }
                break;

            case (GL_LINE_STRIP):
                for (GLsizei i = first; i < first + count; ++i) {
                    addLine(i, i + 1);
                }
                break;

            case (GL_LINE_LOOP):
                for (GLsizei i = first; i < first + count; ++i) {
                    addLine(i, i + 1);
                }
                addLine(first + count - 1, first);
                break;

            default:
                break;
            }
        }
  
        virtual void drawElements(GLenum mode, GLsizei count, const GLubyte* indices)
        {
            drawElementsTemplate(mode, count, indices);
        }

        virtual void drawElements(GLenum mode, GLsizei count, const GLushort* indices)
        {
            drawElementsTemplate(mode, count, indices);
        }

        virtual void drawElements(GLenum mode, GLsizei count, const GLuint* indices)
        {
            drawElementsTemplate(mode, count, indices);
        }

        virtual void begin(GLenum mode)
        {
            _modeCache = mode;
            _vertices.resize(0);
        }

        virtual void vertex(const osg::Vec2& v)
        {
            _vertices.push_back(SGVec3f(v[0], v[1], 0));
        }
        virtual void vertex(const osg::Vec3& v)
        {
            _vertices.push_back(SGVec3f(v[0], v[1], v[2]));
        }
        virtual void vertex(const osg::Vec4& v)
        {
            _vertices.push_back(SGVec3f(v[0]/v[3], v[1]/v[3], v[2]/v[3]));
        }
        virtual void vertex(float x, float y)
        {
            _vertices.push_back(SGVec3f(x, y, 0));
        }
        virtual void vertex(float x, float y, float z)
        {
            _vertices.push_back(SGVec3f(x, y, z));
        }
        virtual void vertex(float x, float y, float z, float w)
        {
            _vertices.push_back(SGVec3f(x/w, y/w, z/w));
        }
        virtual void end()
        {
            if (_vertices.empty())
                return;

            drawArrays(_modeCache, 0, _vertices.size());
        }

        template<typename index_type>
        void drawElementsTemplate(GLenum mode, GLsizei count,
                                  const index_type* indices)
        {
            if (_vertices.empty() || indices == 0 || count == 0)
                return;
    
            switch(mode) {
            case (GL_TRIANGLES):
                for (GLsizei i = 0; i < count; i += 3) {
                    addTriangle(indices[i], indices[i + 1], indices[i + 2]);
                }
                break;

            case (GL_TRIANGLE_STRIP):
                for (GLsizei i = 2; i < count; ++i) {
                    if (i%2)
                        addTriangle(indices[i - 2], indices[i], indices[i - 1]);
                    else
                        addTriangle(indices[i - 2], indices[i - 1], indices[i]);
                }
                break;

            case (GL_QUADS):
                for (GLsizei i = 0; i < count; i += 4) {
                    addQuad(indices[i], indices[i + 1], indices[i + 2], indices[i + 3]);
                }
                break;

            case (GL_QUAD_STRIP):
                for (GLsizei i = 3; i < count; i += 2) {
                    if (i%4)
                        addQuad(indices[i - 2], indices[i - 3], indices[i], indices[i - 1]);
                    else
                        addQuad(indices[i - 3], indices[i - 2], indices[i - 1], indices[i]);
                }
                break;

            case (GL_POLYGON):
            case (GL_TRIANGLE_FAN):
                for (GLsizei i = 2; i < count; ++i) {
                    addTriangle(indices[0], indices[i - 1], indices[i]);
                }
                break;

            case (GL_POINTS):
                for(GLsizei i = 0; i < count; ++i) {
                    addPoint(indices[i]);
                }
                break;

            case (GL_LINES):
                for (GLsizei i = 0; i < count; i += 2) {
                    addLine(indices[i], indices[i + 1]);
                }
                break;

            case (GL_LINE_STRIP):
                for (GLsizei i = 0; i < count; ++i) {
                    addLine(indices[i], indices[i + 1]);
                }
                break;

            case (GL_LINE_LOOP):
                for (GLsizei i = 0; i < count; ++i) {
                    addLine(indices[i], indices[i + 1]);
                }
                addLine(indices[count - 1], indices[0]);
                break;

            default:
                break;
            }
        }    

        void addPoint(unsigned i1)
        {
            addPoint(_vertices[i1]);
        }
        void addLine(unsigned i1, unsigned i2)
        {
            addLine(_vertices[i1], _vertices[i2]);
        }
        void addTriangle(unsigned i1, unsigned i2, unsigned i3)
        {
            addTriangle(_vertices[i1], _vertices[i2], _vertices[i3]);
        }
        void addQuad(unsigned i1, unsigned i2, unsigned i3, unsigned i4)
        {
            addQuad(_vertices[i1], _vertices[i2], _vertices[i3], _vertices[i4]);
        }

        void addPoint(const SGVec3f& v1)
        {
        }
        void addLine(const SGVec3f& v1, const SGVec3f& v2)
        {
        }
        void addTriangle(const SGVec3f& v1, const SGVec3f& v2, const SGVec3f& v3)
        {
            _geometryBuilder->addTriangle(v1, v2, v3);
        }
        void addQuad(const SGVec3f& v1, const SGVec3f& v2,
                     const SGVec3f& v3, const SGVec3f& v4)
        {
            _geometryBuilder->addTriangle(v1, v2, v3);
            _geometryBuilder->addTriangle(v1, v3, v4);
        }

        BVHNode* buildTreeAndClear()
        {
            BVHNode* bvNode = _geometryBuilder->buildTree();
            _geometryBuilder = new BVHStaticGeometryBuilder;
            _vertices.clear();
            return bvNode;
        }

        void swap(PFunctor& primitiveFunctor)
        {
            _vertices.swap(primitiveFunctor._vertices);
            std::swap(_modeCache, primitiveFunctor._modeCache);
            std::swap(_geometryBuilder, primitiveFunctor._geometryBuilder);
        }

        void setCurrentMaterial(const SGMaterial* material)
        {
            _geometryBuilder->setCurrentMaterial(material);
        }
        const SGMaterial* getCurrentMaterial() const
        {
            return _geometryBuilder->getCurrentMaterial();
        }

        std::vector<SGVec3f> _vertices;
        GLenum _modeCache;

        SGSharedPtr<BVHStaticGeometryBuilder> _geometryBuilder;
    };


    // class PrimitiveIndexFunctor
    // {
    // public:

    //     virtual ~PrimitiveIndexFunctor() {}

    //     virtual void setVertexArray(unsigned int count,const Vec2* vertices) = 0;
    //     virtual void setVertexArray(unsigned int count,const Vec3* vertices) = 0;
    //     virtual void setVertexArray(unsigned int count,const Vec4* vertices) = 0;

    //     virtual void setVertexArray(unsigned int count,const Vec2d* vertices) = 0;
    //     virtual void setVertexArray(unsigned int count,const Vec3d* vertices) = 0;
    //     virtual void setVertexArray(unsigned int count,const Vec4d* vertices) = 0;

    //     virtual void drawArrays(GLenum mode,GLint first,GLsizei count) = 0;
    //     virtual void drawElements(GLenum mode,GLsizei count,const GLubyte* indices) = 0;
    //     virtual void drawElements(GLenum mode,GLsizei count,const GLushort* indices) = 0;
    //     virtual void drawElements(GLenum mode,GLsizei count,const GLuint* indices) = 0;

    //     virtual void begin(GLenum mode) = 0;
    //     virtual void vertex(unsigned int pos) = 0;
    //     virtual void end() = 0;
    // };

    BoundingVolumeBuildVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
    {
        setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    }
    ~BoundingVolumeBuildVisitor()
    {
    }

    const SGMaterial* pushMaterial(osg::StateSet* stateSet)
    {
        const SGMaterial* oldMaterial = _primitiveFunctor.getCurrentMaterial();
        const SGMaterial* material = SGMaterialLib::findMaterial(stateSet);
        if (material)
            _primitiveFunctor.setCurrentMaterial(material);
        return oldMaterial;
    }

    void fillWith(osg::Drawable* drawable)
    {
        const SGMaterial* oldMaterial = pushMaterial(drawable->getStateSet());
        drawable->accept(_primitiveFunctor);
        _primitiveFunctor.setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Geode& geode)
    {
        const SGMaterial* oldMaterial = pushMaterial(geode.getStateSet());

        if (!hasBoundingVolumeTree(geode))
            for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
                fillWith(geode.getDrawable(i));

        // Flush the bounding volume tree if we reached the topmost group
        if (getNodePath().size() <= 1)
            addBoundingVolumeTreeToNode(geode);
        _primitiveFunctor.setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Group& group)
    {
        // Note that we do not need to push the already collected list of
        // primitives, since we are now in the topmost node ...

        const SGMaterial* oldMaterial = pushMaterial(group.getStateSet());

        if (!hasBoundingVolumeTree(group))
            traverse(group);

        // Flush the bounding volume tree if we reached the topmost group
        if (getNodePath().size() <= 1)
            addBoundingVolumeTreeToNode(group);

        _primitiveFunctor.setCurrentMaterial(oldMaterial);
    }

    virtual void apply(osg::Transform& transform)
    {
        // push the current active primitive list
        PFunctor previousPrimitives;
        _primitiveFunctor.swap(previousPrimitives);

        const SGMaterial* oldMaterial = pushMaterial(transform.getStateSet());

        // walk the children
        if (!hasBoundingVolumeTree(transform))
            traverse(transform);

        // We know whenever we see a transform, we need to flush the
        // collected bounding volume tree since these transforms are not
        // handled by the plain leafs.
        addBoundingVolumeTreeToNode(transform);

        _primitiveFunctor.setCurrentMaterial(oldMaterial);

        // pop the current active primitive list
        _primitiveFunctor.swap(previousPrimitives);
    }

    virtual void apply(osg::PagedLOD&)
    {
        // Do nothing. In this case we get called by the loading process anyway
    }

    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        apply(static_cast<osg::Transform&>(camera));
    }

    void addBoundingVolumeTreeToNode(osg::Node& node)
    {
        // Build the flat tree.
        BVHNode* bvNode = _primitiveFunctor.buildTreeAndClear();

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
    PFunctor _primitiveFunctor;
};

}

#endif
