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

#ifndef SIMGEAR_PRIMITIVE_COLLECTOR_HXX
#define SIMGEAR_PRIMITIVE_COLLECTOR_HXX

#include <osg/Matrix>
#include <osg/PrimitiveSet>

namespace simgear {

class PrimitiveCollector : public osg::PrimitiveFunctor {
public:
    PrimitiveCollector();
    virtual ~PrimitiveCollector();
    
    void swap(PrimitiveCollector& primitiveFunctor);

    virtual void setVertexArray(unsigned int count, const osg::Vec2* vertices);
    virtual void setVertexArray(unsigned int count, const osg::Vec3* vertices);
    virtual void setVertexArray(unsigned int count, const osg::Vec4* vertices);
    virtual void setVertexArray(unsigned int count, const osg::Vec2d* vertices);
    virtual void setVertexArray(unsigned int count, const osg::Vec3d* vertices);
    virtual void setVertexArray(unsigned int count, const osg::Vec4d* vertices);

    virtual void drawArrays(GLenum mode, GLint first, GLsizei count);
  
    template<typename index_type>
    void drawElementsTemplate(GLenum mode, GLsizei count, const index_type* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLubyte* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLushort* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLuint* indices);

    virtual void begin(GLenum mode);
    virtual void vertex(const osg::Vec2& v);
    virtual void vertex(const osg::Vec3& v);
    virtual void vertex(const osg::Vec4& v);
    virtual void vertex(float x, float y);
    virtual void vertex(float x, float y, float z);
    virtual void vertex(float x, float y, float z, float w);
    virtual void end();

    void addVertex(const osg::Vec3d& v);
    void addVertex(const osg::Vec4d& v);

    void addPoint(unsigned i1);
    void addLine(unsigned i1, unsigned i2);
    void addTriangle(unsigned i1, unsigned i2, unsigned i3);
    void addQuad(unsigned i1, unsigned i2, unsigned i3, unsigned i4);

    /// The callback functions that are called on an apropriate primitive
    virtual void addPoint(const osg::Vec3d& v1) = 0;
    virtual void addLine(const osg::Vec3d& v1, const osg::Vec3d& v2) = 0;
    virtual void addTriangle(const osg::Vec3d& v1, const osg::Vec3d& v2, const osg::Vec3d& v3) = 0;

private:
    std::vector<osg::Vec3d> _vertices;
    GLenum _mode;
};

}

#endif
